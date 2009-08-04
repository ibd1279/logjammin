#pragma once
/*
 \file RssItem.h
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

#include <string>
#include "lunar.h"

//! Rss Item class.
/*!
 \author Jason Watson
 \version 1.0
 \date July 30, 2009
 */
class RssItem {
    std::string _title, _link, _guid, _description, _author, _date;
public:
    //! Lua bindings classname.
    static const char LUNAR_CLASS_NAME[];
    
    //! Lua bindings method array.
    static Lunar<RssItem>::RegType LUNAR_METHODS[];
    
    //! Create a Rss Item object.
    RssItem() { };
    
    //! Lua constructor
    /*!
     \param L The lua state.
     */
    RssItem(lua_State *L) { };
    
    //! Get a copy of the title associated with this item
    /*!
     \return A copy of the title.
     */
    std::string title() const { return _title; };
    
    //! Set the title associated with this item.
    /*!
     \param s The item title.
     */
    void title(const std::string &s) { _title = s; };
    
    //! Get a copy of the link associated with this item.
    /*!
     \return A copy of the link.
     */
    std::string link() const { return _link.size() > 0 ? _link : _guid; };
    
    //! Set the link associated with this item.
    /*!
     \param s The item link.
     */
    void link(const std::string &s) { _link = s; };
    
    //! Set the guid associated with this item.
    /*!
     \param s The item guid.
     */
    void guid(const std::string &s) { _guid = s; };
    
    //! Get a copy of the description associated with this item.
    /*!
     \return A copy of the description.
     */
    std::string description() const { return _description; };
    
    //! Set the description associated with this item.
    /*!
     \param s The item description.
     */
    void description(const std::string &s) { _description = s; };
    
    //! Get a copy of the author associated with this item.
    /*!
     \return A copy of the author.
     */
    std::string author() const { return _author; };
    
    //! Set the author associated with this item.
    /*!
     \param s The item author.
     */
    void author(const std::string &s) { _author = s; };
    
    //! Get a copy of the date associated with this item.
    /*!
     \return A copy of the date.
     */
    std::string date() const { return _date; };
    
    //! Set the date associated with this item.
    /*!
     \param s The item date.
     */
    void date(const std::string &s) { _date = s; };
    
    //! Get the date as a unix timestamp.
    /*!
     \return A unix timestamp.
     */
    long long date_ts() const;
};
