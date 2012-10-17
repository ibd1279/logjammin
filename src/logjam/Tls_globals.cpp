/*!
 \file logjam/Tls_globals.cpp
 \brief Client TLS abstraction for globals.
 \author Jason Watson

 Copyright (c) 2010, Jason Watson
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of the LogJammin nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "logjam/Tls_globals.h"
#include "lj/Exception.h"
#include "lj/Log.h"
extern "C"
{
#include "gnutls/gnutls.h"
}
#include <sstream>

namespace // anonymous
{

    void tls_audit_logger(gnutls_session_t session, const char* msg)
    {
        size_t len = strlen(msg);
        while (msg[len] == '\r' || msg[len] == '\n')
        {
            len--;
        }
        std::string trimmed_msg(msg, len);
        lj::log::format<lj::Critical>("TLS AUDIT: %s [%s]").end(trimmed_msg, session);
    }

    void tls_debug_logger(int level, const char* msg)
    {
        size_t len = strlen(msg);
        while (msg[len] == '\r' || msg[len] == '\n' || msg[len] == 0)
        {
            len--;
        }
        std::string trimmed_msg(msg, len + 1);

        switch (level)
        {
            case 0:
            case 1:
                lj::log::out<lj::Error>(trimmed_msg);
                break;
            case 2:
            case 3:
                lj::log::out<lj::Warning>(trimmed_msg);
                break;
            case 4:
            case 5:
                lj::log::out<lj::Notice>(trimmed_msg);
                break;
            case 6:
            case 7:
                lj::log::out<lj::Info>(trimmed_msg);
                break;
            case 8:
            case 9:
            default:
                lj::log::out<lj::Debug>(trimmed_msg);
                break;
        }
    }
}; // namespace anonymous

namespace logjam
{

    std::string Tls_exception::str() const
    {
        std::ostringstream oss;
        oss << lj::Exception::str();
        if (code() < 0)
        {
            oss << " [" << gnutls_strerror(code()) << "].";
        }
        return oss.str();
    }

    Tls_globals::Tls_globals()
    {
        int result = gnutls_global_init();
        if (result != GNUTLS_E_SUCCESS)
        {
            std::string msg("Unable to initialize the TLS Library: [");
            msg.append(gnutls_strerror(result));
            msg.append("]");
            throw LJ__Exception(msg);
        }
        gnutls_global_set_log_level(10);
        gnutls_global_set_log_function(&tls_debug_logger);
        gnutls_global_set_audit_log_function(&tls_audit_logger);
        if (nullptr == gnutls_check_version("3.0.23"))
        {
            throw logjam::Tls_exception("gnutls version 3.0.23 or higher is required.", 0);
        }
    }

    Tls_globals::~Tls_globals()
    {
    }

}; // namespace logjam