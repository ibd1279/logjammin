/*
 \file logjam.cpp
 \author Jason Watson

 Copyright (c) 2009, Jason Watson
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


#include "logjam/Tls_globals.h"
#include "lj/Bson.h"
#include "lj/Log.h"

#include "gnutls/gnutlsxx.h"

int main(int argc, char * const argv[])
{
    try
    {
        logjam::Tls_globals globals; // this ensures that the deinit is called when the stack unwinds.
    /*
        // http://www.gnu.org/software/gnutls/manual/gnutls.html#Simple-client-example-with-X_002e509-certificate-support
        gnutls::certificate_client_credentials client_cert_creds;
        client_cert_creds.set_x509_trust_file("/tmp/foo.crt", GNUTLS_X509_FMT_PEM);
        // no c++ way to set the verify function like the example.
        client_cert_creds.set_x509_key_file("/tmp/cert.pem", "/tmp/key.pem", GNUTLS_X509_FMT_PEM);
        gnutls::session client_session(GNUTLS_CLIENT);
        client_session.set_user_ptr("my_host_name");
        // no c++ way to set the server name.
        const char* err;
        client_session.set_priority("NORMAL", &err);
        client_session.set_credentials(client_cert_creds);

        // create the socket descriptor.
        //client_session.set_transport_ptr(socket);

        do
        {
            int ret = client_session.handshake();
        }
        while (false); // not sure this translates into what the C++ wrapper does.. :(
    */
    }
    catch (const lj::Exception& ex)
    {
        lj::log::out<lj::Error>(ex.str());
    }
}