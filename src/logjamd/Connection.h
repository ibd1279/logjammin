#pragma once
/*!
 \file logjamd/Connection.h
 \brief Logjam server connection to a client definition.
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

#include "lj/Bson.h"
#include "lj/Log.h"
#include "logjamd/Server.h"
#include "logjamd/User.h"

#include <istream>

namespace logjamd
{
    class User;
    class Stage_auth;

    //! Connection Base Class
    class Connection {
    public:
        friend class Stage_auth;
        friend class Stage_pre;

        //! Constructor
        /*!
         \par
         The user is initially NULL and can only be modified by
         the connection object and the authorization stage.
         \param server The server associated with this connection.
         \param state Bson node for tracking the connection state.
         \param stream The stream used for communication.
         */
        Connection(logjamd::Server* server,
                lj::bson::Node* state,
                std::iostream* stream) :
                server_(server),
                state_(state),
                stream_(stream),
                user_(NULL)
        {
        }

        //! Destructor.
        virtual ~Connection()
        {
            if (state_)
            {
                delete state_;
            }

            if (stream_)
            {
                stream_->flush();
                delete stream_;
            }

            if (user_)
            {
                delete user_;
            }
        }

        //! Perform the connection logic.
        /*!
         \par
         The logic for the connection object is unique to each connection
         type. When start exits, the connection object will be deleted.
         */
        virtual void start() = 0;

        //! Get the server object.
        /*!
         \return The server object.
         */
        virtual logjamd::Server& server()
        {
            return *server_;
        }

        //! Get the connection state.
        /*!
         \return The connection state.
         */
        virtual lj::bson::Node& state()
        {
            return *state_;
        }

        //! Get the io stream associated with this connection.
        /*!
         \return The connection stream.
         */
        virtual std::iostream& io()
        {
            return *stream_;
        }

        //! Get the user associated with this connection.
        /*!
         \todo This should handle the un-authenticated case better. By
         possibly returning null, most methods will need to test the result
         before they can perform any logic. If a reference was returned and
         the un-authenticated case had an "unauth user object" that returned
         false for all ACL checks, that would simplify the calling code.
         \return NULL for unauthenticated connections, a user otherwise.
         */
        virtual const User* user()
        {
            return user_;
        }

        //! Test if this is considered a secure connection.
        /*!
         \return True if the connection is secure, false otherwise.
         */
        virtual bool secure()
        {
            return false;
        }

        //! Store an encryption key with the connection.
        /*!
         \par
         Each connection type must have a mechanism for securely storing the
         encryption keys for a connection instance. Implementations are
         expected to copy the provided key data to memory they will manage.
         Management of that memory includes erasing/overwriting the value
         and releasing the memory.
         \par
         The caller is responsible managing the security of the void pointer
         provided to the method. That includes securely handling the memory
         after the key is stored .
         \param identifier The identifier to associate with the key
         \param key The pointer to the key data.
         \param sz The size of the key.
         */
        virtual void set_crypto_key(const std::string& identifier,
                const void* key,
                int sz) = 0;

        //! Retreive an encryption key from the connection.
        /*!
        \par
        Each connection type must have a mechanism for securly storing the
        encryption keys for a connection instance. Implementations are
        expected to erase/overwrite the value of that data when the connection
        is destroyed.
        \par
        The caller is not responsible for the pointer returned by this method.
        \param identifier The identifier associated with the key
        \param sz Where to store the size of the key.
        \return Pointer to the key data. Null if the identifier is unknown.
        */
        virtual const void* get_crypto_key(const std::string& identifier,
                int* sz) = 0;
    protected:
        //! Set the user for this connection.
        /*!
         \par
         This is hidden from the public scope. Only friends are meant to call
         this method. Stage_auth is the only expected caller.
         \param u The new user pointer.
         */
        void user(logjamd::User* u)
        {
            user_ = u;
        }

    private:
        logjamd::Server* server_; //!< Server Pointer.
        lj::bson::Node* state_;   //!< Connection State Pointer.
        std::iostream* stream_;   //!< Stream Pointer.
        logjamd::User* user_;     //!< User Pointer.
    };

    //! Use a different stream with an existing Connection.
    /*!
     \note User
     This xlator does not copy the user over.
     */
    class Connection_xlator : public Connection
    {
    public:
        //! Constructor
        Connection_xlator(Connection* connection,
                std::iostream* stream) :
                Connection(NULL, NULL, stream),
                real_connection_(connection)
        {
        }

        //! Destructor
        virtual ~Connection_xlator()
        {
        }

        //! Call start on the real connection.
        virtual void start()
        {
            real_connection_->start();
        }

        virtual logjamd::Server& server()
        {
            return real_connection_->server();
        }

        virtual lj::bson::Node& state()
        {
            return real_connection_->state();
        }

        virtual bool secure()
        {
            return real_connection_->secure();
        }

        virtual void set_crypto_key(const std::string& identifier,
                const void* key,
                int sz)
        {
            real_connection_->set_crypto_key(identifier, key, sz);
        }

        virtual const void* get_crypto_key(const std::string& identifier,
                int* sz)
        {
            return real_connection_->get_crypto_key(identifier, sz);
        }

        virtual Connection& real_connection()
        {
            return *real_connection_;
        }
    private:
        Connection* real_connection_;
    };
};


