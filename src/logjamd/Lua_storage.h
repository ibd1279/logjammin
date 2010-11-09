#pragma once
/*!
 \file Lua_storage.h
 \brief Logjamd lj::Storage wrapper header.
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

#include "lj/lunar.h"
#include "lj/Storage.h"

namespace logjamd
{
    //! Lua Storage wrapper.
    /*!
     \par
     Known as "Storage" in lua.
     \author Jason Watson
     \version 1.0
     \date April 27, 2010
     */
    class Lua_storage {
    public:
        //! Class name in lua.
        static const char LUNAR_CLASS_NAME[];
        
        //! Table of methods for lua.
        static Lunar<Lua_storage>::RegType LUNAR_METHODS[];
        
        //! Create a new Lua_storage wrapper for lj::Storage.
        /*!
         \param dbname The name of the DB to open.
         */
        Lua_storage(const std::string& dbname);
        
        //! Create a new Lua_storage wrapper for lj::Storage.
        /*!
         \par
         Expects a String to be ontop of the lua state.
         \param L The lua state.
         */
        Lua_storage(lua_State* L);
        
        //! Destructor.
        ~Lua_storage();
        
        //! Get a Record_set containing all records.
        /*!
         \param L The lua state.
         \return 1
         */
        int all(lua_State* L);
        
        //! Get a Record_set containing no records.
        /*!
         \param L The lua state.
         \return 1
         */
        int none(lua_State* L);
        
        //! Get a Record_set containing one record.
        /*!
         \param L The lua state.
         \return 1.
         */
        int at(lua_State* L);
        
        //! Place a new record into the database.
        /*!
         \param L The lua state.
         \return 0
         */
        int place(lua_State* L);
        
        //! Remove a record from the database
        /*!
         \param L The lua state.
         \return 0
         */
        int remove(lua_State* L);
        
        //! Checkpoint the database.
        /*!
         \par
         Clears out the journal, and creates a backup copy of the database.
         \param L The lua state.
         \return 0
         */
        int checkpoint(lua_State* L);
        
        //! Add an index to a running Storage object.
        /*!
         \code
         db.role = Storage:new('role')
         db.role:add_index('tree', 'some/field', 'lex')
         \endcode
         \param L The lua state.
         \return 0
         */
        int add_index(lua_State* L);
        
        //! Remove an index from a running storage object.
        /*!
         \code
         db.role = Storage:new('role')
         db.role:remove_index('tree', 'some/field')
         \endcode
         \param L The lua state.
         \return 0
         \sa add_index(lua_State*)
         */
        int remove_index(lua_State* L);
        
        //! Rebuild all the indices for the storage.
        int rebuild(lua_State* L);
        
        //! Optimize the database and indices.
        int optimize(lua_State* L);
        
        //! Close the pooled DB and re-open.  Primary used for debugging.
        int recall(lua_State* L);
        
        //! Get the real storage object.
        /*!
         \param L The Lua state to get the server configuration from.
         \return A reference to the storage object.
         */
        lj::Storage& real_storage(lua_State* L);
        
        //! Get the real storage object.
        /*!
         \param server_config The configuration required for production.
         \return A reference to the storage object.
         */
        lj::Storage& real_storage(const lj::Bson& server_config);
    private:
        //! Hidden.
        Lua_storage(const Lua_storage&);
        
        //! Hidden.
        Lua_storage& operator=(const Lua_storage&);
        
        std::string dbname_;
    };
}; // namespace logjamd
