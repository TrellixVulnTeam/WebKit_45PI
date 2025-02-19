/*
 * Copyright (C) 2015, 2016 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// @conditional=ENABLE(WEB_RTC)

function initializeRTCPeerConnection(configuration)
{
    "use strict";

    if (configuration == null)
        configuration = {};
    else if (!@isObject(configuration))
        @throwTypeError("RTCPeerConnection argument must be a valid dictionary");

    this.@initializeWith(configuration);
    @putByIdDirectPrivate(this, "operations", []);

    return this;
}

function createOffer()
{
    "use strict";

    if (!@isRTCPeerConnection(this))
        return @Promise.@reject(@makeThisTypeError("RTCPeerConnection", "createOffer"));

    const peerConnection = this;

    return @callbacksAndDictionaryOverload(arguments, "createOffer", function (options) {
        return @enqueueOperation(peerConnection, function () {
            return peerConnection.@queuedCreateOffer(options);
        });
    });
}

function createAnswer()
{
    "use strict";

    if (!@isRTCPeerConnection(this))
        return @Promise.@reject(@makeThisTypeError("RTCPeerConnection", "createAnswer"));

    const peerConnection = this;

    return @callbacksAndDictionaryOverload(arguments, "createAnswer", function (options) {
        return @enqueueOperation(peerConnection, function () {
            return peerConnection.@queuedCreateAnswer(options);
        });
    });
}

function setLocalDescription()
{
    "use strict";

    if (!@isRTCPeerConnection(this))
        return @Promise.@reject(@makeThisTypeError("RTCPeerConnection", "setLocalDescription"));

    const peerConnection = this;

    // FIXME 169644: According the spec, we should throw when receiving a RTCSessionDescription.
    const objectInfo = {
        "constructor": @RTCSessionDescription,
        "argName": "description",
        "argType": "RTCSessionDescription",
        "maybeDictionary": "true"
    };
    return @objectAndCallbacksOverload(arguments, "setLocalDescription", objectInfo, function (description) {
        return @enqueueOperation(peerConnection, function () {
            return peerConnection.@queuedSetLocalDescription(description);
        });
    });
}

function setRemoteDescription()
{
    "use strict";

    if (!@isRTCPeerConnection(this))
        return @Promise.@reject(@makeThisTypeError("RTCPeerConnection", "setRemoteDescription"));

    const peerConnection = this;

    // FIXME: According the spec, we should only expect RTCSessionDescriptionInit.
    const objectInfo = {
        "constructor": @RTCSessionDescription,
        "argName": "description",
        "argType": "RTCSessionDescription",
        "maybeDictionary": "true"
    };
    return @objectAndCallbacksOverload(arguments, "setRemoteDescription", objectInfo, function (description) {
        return @enqueueOperation(peerConnection, function () {
            return peerConnection.@queuedSetRemoteDescription(description);
        });
    });
}

function addIceCandidate(candidate)
{
    "use strict";

    if (!@isRTCPeerConnection(this))
        return @Promise.@reject(@makeThisTypeError("RTCPeerConnection", "addIceCandidate"));

    if (arguments.length < 1)
        return @Promise.@reject(new @TypeError("Not enough arguments"));

    const peerConnection = this;

    const objectInfo = {
        "constructor": @RTCIceCandidate,
        "argName": "candidate",
        "argType": "RTCIceCandidate",
        "maybeDictionary": "true",
        "defaultsToNull" : "true"
    };
    return @objectAndCallbacksOverload(arguments, "addIceCandidate", objectInfo, function (candidate) {
        return @enqueueOperation(peerConnection, function () {
            return peerConnection.@queuedAddIceCandidate(candidate);
        });
    });
}
