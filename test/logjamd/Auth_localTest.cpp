/*
 * File:   Auth_localTest.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "logjamd/Auth_local.h"
#include "logjamd/constants.h"

#include "test/logjamd/Auth_localTest_driver.h"

using namespace logjamd;

namespace
{
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

void testAuth_repository_enable()
{
    logjam::Authentication_repository ar;
    ar.enable(new logjam::Authentication_provider_simple<Auth_method_password_hash>(
            logjamd::k_auth_provider_local));

    try
    {
        ar.provider(logjamd::k_auth_provider_local);
    }
    catch (...)
    {
        TEST_FAILED("Provider should have returned. Got an exception instead.");
    }

    try
    {
        ar.provider(logjamd::k_auth_method_password);
        TEST_FAILED("Repository.provider was expecting an exception, fell through to the next line. :(");
    }
    catch (const logjam::Authentication_provider_not_found_exception& apnfe)
    {
        // Do nothing, as this is the expected result case.
    }
    catch (const Test_failure& tf)
    {
        throw;
    }
    catch (...)
    {
        TEST_FAILED("Repository.provider expecting a provider not found exception, got something different instead.");
    }
}
void testAuthentication_provider_method()
{
    std::unique_ptr<logjam::Authentication_provider> provider(
            new logjam::Authentication_provider_simple<Auth_method_password_hash>(
                    logjamd::k_auth_provider_local));

    try
    {
        provider->method("bcrypt");
    }
    catch (...)
    {
        TEST_FAILED("Method should have returned. Got an exception instead.");
    }

    try
    {
        provider->method("unknown");
        TEST_FAILED("Provider.method was expecting an exception, fell through to next line. :(");
    }
    catch (const logjam::Authentication_method_not_found_exception& amnfe)
    {
        // Expected result;
    }
    catch (const Test_failure& tf)
    {
        throw;
    }
    catch (...)
    {
        TEST_FAILED("Provider.method was expecting a method not found exception, got something different instead.");
    }
}
void testAuth_method_authenticate()
{
    Auth_method_password_hash method;
    lj::Uuid id(12034);
    creds first(false);
    method.change_credential(id, first.n);

    try
    {
        lj::Uuid result(method.authenticate(first.n));
        TEST_ASSERT(result == id);
    }
    catch (Test_failure& tf)
    {
        throw;
    }
    catch (...)
    {
        TEST_FAILED("Method.authenticate was expecting to return a uuid. Threw an exception instead.");
    }

    try
    {
        creds second(true);
        method.authenticate(second.n);
        TEST_FAILED("Method.authenticate was expecting to return an exception. Fell through to next line instead.");
    }
    catch (const Test_failure& tf)
    {
        throw;
    }
    catch (const logjam::User_not_found_exception unfe)
    {
        // Expected result.
    }
    catch (...)
    {
        TEST_FAILED("Method.authenticate was expecting a User not found exception. Something else thrown instead.");
    }
}
void testAuth_method_change_creds()
{
    Auth_method_password_hash method;
    lj::Uuid id(12034);
    creds first(false);
    creds second(true);
    method.change_credential(id, first.n);

    lj::Uuid precheck(method.authenticate(first.n));
    TEST_ASSERT(precheck == id);

    method.change_credential(id, second.n);
    try
    {
        lj::Uuid result(method.authenticate(first.n));
        TEST_FAILED("Method.authenticate was expecting to return an exception. Fell through to next line instead.");
    }
    catch (const Test_failure& tf)
    {
        throw;
    }
    catch (const logjam::User_not_found_exception unfe)
    {
        // Expected result.
    }
    catch (...)
    {
        TEST_FAILED("Method.authenticate was expecting a User not found exception. Something else thrown instead.");
    }

    try
    {
        lj::Uuid result(method.authenticate(second.n));
        TEST_ASSERT(result == id);
    }
    catch (Test_failure& tf)
    {
        throw;
    }
    catch (...)
    {
        TEST_FAILED("Method.authenticate was expecting to return a uuid. Threw an exception instead.");
    }

}
int main(int argc, char** argv)
{
    return Test_util::runner("logjamd::Auth_local", tests);
}

