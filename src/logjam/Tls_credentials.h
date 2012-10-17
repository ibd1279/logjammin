#pragma once
/*
 \file logjam/Tls_credentials.h
 \brief Logjam TLS Credentials Header
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

extern "C"
{
#include "gnutls/gnutls.h"
}
#include <cassert>
#include <string>

namespace logjam
{
    //! Diffie-Hellman Key Exchange Class.
    /*!
     \par
     Used to configure Key exchange for credentials.
     \author Jason Watson
     \since 0.2
     \date October 15, 2012
     */
    class Tls_key_exchange_diffie_hellman
    {
    public:
        //! Default number of bits used in Diffie-Hellman key exchange.
        constexpr static size_t k_bits_default = 1024;

        //! Create a new key exchange.
        /*!
         \param bits The number of bits to use for DH values.
         */
        Tls_key_exchange_diffie_hellman(size_t bits = k_bits_default) : params_(nullptr), bits_(bits)
        {
            gnutls_dh_params_init(&params_);
            regenerate();
        }

        //! Deleted copy constructor.
        /*!
         This object deals with C pointers, so copying isn't trivial
         \param o The other object.
         */
        Tls_key_exchange_diffie_hellman(const Tls_key_exchange_diffie_hellman& o) = delete;

        //! Move constructor.
        /*!
         \param o The other object.
         */
        Tls_key_exchange_diffie_hellman(Tls_key_exchange_diffie_hellman&& o) : params_(o.params_), bits_(o.bits_)
        {
            o.params_ = nullptr;
        }

        //! Destructor.
        ~Tls_key_exchange_diffie_hellman()
        {
            gnutls_dh_params_deinit(params_);
        }

        //! Generate new DH key exchange values.
        virtual void regenerate()
        {
            gnutls_dh_params_generate2(params_, bits_);
        }

        //! Number of bits.
        /*!
         \return Number of bits.
         */
        virtual size_t bits()
        {
            return bits_;
        }

        //! Get the underlying C pointer.
        /*!
         This is mainly used by other TLS classes that need to use the C pointer
         for a function parameter.
         \return The gnutls pointer.
         */
        virtual void* gnutls_ptr()
        {
            return params_;
        }
    private:
        gnutls_dh_params_t params_;
        size_t bits_;
    };

    //! Abstract base class for TLS Credentials.
    /*!
     \par
     provides an abstract parent class for credential classes. This is mostly
     used to provide access to the underlying gnutls structures.
     \author Jason Watson
     \since 0.2
     \date October 12, 2012
     */
    class Tls_credentials
    {
    public:
        //! Get the gnutls credential type.
        /*!
         \return The gnutls credential type.
         */
        virtual gnutls_credentials_type_t gnutls_type() const = 0;

        //! Get the gnutls pointer.
        /*!
         \return The gnutls pointer.
         */
        virtual void* gnutls_ptr() = 0;
    };

    //! TLS Certificate Credentials class.
    /*!
     \par
     Wrapper for TLS certificate data structures.
     \author Jason Watson
     \since 0.2
     \date September 28, 2012
     */
    class Tls_certificate_credentials : public Tls_credentials
    {
    public:
        //! Certificate format for PEM.
        constexpr static gnutls_x509_crt_fmt_t k_x509_format_pem = GNUTLS_X509_FMT_PEM;

        //! Certificate format for DER.
        constexpr static gnutls_x509_crt_fmt_t k_x509_format_der = GNUTLS_X509_FMT_DER;
        Tls_certificate_credentials();
        Tls_certificate_credentials(const Tls_certificate_credentials& o) = delete;
        Tls_certificate_credentials(Tls_certificate_credentials&& o);
        virtual ~Tls_certificate_credentials();
        Tls_certificate_credentials& operator=(const Tls_certificate_credentials& o) = delete;
        Tls_certificate_credentials& operator=(Tls_certificate_credentials&& o);
        void set_x509_trust_file(const std::string& cafile, gnutls_x509_crt_fmt_t fmt);
        void set_x509_key_file(const std::string& cert_file, const std::string& key_file, gnutls_x509_crt_fmt_t fmt);
        virtual void* gnutls_ptr() override;
        virtual gnutls_credentials_type_t gnutls_type() const override;
        void configure_key_exchange(Tls_key_exchange_diffie_hellman& kx);
    private:
        gnutls_certificate_credentials_t certificate_credentials_;
    }; // class logjam::Tls_certificate_credentials

    //! TLS Anonymous Client Credentials class.
    /*!
     \par
     Wrapper for TLS anonymous client data structures.
     \author Jason Watson
     \since 0.2
     \date October 5, 2012
     */
    class Tls_credentials_anonymous_client : public Tls_credentials
    {
    public:
        Tls_credentials_anonymous_client();
        Tls_credentials_anonymous_client(const Tls_credentials_anonymous_client& o) = delete;
        Tls_credentials_anonymous_client(Tls_credentials_anonymous_client&& o);
        virtual ~Tls_credentials_anonymous_client();
        Tls_credentials_anonymous_client& operator=(const Tls_credentials_anonymous_client& o) = delete;
        Tls_credentials_anonymous_client& operator=(Tls_credentials_anonymous_client&& o);
        virtual void* gnutls_ptr() override;
        virtual gnutls_credentials_type_t gnutls_type() const override;
    private:
        gnutls_anon_client_credentials_t anonymous_credentials_;
    };

    //! TLS Anonymous Server Credentials class.
    /*!
     \par
     Wrapper for TLS anonymous server data structures.
     \author Jason Watson
     \since 0.2
     \date October 12, 2012
     */
    class Tls_credentials_anonymous_server : public Tls_credentials
    {
    public:
        Tls_credentials_anonymous_server();
        Tls_credentials_anonymous_server(const Tls_credentials_anonymous_server& o) = delete;
        Tls_credentials_anonymous_server(Tls_credentials_anonymous_server&& o);
        virtual ~Tls_credentials_anonymous_server();
        Tls_credentials_anonymous_server& operator=(const Tls_credentials_anonymous_server& o) = delete;
        Tls_credentials_anonymous_server& operator=(Tls_credentials_anonymous_server&& o);
        virtual void* gnutls_ptr() override;
        virtual gnutls_credentials_type_t gnutls_type() const override;
        void configure_key_exchange(Tls_key_exchange_diffie_hellman& kx);
    private:
        gnutls_anon_server_credentials_t anonymous_credentials_;
    };

    //! TLS adapter for reusing credentials.
    /*!
     \par
     In certain situations, like a server, you will want to reuse the credentials
     object between different connections (the server normally hasn't changed
     its identity between connected sessions). This adapter provides a way to
     reuse a credentials object between different session objects.
     \tparam TCred The credentials class to provide reuse adapting for.
     \author Jason Watson
     \since 0.2
     \date October 5, 2012
     */
    template<class TCred>
    class Tls_credentials_reuse_adapter : public Tls_credentials
    {
    public:
        Tls_credentials_reuse_adapter() : credentials_(nullptr)
        {
        }
        Tls_credentials_reuse_adapter(const Tls_credentials_reuse_adapter& o) :
                credentials_(o.credentials_)
        {
        }
        Tls_credentials_reuse_adapter(Tls_credentials_reuse_adapter&& o) :
                credentials_(o.credentials_)
        {
        }
        ~Tls_credentials_reuse_adapter()
        {
        }
        Tls_credentials_reuse_adapter& operator=(const Tls_credentials_reuse_adapter& o)
        {
            credentials_ = o.credentials_;
        }
        Tls_credentials_reuse_adapter& operator=(Tls_credentials_reuse_adapter&& o)
        {
            credentials_ = o.credentials_;
        }
        TCred* get()
        {
            return credentials_;
        }
        void set(TCred* ptr)
        {
            credentials_ = ptr;
        }
        virtual void* gnutls_ptr() override
        {
            assert(credentials_ != nullptr);
            return credentials_->gnutls_ptr();
        }
        virtual gnutls_credentials_type_t gnutls_type() const override
        {
            assert(credentials_ != nullptr);
            return credentials_->gnutls_type();
        }
    private:
        TCred* credentials_;
    };
}; // namespace logjam