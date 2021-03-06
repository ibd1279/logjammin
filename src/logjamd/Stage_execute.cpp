/*!
 \file Stage_execute.cpp
 \brief Logjam server client command implementation.
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

#include "logjamd/Stage_execute.h"
#include "logjamd/Command_language.h"
#include "logjamd/Response.h"
#include "lua/Command_language_lua.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include "lj/Stopclock.h"
#include <iostream>

namespace
{
    const std::string k_language_js("js");
    const std::string k_language_lua("lua");
};

namespace logjamd
{
    std::unique_ptr<logjam::Stage> Stage_execute::logic(
            logjam::pool::Swimmer& swmr) const
    {
        log("Executing command.").end();
        lj::Stopclock timer;

        lj::bson::Node request;
        swmr.io() >> request;

        // The command language should be swapped out for different langauges.
        // Lua is the only supported language right now.
        std::unique_ptr<Command_language> cmd_lang(
                new lua::Command_language_lua());

        log("Using %s for the command language.").end(cmd_lang->name());

        lj::bson::Node response(response::new_empty(*this));
        response.set_child("output",
                new lj::bson::Node(lj::bson::Type::k_array, NULL));
        bool result = cmd_lang->perform(swmr, request, response);
        response.set_child("elapsed", lj::bson::new_uint64(timer.elapsed()));
        swmr.io() << response;

        // Setup the return object.
        std::unique_ptr<Stage> next_stage(nullptr);
        if (result)
        {
            next_stage = this->clone();
        }

        log("Elapsed %llu ns.").end(timer.elapsed());
        return next_stage;
    }

    std::string Stage_execute::name() const
    {
        return std::string("Execution");
    }

    std::unique_ptr<logjam::Stage> Stage_execute::clone() const
    {
        return std::unique_ptr<logjam::Stage>(new Stage_execute(*this));
    }
};

