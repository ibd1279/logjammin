#pragma once
#include "OpenID.h"

class LogJamminConsumer : public openid_1_1::AssociatedRelayConsumer {
public:
    LogJamminConsumer(const std::string &identifier);
    virtual void invalidate_assoc_handle(const std::string &assoc_handle);
    virtual const std::string *lookup_assoc_handle(const std::string &provider);
    virtual openid_1_1::Association *lookup_association(const std::string &assoc_handle);
    virtual void store_assoc_handle(const openid_1_1::Association *association);
};
