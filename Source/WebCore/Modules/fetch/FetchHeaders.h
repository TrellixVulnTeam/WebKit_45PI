/*
 * Copyright (C) 2016 Canon Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Canon Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "ExceptionOr.h"
#include "HTTPHeaderMap.h"
#include <wtf/HashTraits.h>
#include <wtf/Variant.h>
#include <wtf/Vector.h>

namespace WebCore {

class FetchHeaders : public RefCounted<FetchHeaders> {
public:
    enum class Guard {
        None,
        Immutable,
        Request,
        RequestNoCors,
        Response
    };

    using Init = Variant<Vector<Vector<String>>, Vector<WTF::KeyValuePair<String, String>>>;
    static ExceptionOr<Ref<FetchHeaders>> create(std::optional<Init>&&);

    static Ref<FetchHeaders> create(Guard guard = Guard::None, HTTPHeaderMap&& headers = { }) { return adoptRef(*new FetchHeaders { guard, WTFMove(headers) }); }
    static Ref<FetchHeaders> create(const FetchHeaders& headers) { return adoptRef(*new FetchHeaders { headers }); }

    ExceptionOr<void> append(const String& name, const String& value);
    ExceptionOr<void> remove(const String&);
    ExceptionOr<String> get(const String&) const;
    ExceptionOr<bool> has(const String&) const;
    ExceptionOr<void> set(const String& name, const String& value);

    ExceptionOr<void> fill(const Init&);
    ExceptionOr<void> fill(const FetchHeaders&);
    void filterAndFill(const HTTPHeaderMap&, Guard);

    String fastGet(HTTPHeaderName name) const { return m_headers.get(name); }
    bool fastHas(HTTPHeaderName name) const { return m_headers.contains(name); }
    void fastSet(HTTPHeaderName name, const String& value) { m_headers.set(name, value); }

    class Iterator {
    public:
        explicit Iterator(FetchHeaders&);
        std::optional<WTF::KeyValuePair<String, String>> next();

    private:
        Ref<FetchHeaders> m_headers;
        size_t m_currentIndex { 0 };
        Vector<String> m_keys;
    };
    Iterator createIterator() { return Iterator { *this }; }

    const HTTPHeaderMap& internalHeaders() const { return m_headers; }

    void setGuard(Guard);
    Guard guard() const { return m_guard; }

private:
    FetchHeaders(Guard, HTTPHeaderMap&&);
    FetchHeaders(const FetchHeaders&);

    Guard m_guard;
    HTTPHeaderMap m_headers;
};

inline FetchHeaders::FetchHeaders(Guard guard, HTTPHeaderMap&& headers)
    : m_guard(guard)
    , m_headers(WTFMove(headers))
{
}

inline FetchHeaders::FetchHeaders(const FetchHeaders& other)
    : RefCounted<FetchHeaders>()
    , m_guard(other.m_guard)
    , m_headers(other.m_headers)
{
}

inline void FetchHeaders::setGuard(Guard guard)
{
    ASSERT(!m_headers.size());
    m_guard = guard;
}

} // namespace WebCore

namespace WTF {

template<> struct EnumTraits<WebCore::FetchHeaders::Guard> {
    using values = EnumValues<
    WebCore::FetchHeaders::Guard,
    WebCore::FetchHeaders::Guard::None,
    WebCore::FetchHeaders::Guard::Immutable,
    WebCore::FetchHeaders::Guard::Request,
    WebCore::FetchHeaders::Guard::RequestNoCors,
    WebCore::FetchHeaders::Guard::Response
    >;
};

}
