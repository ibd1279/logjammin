#pragma once
/*
 \file OpenID.h
 \author Jason Watson
 Copyright (c) 2014, Jason Watson
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

#include <string>
#include <map>

namespace openid_1_1 {
    //! Dumb Relay Consumer.
    /*!
     \par
     A dumb relay consumer does not store state. This implementation can be
     used with out any modification or extra implementation.
     \par
     The DumbRelayConsumer provides a standard interface for the more
     sophisticated consumers.
     \author Jason Watson
     \version 1.1
     \date July 29, 2009
     */
    class DumbRelayConsumer {
    public:
        
        //! Create a new relay consumer for the provider identifier.
        /*!
         \par
         After the identifier is canonicalized, the
         DumbRelayConsumer::discovery() method is invoked to follow redirects,
         resolve delegation, and get the open id provider information.
         \param identifier The identifier provided by the user.
         \sa discovery()
         */
        DumbRelayConsumer(const std::string &identifier);
        
        //! Set the identifier after construction.
        /*!
         \par
         This is normally invoked by discovery, to re-canonicalize
         redirects and delegation.
         \param identifier The identifier.
         \sa discovery()
         */
        void identifier(const std::string &identifier);
        
        //! Get the identifier for this consumer.
        /*!
         \par
         The returned identifier is the current identifier value. During
         discovery, this value will change as a result of redirects and 
         delegation at the identity page.
         \return The current identifier value.
         \sa discovery()
         */
        const std::string &identifier() const { return _identifier; };
        
        //! Set the open id provider URL.
        /*!
         \par
         This is normally invoked as the last stage of discovery, to
         store the open id server referenced by the identity page.
         \param openid_provider The openid provider URL.
         \sa discovery()
         */
        void openid_provider(const std::string &openid_provider);
        
        //! Get the openid provider URL.
        /*!
         \par
         This method is used to construct redirect URLs returned by
         checkid_setup, and checkid_immediate, as well as for communication
         by associate and check_authentication.
         \return The openid provider URL.
         */
        const std::string &openid_provider() const { return _openid_provider; };
        
        //! Get the checkid setup redirect URL.
        /*!
         \par
         method takes the discovered values, and constructs a "login"
         redirect URL. The login redirect URL should be sent to the user browser
         as a redirect. When the authentication is completed, the user will be
         redirected back to the return_url with some additional parameters.
         \par
         The additional parameters can be verified by using the
         DumbRelayConsumer::check_authentication() method.
         \param return_to Where the openid provider should send the user after
         authentication.
         \param trust_root What to display to the user for identification of
         the site.
         \return The URL to the openid provider with the query parameters already
         built up.
         \sa check_authentication()
         */
        virtual std::string checkid_setup(const std::string &return_to,
                                          const std::string &trust_root);
        
        //! Check a login request.
        /*!
         \par
         This method is called after a user returns from the checkid_setup
         step. The query parameters from the openid provider are passed to the
         method and the method verifies they are valid.
         \par
         Part of the verification process involves contacting the openid
         provider and asking them directly to verify the response. An outbound
         http(s) connection is made for this purpose.
         \par
         If the user validation checks out, the user is considered to be
         authenticated.
         \param params A map of all the params included in the user request.
         \return true if the authentication is valid, false if the authentication
         is forged or fake.
         \sa checkid_setup()
         */
        virtual bool check_authentication(const std::multimap<std::string, std::string> &params);
    protected:
        
        //! Discover an openid identity.
        /*!
         \par
         After providing an openid identifier, the associated identity
         page must be fetched. The redirects and the information on the page
         provide information about how to complete the authentication.
         */
        void discovery();
        
        //! Post a request directly to an openid provider.
        /*!
         \par
         certain openid functions must post data directly to an openid
         provider. This method posts that data directly to the provider and
         returns the response.
         \param post_data The data to send as the post body.
         \return The response body from the openid provider server.
         \sa check_authentication()
         */
        std::string contact_openid_provider(const std::string &post_data);
    private:
        std::string _identifier;
        std::string _openid_provider;
    };

    //! Association information structure.
    /*!
     \par
     Structure for storing information about Provider consumer association.
     \author Jason Watson
     \version 1.1
     \date July 29, 2009
     */
    struct Association {
        //! Create a new association object.
        Association() : expires_at(0) { };
        
        //! Delete an association.
        virtual ~Association() { };
        
        /*!
         \var std::string assoc_type
         \brief The type of association.
         \par
         The OpenID 1.1 spec says this must be stored, however only
         one value is defined in the spec: SHA-1.
         */
        /*!
         \var std::string assoc_handle
         \brief Opaque handle used to reference the association.
         \par
         The handle is a shared, public ID for the association.
         */
        /*!
         \var std::string provider
         \brief The OpenID provider.
         \par
         The provider this association is connected to.
         */
        /*!
         \var std::string session_type
         \brief The session type.
         \par
         Blank is used for clear text transmitted secrets between the
         provider and the consumer. DH-SHA1 is used when diffie-hellman
         public key encryption is used.
         */
        std::string assoc_type, assoc_handle, provider, session_type;
        
        /*!
         \var std::string dh_server_public
         \brief The diffie-hellman public key.
         \par
         An empty string unless using the DH-SHA1 session type.
         */
        /*!
         \var std::string secret
         \brief The shared secret.
         \par
         The secret shared between the provider and the consumer.
         */
        std::string dh_server_public, secret;
        
        /*!
         \brief When the association exipres.
         \par
         Value is calculated as the TTL of the association plus the
         time the association was made.
         */
        long long expires_at;
    };    
    
    //! Associated Relay Consumer
    /*!
     \par
     A "smart" relay consumer that stores information about different
     providers. This implementation requires a custom implementation because
     state information must be stored.
     \par
     The following methods must be overridden in a specific implementation:
     \link invalidate_assoc_handle() \endlink,
     \link lookup_assoc_handle() \endlink,
     \link lookup_association() \endlink, and
     \link store_assoc_handle() \endlink.
     \author Jason Watson
     \version 1.1
     \date July 29, 2009
     */
    class AssociatedRelayConsumer : public DumbRelayConsumer {
    public:
        //! Create a new relay consumer for the provider identifier.
        /*!
         \par
         After the identifier is canonicalized, the
         \link discovery() \endlink method is invoked to
         follow redirects, resolve delegation, and get the open id provider
         information.
         \param identifier The identifier provided by the user.
         \sa discovery()
         */
        AssociatedRelayConsumer(const std::string &identifier);
        virtual ~AssociatedRelayConsumer();
        virtual std::string checkid_setup(const std::string &return_to,
                                          const std::string &trust_root);
        virtual bool check_authentication(const std::multimap<std::string, std::string> &params);
        
        //! Invalidate a stored association handle.
        /*!
         \par
         Called when the provider has confirmed that a handle is invalid.
         \par
         Associations passed to \link store_assoc_handle() \endlink are allocated
         with the new operator, and should be deallocated with the delete
         operator after being invalidated.
         \param assoc_handle The handle to invalidate.
         \sa lookup_association(), lookup_assoc_handle(), and store_assoc_handle().
         */
        virtual void invalidate_assoc_handle(const std::string &assoc_handle) = 0;
    protected:
        //! Create an association with the current provider.
        /*!
         \par
         A new association is created with the provider found during
         \link discovery() \endlink . The handle for the new association is
         returned.
         \par
         The returned string pointer is allocated with new, and must be
         deallocated with delete.
         \return The new assocation handle.
         */
        virtual std::string *associate();
        
        //! Look up an association handle.
        /*!
         \par
         If an association handle exists for the requested provider, return a pointer
         to a string containing the handle value. If the association does not
         exist, return \p NULL. If the association is expired, return \p NULL.
         \par
         This method is normally invoked during the \link checkid_setup() \endlink
         and the \link check_authentication() \endlink steps.
         \par
         The returned string pointer must be allocated with new.
         \param provider The OpenID provider URL.
         \return Pointer to the opaque assocation handle, or NULL if the
         provider is unknown.
         \sa lookup_associaton(), invalidate_assoc_handle(), and store_assoc_handle().
         */
        virtual const std::string *lookup_assoc_handle(const std::string &provider) = 0;
        
        //! Look up an association.
        /*!
         \par
         If an assocation exists matching the provided \p assoc_handle, return
         the full association object associated with it. If the association
         does not exist, return \p NULL. If the association is expired,
         return \p NULL.
         \par
         This method is normally invoked during the
         \link check_authentication() \endlink step. The association is used to
         validate the signature provided by the user.
         \par
         The returned association pointer must be allocated with new.
         \param assoc_handle The handle for the association.
         \return The association if it is valid, NULL otherwise.
         \sa lookup_assoc_handle(), invalidate_assoc_handle(), and store_assoc_handle().
         */
        virtual Association *lookup_association(const std::string &assoc_handle) = 0;
        
        //! Store an association.
        /*!
         \par
         After an association is made, the association must be stored for
         future reference.
         \par
         This method is normally invoked from the \link associate() \endlink
         method.
         \param association Const pointer to the association to store.
         \sa lookup_association(), lookup_assoc_handle(), and invalidate_assoc_handle().
         */
        virtual void store_assoc_handle(const Association *association) = 0;
    private:
    };
};
