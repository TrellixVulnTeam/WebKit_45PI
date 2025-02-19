// Copyright 2018 The Chromium Authors. All rights reserved.
// Copyright (C) 2018 Apple Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"
#include "DeviceResponseConverter.h"

#if ENABLE(WEB_AUTHN)

#include "AuthenticatorSupportedOptions.h"
#include "CBORReader.h"
#include "CBORWriter.h"
#include <wtf/StdSet.h>
#include <wtf/Vector.h>

namespace fido {
using namespace WebCore;
using CBOR = cbor::CBORValue;

constexpr size_t kResponseCodeLength = 1;

static ProtocolVersion convertStringToProtocolVersion(const String& version)
{
    if (version == kCtap2Version)
        return ProtocolVersion::kCtap;
    if (version == kU2fVersion)
        return ProtocolVersion::kU2f;

    return ProtocolVersion::kUnknown;
}

CtapDeviceResponseCode getResponseCode(const Vector<uint8_t>& buffer)
{
    if (buffer.isEmpty())
        return CtapDeviceResponseCode::kCtap2ErrInvalidCBOR;

    auto code = static_cast<CtapDeviceResponseCode>(buffer[0]);
    return isCtapDeviceResponseCode(code) ? code : CtapDeviceResponseCode::kCtap2ErrInvalidCBOR;
}

static Vector<uint8_t> getCredentialId(const Vector<uint8_t>& authenticatorData)
{
    const size_t credentialIdLengthOffset = kRpIdHashLength + kFlagsLength + kSignCounterLength + kAaguidLength;

    if (authenticatorData.size() < credentialIdLengthOffset + kCredentialIdLengthLength)
        return { };
    size_t credentialIdLength = (static_cast<size_t>(authenticatorData[credentialIdLengthOffset]) << 8) | static_cast<size_t>(authenticatorData[credentialIdLengthOffset + 1]);

    if (authenticatorData.size() < credentialIdLengthOffset + kCredentialIdLengthLength + credentialIdLength)
        return { };
    Vector<uint8_t> credentialId;
    credentialId.reserveInitialCapacity(credentialIdLength);
    auto beginIt = authenticatorData.begin() + credentialIdLengthOffset + kCredentialIdLengthLength;
    credentialId.appendRange(beginIt, beginIt + credentialIdLength);
    return credentialId;
}


// Decodes byte array response from authenticator to CBOR value object and
// checks for correct encoding format.
std::optional<PublicKeyCredentialData> readCTAPMakeCredentialResponse(const Vector<uint8_t>& inBuffer)
{
    if (inBuffer.size() <= kResponseCodeLength)
        return std::nullopt;

    Vector<uint8_t> buffer;
    buffer.append(inBuffer.data() + 1, inBuffer.size() - 1);
    std::optional<CBOR> decodedResponse = cbor::CBORReader::read(buffer);
    if (!decodedResponse || !decodedResponse->isMap())
        return std::nullopt;
    const auto& decodedMap = decodedResponse->getMap();

    auto it = decodedMap.find(CBOR(1));
    if (it == decodedMap.end() || !it->second.isString())
        return std::nullopt;
    auto format = it->second.clone();

    it = decodedMap.find(CBOR(2));
    if (it == decodedMap.end() || !it->second.isByteString())
        return std::nullopt;
    auto authenticatorData = it->second.clone();

    auto credentialId = getCredentialId(authenticatorData.getByteString());
    if (credentialId.isEmpty())
        return std::nullopt;

    it = decodedMap.find(CBOR(3));
    if (it == decodedMap.end() || !it->second.isMap())
        return std::nullopt;
    auto attStmt = it->second.clone();

    CBOR::MapValue attestationObjectMap;
    attestationObjectMap[CBOR("authData")] = WTFMove(authenticatorData);
    attestationObjectMap[CBOR("fmt")] = WTFMove(format);
    attestationObjectMap[CBOR("attStmt")] = WTFMove(attStmt);
    auto attestationObject = cbor::CBORWriter::write(CBOR(WTFMove(attestationObjectMap)));

    return PublicKeyCredentialData { ArrayBuffer::create(credentialId.data(), credentialId.size()), true, nullptr, ArrayBuffer::create(attestationObject.value().data(), attestationObject.value().size()), nullptr, nullptr, nullptr };
}

std::optional<PublicKeyCredentialData> readCTAPGetAssertionResponse(const Vector<uint8_t>& inBuffer)
{
    if (inBuffer.size() <= kResponseCodeLength)
        return std::nullopt;

    Vector<uint8_t> buffer;
    buffer.append(inBuffer.data() + 1, inBuffer.size() - 1);
    std::optional<CBOR> decodedResponse = cbor::CBORReader::read(buffer);

    if (!decodedResponse || !decodedResponse->isMap())
        return std::nullopt;

    auto& responseMap = decodedResponse->getMap();

    RefPtr<ArrayBuffer> credentialId;
    auto it = responseMap.find(CBOR(1));
    if (it != responseMap.end() && it->second.isMap()) {
        auto& credential = it->second.getMap();
        auto itr = credential.find(CBOR(kCredentialIdKey));
        if (itr == credential.end() || !itr->second.isByteString())
            return std::nullopt;
        auto& id = itr->second.getByteString();
        credentialId = ArrayBuffer::create(id.data(), id.size());
    }

    it = responseMap.find(CBOR(2));
    if (it == responseMap.end() || !it->second.isByteString())
        return std::nullopt;
    auto& authData = it->second.getByteString();

    it = responseMap.find(CBOR(3));
    if (it == responseMap.end() || !it->second.isByteString())
        return std::nullopt;
    auto& signature = it->second.getByteString();

    RefPtr<ArrayBuffer> userHandle;
    {
        it = responseMap.find(CBOR(4));
        if (it == responseMap.end() || !it->second.isMap())
            return std::nullopt;
        auto& user = it->second.getMap();
        auto itr = user.find(CBOR(kEntityIdMapKey));
        if (itr == user.end() || !itr->second.isByteString())
            return std::nullopt;
        auto& id = itr->second.getByteString();
        userHandle = ArrayBuffer::create(id.data(), id.size());
    }

    return PublicKeyCredentialData { WTFMove(credentialId), false, nullptr, nullptr, ArrayBuffer::create(authData.data(), authData.size()), ArrayBuffer::create(signature.data(), signature.size()), WTFMove(userHandle) };
}

std::optional<AuthenticatorGetInfoResponse> readCTAPGetInfoResponse(const Vector<uint8_t>& inBuffer)
{
    if (inBuffer.size() <= kResponseCodeLength || getResponseCode(inBuffer) != CtapDeviceResponseCode::kSuccess)
        return std::nullopt;

    Vector<uint8_t> buffer;
    buffer.append(inBuffer.data() + 1, inBuffer.size() - 1);
    std::optional<CBOR> decodedResponse = cbor::CBORReader::read(buffer);
    if (!decodedResponse || !decodedResponse->isMap())
        return std::nullopt;
    const auto& responseMap = decodedResponse->getMap();

    auto it = responseMap.find(CBOR(1));
    if (it == responseMap.end() || !it->second.isArray() || it->second.getArray().size() > 2)
        return std::nullopt;
    StdSet<ProtocolVersion> protocolVersions;
    for (const auto& version : it->second.getArray()) {
        if (!version.isString())
            return std::nullopt;

        auto protocol = convertStringToProtocolVersion(version.getString());
        if (protocol == ProtocolVersion::kUnknown) {
            LOG_ERROR("Unexpected protocol version received.");
            continue;
        }

        if (!protocolVersions.insert(protocol).second)
            return std::nullopt;
    }
    if (protocolVersions.empty())
        return std::nullopt;

    it = responseMap.find(CBOR(3));
    if (it == responseMap.end() || !it->second.isByteString() || it->second.getByteString().size() != kAaguidLength)
        return std::nullopt;

    AuthenticatorGetInfoResponse response(WTFMove(protocolVersions), Vector<uint8_t>(it->second.getByteString()));

    it = responseMap.find(CBOR(2));
    if (it != responseMap.end()) {
        if (!it->second.isArray())
            return std::nullopt;

        Vector<String> extensions;
        for (const auto& extension : it->second.getArray()) {
            if (!extension.isString())
                return std::nullopt;

            extensions.append(extension.getString());
        }
        response.setExtensions(WTFMove(extensions));
    }

    AuthenticatorSupportedOptions options;
    it = responseMap.find(CBOR(4));
    if (it != responseMap.end()) {
        if (!it->second.isMap())
            return std::nullopt;
        const auto& optionMap = it->second.getMap();
        auto optionMapIt = optionMap.find(CBOR(kPlatformDeviceMapKey));
        if (optionMapIt != optionMap.end()) {
            if (!optionMapIt->second.isBool())
                return std::nullopt;

            options.setIsPlatformDevice(optionMapIt->second.getBool());
        }

        optionMapIt = optionMap.find(CBOR(kResidentKeyMapKey));
        if (optionMapIt != optionMap.end()) {
            if (!optionMapIt->second.isBool())
                return std::nullopt;

            options.setSupportsResidentKey(optionMapIt->second.getBool());
        }

        optionMapIt = optionMap.find(CBOR(kUserPresenceMapKey));
        if (optionMapIt != optionMap.end()) {
            if (!optionMapIt->second.isBool())
                return std::nullopt;

            options.setUserPresenceRequired(optionMapIt->second.getBool());
        }

        optionMapIt = optionMap.find(CBOR(kUserVerificationMapKey));
        if (optionMapIt != optionMap.end()) {
            if (!optionMapIt->second.isBool())
                return std::nullopt;

            if (optionMapIt->second.getBool())
                options.setUserVerificationAvailability(AuthenticatorSupportedOptions::UserVerificationAvailability::kSupportedAndConfigured);
            else
                options.setUserVerificationAvailability(AuthenticatorSupportedOptions::UserVerificationAvailability::kSupportedButNotConfigured);
        }

        optionMapIt = optionMap.find(CBOR(kClientPinMapKey));
        if (optionMapIt != optionMap.end()) {
            if (!optionMapIt->second.isBool())
                return std::nullopt;

            if (optionMapIt->second.getBool())
                options.setClientPinAvailability(AuthenticatorSupportedOptions::ClientPinAvailability::kSupportedAndPinSet);
            else
                options.setClientPinAvailability(AuthenticatorSupportedOptions::ClientPinAvailability::kSupportedButPinNotSet);
        }
        response.setOptions(WTFMove(options));
    }

    it = responseMap.find(CBOR(5));
    if (it != responseMap.end()) {
        if (!it->second.isUnsigned())
            return std::nullopt;

        response.setMaxMsgSize(it->second.getUnsigned());
    }

    it = responseMap.find(CBOR(6));
    if (it != responseMap.end()) {
        if (!it->second.isArray())
            return std::nullopt;

        Vector<uint8_t> supportedPinProtocols;
        for (const auto& protocol : it->second.getArray()) {
            if (!protocol.isUnsigned())
                return std::nullopt;

            supportedPinProtocols.append(protocol.getUnsigned());
        }
        response.setPinProtocols(WTFMove(supportedPinProtocols));
    }

    return WTFMove(response);
}

} // namespace fido

#endif // ENABLE(WEB_AUTHN)
