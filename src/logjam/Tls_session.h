#pragma once
/*
 \file logjamd/Tls_session.h
 \brief Logjam TLS Session Header
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

#include "logjam/Tls_credentials.h"
#include "logjam/Tls_globals.h"
extern "C"
{
#include "gnutls/gnutls.h"
}
#include <sstream>

namespace logjam
{

    //! TLS session class.
    /*!
     \par
     Wrapper for TLS session data structure.
     \tparam TCred The credentials class to use with this session.
     \author Jason Watson
     \since 0.2
     \date September 28, 2012
     */
    template <class TCred>
    class Tls_session
    {
    public:
        constexpr static unsigned int k_server = GNUTLS_SERVER; //!< Server session flag.
        constexpr static unsigned int k_client = GNUTLS_CLIENT; //!< Client session flag.
        //! Create a new TLS Session object.
        /*!
         \par
         The new session is associated with the \c TCred credentials.
         \par
         The flags are currently borrowed directly from gnutls. See the gnutls
         for more up to date information:
         \li \c GNUTLS_SERVER: Connection end is a server.
         \li \c GNUTLS_CLIENT: Connection end is a client.
         \li \c GNUTLS_DATAGRAM: Connection is datagram oriented (DTLS).
         \li \c GNUTLS_NONBLOCK: Connection should not block (DTLS).
         \param flags The flags used to initialize the session.
         \sa gnutls.h
         */
        explicit Tls_session(unsigned int flags) :
                session_(nullptr),
                credentials_(),
                is_setup_(false),
                medium_ret_(0)
        {
            gnutls_init(&session_, flags);
        }

        //! Explicitly deleted copy constructor.
        /*!
         \param o The other object.
         */
        Tls_session(const Tls_session<TCred>& o) = delete;

        //! Move constructor.
        /*!
         \param o The other object.
         */
        Tls_session(Tls_session<TCred>&& o) :
                session_(o.session_),
                credentials_(std::move(o.credentials_)),
                is_setup_(o.is_setup_),
                medium_ret_(o.medium_ret_)
        {
            o.session_ = nullptr;
        }

        //! Destructor.
        virtual ~Tls_session()
        {
            if (session_)
            {
                gnutls_deinit(session_);
            }
        }

        //! Explicitly deleted copy assignment operator.
        /*!
         \param o The other object.
         \return This object.
         */
        Tls_session<TCred>& operator=(const Tls_session<TCred>& o) = delete;

        //! Move assignment operator.
        /*!
         \param o The other object.
         \return This object.
         */
        Tls_session<TCred>& operator=(Tls_session<TCred>&& o)
        {
            std::swap(session_, o.session_);
            credentials_ = std::move(o.credentials_);
            is_setup_ = o.is_setup_;
            medium_ret_ = o.medium_ret_;
            return *this;
        }

        //! Get the credentials associated with this session.
        /*!
         \return The credentials.
         */
        inline TCred& credentials()
        {
            return credentials_;
        }

        //! Set the user data associated with this object.
        /*!
         \param data The data to associate with the session.
         */
        void set_user_data(void* data)
        {
            gnutls_session_set_ptr(session_, data);
        }

        //! Get the user data associated with this object.
        template<typename UserDataT>
        UserDataT* user_data()
        {
            return dynamic_cast<UserDataT*>(gnutls_session_get_ptr(session_));
        }

        //! Set the DNS name for the session.
        /*!
         \param name The DNS name.
         */
        void set_hostname(const std::string& name)
        {
            gnutls_server_name_set(session_, GNUTLS_NAME_DNS, name.c_str(), name.size());
        }

        //! Set the ciphers allowed for this connection.
        /*!
         \par
         Would be something like NORMAL:+ANON-ECDH:+ANON-DH
         \param priority The gnutls priority string.
         \exception logjam::Tls_exception If gnutls rejects the string.
         */
        void set_cipher_priority(const std::string& priority)
        {
            const char* epos;
            int ret = gnutls_priority_set_direct(session_, priority.c_str(), &epos);

            if (0 > ret)
            {
                std::ostringstream oss;
                oss << "Error setting cipher priority.";
                if (ret == GNUTLS_E_INVALID_REQUEST)
                {
                    oss << " Cipher error at or before \"" << epos << "\"";
                }
                throw logjam::Tls_exception(oss.str(), ret);
            }
        }

        //! Set the number of bits to use for the Diffie-Hellman key exchange.
        /*!
         \param bits The number of bits to use.
         */
        void set_dh_prime_bits(size_t bits)
        {
            gnutls_dh_set_prime_bits(session_, bits);
        }

        //! Attach the socket descriptor to the session.
        /*!
         \par
         Gnutls uses BSD sockets by default. This method is used to provide
         the BSD socket descriptor to the default implementation.
         \param sockfd The socket descriptor.
         */
        void set_socket(int sockfd)
        {
            gnutls_transport_set_ptr(session_, (gnutls_transport_ptr_t)sockfd);
        }

        //! Assuming this connection is setup, perform the TLS handshake.
        /*!
         \exception logjam::Tls_exception is thrown if there is a fatal error
         or alert received during the handshake.
         */
        void handshake()
        {
            // Shove the credentials onto the session object at the last possible
            // moment.
            if (!is_setup_)
            {
                gnutls_credentials_set(session_,
                        credentials_.gnutls_type(),
                        credentials_.gnutls_ptr());
                is_setup_ = true;
            }

            // Handshake until we are successful or fatal.
            int ret;
            do
            {
                ret = gnutls_handshake (session_);
            }
            while (0 > ret && 0 == gnutls_error_is_fatal(ret));

            // Deal with the error cases.
            if (0 > ret)
            {
                std::ostringstream oss;
                oss << "Handshake failed";
                if (GNUTLS_E_FATAL_ALERT_RECEIVED == ret)
                {
                    gnutls_alert_description_t alert_desc =
                            gnutls_alert_get(session_);
                    oss << ": " << gnutls_alert_get_name(alert_desc);
                }
                oss << ".";
                throw logjam::Tls_exception(oss.str(), ret);
            }
        }

        //! Send bytes over the TLS connection.
        /*!
         \par
         See the lj::Streambuf_bsd for more details.
         \param ptr Pointer to the data to write.
         \param len The number of bytes to write.
         \return The number of bytes actually written. Negative return values
         indicate an error.
         \sa lj::Streambuf_bsd These methods support using the session with the
         lj::Streambuf_bsd class.
         \sa lj::medium::Socket::write() For an example of another Streambuf_bsd medium.
         */
        int write(const uint8_t* ptr, size_t len)
        {
            medium_ret_ = gnutls_record_send(session_, ptr, len);
            return medium_ret_;
        }

        //! Receive bytes over the TLS connection.
        /*!
         \par
         See the lj::Streambuf_bsd for more details.
         \param ptr Pointer to the buffer for reading.
         \param len Maximum number of bytes to read.
         \return The number of bytes actually read. Negative return values
         indicate an error.
         \sa lj::Streambuf_bsd These methods support using the session with the
         lj::Streambuf_bsd class.
         \sa lj::medium::Socket::read() For an example of another Streambuf_bsd medium.
         */
        int read(uint8_t* ptr, size_t len)
        {
            medium_ret_ = gnutls_record_recv(session_, ptr, len);
            return medium_ret_;
        }

        //! Convert medium errors into an error string.
        /*!
         \par
         See the lj::Streambuf_bsd for more details.
         \return String representation of the error.
         \sa lj::Streambuf_bsd These methods support using the session with the
         lj::Streambuf_bsd class.
         \sa lj::medium::Socket::error() For an example of another Streambuf_bsd medium.
         */
        std::string error()
        {
            return std::string(gnutls_strerror(medium_ret_));
        }

    private:
        gnutls_session_t session_;
        TCred credentials_;
        bool is_setup_;
        int medium_ret_;
    }; // class logjam::Tls_session
}; // namespace logjam