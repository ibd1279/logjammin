/*!
 \file logjamd.cpp
 \brief Logjam Server Executable
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

#include "logjamd/logjamd_lua.h"
#include "logjamd/lua/core.h"
#include "logjamd/Server.h"
#include "lj/Logger.h"
#include "build/default/config.h"

#include <cstdlib>
#include <cstring>


int usage()
{
    std::cerr << "LogJam Distributed Storage Server (Version 1.0)" << std::endl << std::endl;
    std::cerr << "Copyright (C) 2010 Jason Watson <jwatson@slashopt.net>" << std::endl;
    std::cerr << "usage: logjamd mode configfile" << std::endl;
    std::cerr << "  mode is either \"config\", \"readonly\", or \"readwrite\"" << std::endl;
    std::cerr << "    config is for creating and modifying the configfile." << std::endl;
    std::cerr << "    readonly prevents storage modification." << std::endl;
    std::cerr << "    readwrite enabled storage modification." << std::endl;
    return 1;
}

void populate_config(lj::Bson* config)
{
    config->set_child("server/port", lj::bson_new_int64(27754));
    config->set_child("server/directory", lj::bson_new_string(DBDIR));
    config->set_child("server/id", lj::bson_new_int64(1));
    config->set_child("replication/enabled", lj::bson_new_boolean(false));
    config->set_child("logging/debug", lj::bson_new_boolean(false));
    config->set_child("logging/info", lj::bson_new_boolean(true));
    config->set_child("logging/notice", lj::bson_new_boolean(true));
    config->set_child("logging/warning", lj::bson_new_boolean(true));
    config->set_child("logging/error", lj::bson_new_boolean(true));
    config->set_child("logging/critical", lj::bson_new_boolean(true));
    config->set_child("logging/alert", lj::bson_new_boolean(true));
    config->set_child("logging/emergency", lj::bson_new_boolean(true));
}

logjamd::Mutable_mode string_to_mutable_mode(const std::string& m)
{
    if (m.compare("config") == 0)
    {
        return logjamd::Mutable_mode::k_config;
    }
    if (m.compare("readonly") == 0)
    {
        return logjamd::Mutable_mode::k_readonly;
    }
    if (m.compare("readwrite") == 0)
    {
        return logjamd::Mutable_mode::k_readwrite;
    }

    return logjamd::Mutable_mode::k_readonly;
}

//! Server main entry point.
int main(int argc, char* const argv[]) {

    // Check that we have two arguments.
    if (argc != 3)
    {
        return usage();
    }

    // Load the configuration from disk
    lj::Bson* mutable_config;
    logjamd::Mutable_mode server_mutable_mode = string_to_mutable_mode(argv[1]);
    std::cerr << "starting server in "
              << static_cast<int64_t>(server_mutable_mode)
              << " mode."
              << std::endl;
    try
    {
        mutable_config = lj::bson_load(argv[2]);
        if (!mutable_config->exists() &&
            logjamd::Mutable_mode::k_config == server_mutable_mode)
        {
            // Create a new default configuration file and let the server
            // go ahead and start.
            std::cerr << "creating default configuration in ["
                      << argv[2]
                      << "]"
                      << std::endl;
            mutable_config = new lj::Bson();
            populate_config(mutable_config);
            lj::bson_save(*mutable_config, argv[2]);
        }

        // force the configfile setting to reference the one loaded.
        mutable_config->set_child("server/configfile", lj::bson_new_string(argv[2]));
    }
    catch (lj::Exception* e)
    {
        // abort and shutdown.
        std::cerr << "unable to load configuration from ["
                  << argv[2]
                  << "]"
                  << std::endl
                  << e->to_string()
                  << std::endl;
        delete e;
        return 2;
    }

    // Get the values we need to startup from the configuration.
    try
    {
        // We hade the real config in a const Bson* to error out when
        // paths are not found in the configuration object. accidently
        // creating a port value of 0 would be less than useful.
        const lj::Bson* config = const_cast<const lj::Bson*>(mutable_config);
        int port = lj::bson_as_int32(config->nav("server/port"));

        // set the logging levels.
        logjamd::set_logging_levels(*config);

        // Create the right dispatchers based on the mode.
        mutable_config->set_child("server/mode",
                                  lj::bson_new_int64(static_cast<int64_t>(server_mutable_mode)));
        lj::Socket_dispatch* dispatcher = new logjamd::Server(mutable_config);
        
        // Bind the sockets, and loop for connections.
        lj::Socket_selector::instance().bind_port(port, dispatcher);
        lj::Socket_selector::instance().loop();
    }
    catch(lj::Exception* e)
    {
        std::cerr << e->to_string() << std::endl;
    }

    return 0;
}
