#pragma once
#include <string>
#include <map>

namespace openid_1_1 {
    //! Dumb Relay Consumer.
    /*!
     \par A dumb relay consumer does not store state. This implementation can be
     used with out any modification or extra implementation.
     \par The DumbRelayConsumer provides a standard interface for the more
     sophisticated consumers.
     \author Jason Watson
     \version 1.1
     \date July 29, 2009
     */
    class DumbRelayConsumer {
    public:
        
        //! Create a new relay consumer for the provider identifier.
        /*!
         \par After the identifier is canonicalized, the
         DumbRelayConsumer::discovery() method is invoked to follow redirects,
         resolve delegation, and get the open id provider information.
         \param identifier The identifier provided by the user.
         \sa discovery()
         */
        DumbRelayConsumer(const std::string &identifier);
        
        //! Set the identifier after construction.
        /*!
         \par This is normally invoked by discovery, to re-canonicalize
         redirects and delegation.
         \param identifier The identifier.
         \sa discovery()
         */
        void identifier(const std::string &identifier);
        
        //! Get the identifier for this consumer.
        /*!
         \par The returned identifier is the current identifier value. During
         discovery, this value will change as a result of redirects and 
         delegation at the identity page.
         \return The current identifier value.
         \sa discovery()
         */
        const std::string &identifier() const { return _identifier; };
        
        //! Set the open id provider URL.
        /*!
         \par This is normally invoked as the last stage of discovery, to
         store the open id server referenced by the identity page.
         \param openid_provider The openid provider URL.
         \sa discovery()
         */
        void openid_provider(const std::string &openid_provider);
        
        //! Get the openid provider URL.
        /*!
         \param This method is used to construct redirect URLs returned by
         checkid_setup, and checkid_immediate, as well as for communication
         by associate and check_authentication.
         \return The openid provider URL.
         */
        const std::string &openid_provider() const { return _openid_provider; };
        
        //! Get the checkid setup redirect URL.
        /*!
         \par method takes the discovered values, and constructs a "login"
         redirect URL. The login redirect URL should be sent to the user browser
         as a redirect. When the authentication is completed, the user will be
         redirected back to the return_url with some additional parameters.
         \par The additional parameters can be verified by using the
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
         \par This method is called after a user returns from the checkid_setup
         step. The query parameters from the openid provider are passed to the
         method and the method verifies they are valid.
         \par Part of the verification process involves contacting the openid
         provider and asking them directly to verify the response. An outbound
         http(s) connection is made for this purpose.
         \par If the user validation checks out, the user is considered to be
         authenticated.
         \param params A map of all the params included in the user request.
         \return true if the authentication is valid, false if the authentication
         is forged or fake.
         */
        virtual bool check_authentication(const std::multimap<std::string, std::string> &params);
    protected:
        
        //! Discover an openid identity.
        /*!
         \par After providing an openid identifier, the associated identity
         page must be fetched. The redirects and the information on the page
         provide information about how to complete the authentication.
         */
        void discovery();
        
        //! Post a request directly to an openid provider.
        /*!
         \par certain openid functions must post data directly to an openid
         provider. This method posts that data directly to the provider and
         returns the response.
         /param post_data The data to send as the post body.
         \return The response body from the openid provider server.
         \sa check_authentication()
         */
        std::string contact_openid_provider(const std::string &post_data);
    private:
        std::string _identifier;
        std::string _openid_provider;
    };
    
    //! Associated Relay Consumer 
    class AssociatedRelayConsumer : public DumbRelayConsumer {
    public:
        struct Association {
            std::string assoc_type, assoc_handle, provider, session_type; 
            std::string dh_server_public, secret;
            long long expires_at;
        };
        AssociatedRelayConsumer(const std::string &identifier);
        virtual std::string checkid_setup(const std::string &return_to,
                                          const std::string &trust_root);
        virtual bool check_authentication(const std::multimap<std::string, std::string> &params);
        virtual void invalidate_assoc_handle(const std::string &assoc_handle) = 0;
    protected:
        virtual std::string *associate();
        virtual std::string *lookup_assoc_handle(const std::string &provider) = 0;
        virtual Association *lookup_association(const std::string &assoc_handle) = 0;
        virtual void store_assoc_handle(const Association *association) = 0;
    private:
    };
};
