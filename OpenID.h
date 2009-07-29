#pragma once
#include <string>
#include <map>

namespace openid_1_1 {
    //! Dumb Relay 
    class DumbRelayConsumer {
    public:
        DumbRelayConsumer(const std::string &identifier);
        void identifier(const std::string &identifier);
        const std::string &identifier() const { return _identifier; };
        void openid_provider(const std::string &openid_provider);
        const std::string &openid_provider() const { return _openid_provider; };
        
        virtual std::string checkid_setup(const std::string &return_to,
                                          const std::string &trust_root);
        virtual bool check_authentication(const std::multimap<std::string, std::string> &params);
    protected:
        void discovery();
        std::string contact_openid_provider(const std::string &post_data);
    private:
        std::string _identifier;
        std::string _openid_provider;
    };
    
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
