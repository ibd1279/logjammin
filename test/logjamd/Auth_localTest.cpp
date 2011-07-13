/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "logjamd/Auth.h"
#include "logjamd/Auth_local.h"
#include "logjamd/constants.h"

using namespace logjamd;

namespace
{
    const lj::Uuid k_auth_method_password_hash(k_auth_method, "password_hash", 13);
    const lj::Uuid k_auth_provider_local(k_auth_provider, "local", 5);

    struct creds
    {
        creds(bool alt_password)
        {
            n.set_child("login",
                    lj::bson::new_string("admin"));
            if (alt_password)
            {
                n.set_child("password",
                        lj::bson::new_string("abc123"));
            }
            else
            {
                n.set_child("password",
                        lj::bson::new_string("1!aA2@bB"));
            }
        }

        lj::bson::Node n;
    };
};

void testAuth_registry_enable()
{
    Auth_registry::enable(new Auth_provider_local());

    Auth_provider* provider = 
            Auth_registry::provider(k_auth_provider_local);
    TEST_ASSERT(provider != NULL);

    provider = Auth_registry::provider(k_auth_method_password_hash);
    TEST_ASSERT(provider == NULL);
}
void testAuth_provider_method()
{
    Auth_provider_local local;
    Auth_method* method = local.method(k_auth_method_password_hash);
    TEST_ASSERT(method != NULL);

    method = local.method(k_auth_provider_local);
    TEST_ASSERT(method == NULL);
}
void testAuth_method_authenticate()
{
    Auth_method_password_hash method;
    User user(lj::Uuid(12), "admin");
    creds first(false);
    method.change_credentials(&user, &user, first.n);

    User* result = method.authenticate(first.n);
    TEST_ASSERT(result != NULL);
    TEST_ASSERT(result->id() == user.id());

    creds second(true);
    result = method.authenticate(second.n);
    TEST_ASSERT(result == NULL);
}
void testAuth_method_change_creds()
{
    Auth_method_password_hash method;
    User user(lj::Uuid(12), "admin");
    creds first(false);
    method.change_credentials(&user, &user, first.n);
    creds second(true);
    method.change_credentials(&user, &user, second.n);

    User* result = method.authenticate(first.n);
    TEST_ASSERT(result == NULL);

    result = method.authenticate(second.n);
    TEST_ASSERT(result != NULL);
    TEST_ASSERT(result->id() == user.id());
}
int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testAuth_registry_enable),
        PREPARE_TEST(testAuth_provider_method),
        PREPARE_TEST(testAuth_method_authenticate),
        PREPARE_TEST(testAuth_method_change_creds),
        {0, ""}
    };
    return Test_util::runner("logjamd::Auth_local", tests);
}

