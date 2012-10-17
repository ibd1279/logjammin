/*
 \file logjam/Tls_credentials.cpp
 \brief Logjam TLS Credentials Implementation
 \author Jason Watson

 Copyright (c) 2012, Jason Watson
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

#include "Tls_credentials.h"

namespace logjam
{
    Tls_certificate_credentials::Tls_certificate_credentials()
    {
        gnutls_certificate_allocate_credentials(&certificate_credentials_);
    }

    Tls_certificate_credentials::Tls_certificate_credentials(Tls_certificate_credentials&& o) :
            certificate_credentials_(o.certificate_credentials_)
    {
        o.certificate_credentials_ = nullptr;
    }

    Tls_certificate_credentials::~Tls_certificate_credentials()
    {
        if (certificate_credentials_)
        {
            gnutls_certificate_free_credentials(certificate_credentials_);
        }
    }

    Tls_certificate_credentials& Tls_certificate_credentials::operator=(Tls_certificate_credentials&& o)
    {
        std::swap(certificate_credentials_, o.certificate_credentials_);
        return *this;
    }

    void Tls_certificate_credentials::set_x509_trust_file(const std::string& cafile,
            gnutls_x509_crt_fmt_t fmt)
    {
        gnutls_certificate_set_x509_trust_file(certificate_credentials_,
                cafile.c_str(), fmt);
    }

    void Tls_certificate_credentials::set_x509_key_file(const std::string& cert_file,
            const std::string& key_file,
            gnutls_x509_crt_fmt_t fmt)
    {
        gnutls_certificate_set_x509_key_file(certificate_credentials_,
                cert_file.c_str(),
                key_file.c_str(),
                fmt);
    }

    void* Tls_certificate_credentials::gnutls_ptr()
    {
        return certificate_credentials_;
    }

    gnutls_credentials_type_t Tls_certificate_credentials::gnutls_type() const
    {
        return GNUTLS_CRD_CERTIFICATE;
    }

    void Tls_certificate_credentials::configure_key_exchange(Tls_key_exchange_diffie_hellman& kx)
    {
        gnutls_certificate_set_dh_params(certificate_credentials_,
                (gnutls_dh_params_t) kx.gnutls_ptr());
    }

    Tls_credentials_anonymous_client::Tls_credentials_anonymous_client()
    {
        gnutls_anon_allocate_client_credentials(&anonymous_credentials_);
    }

    Tls_credentials_anonymous_client::Tls_credentials_anonymous_client(Tls_credentials_anonymous_client&& o) :
            anonymous_credentials_(o.anonymous_credentials_)
    {
        o.anonymous_credentials_ = nullptr;
    }

    Tls_credentials_anonymous_client::~Tls_credentials_anonymous_client()
    {
        if (anonymous_credentials_)
        {
            gnutls_anon_free_client_credentials(anonymous_credentials_);
        }
    }

    Tls_credentials_anonymous_client& Tls_credentials_anonymous_client::operator=(Tls_credentials_anonymous_client&& o)
    {
        std::swap(anonymous_credentials_, o.anonymous_credentials_);
        return *this;
    }

    void* Tls_credentials_anonymous_client::gnutls_ptr()
    {
        return anonymous_credentials_;
    }

    gnutls_credentials_type_t Tls_credentials_anonymous_client::gnutls_type() const
    {
        return GNUTLS_CRD_ANON;
    }

    Tls_credentials_anonymous_server::Tls_credentials_anonymous_server()
    {
        gnutls_anon_allocate_server_credentials(&anonymous_credentials_);
    }

    Tls_credentials_anonymous_server::Tls_credentials_anonymous_server(Tls_credentials_anonymous_server&& o) :
            anonymous_credentials_(o.anonymous_credentials_)
    {
        o.anonymous_credentials_ = nullptr;
    }

    Tls_credentials_anonymous_server::~Tls_credentials_anonymous_server()
    {
        if (anonymous_credentials_)
        {
            gnutls_anon_free_server_credentials(anonymous_credentials_);
        }
    }

    Tls_credentials_anonymous_server& Tls_credentials_anonymous_server::operator=(Tls_credentials_anonymous_server&& o)
    {
        std::swap(anonymous_credentials_, o.anonymous_credentials_);
        return *this;
    }

    void* Tls_credentials_anonymous_server::gnutls_ptr()
    {
        return anonymous_credentials_;
    }

    gnutls_credentials_type_t Tls_credentials_anonymous_server::gnutls_type() const
    {
        return GNUTLS_CRD_ANON;
    }

    void Tls_credentials_anonymous_server::configure_key_exchange(Tls_key_exchange_diffie_hellman& kx)
    {
        gnutls_anon_set_server_dh_params(anonymous_credentials_, (gnutls_dh_params_t)kx.gnutls_ptr());
    }
};