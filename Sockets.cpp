/*!
 \file Sockets.cpp
 \brief LJ networking implementation.
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

#include <sstream>
#include <cerrno>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <list>
#include "Exception.h"
#include "Logger.h"
#include "Sockets.h"

namespace
{
    void *get_in_addr(struct sockaddr *sa)
    {
        if (sa->sa_family == AF_INET)
        {
            return &(((struct sockaddr_in*)sa)->sin_addr);
        }
        
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
    
    struct addrinfo* get_address_info(const char* ip, int port)
    {
        struct addrinfo hints; 
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        struct addrinfo *info;
        int status;
        std::ostringstream port_str;
        port_str << port;
        if ((status = ::getaddrinfo(ip, port_str.str().c_str(), &hints, &info)))
        {
            throw new lj::Exception("Unable to get address info",
                                gai_strerror(status));
        }
        return info;
    }
};

namespace lj
{
    Socket_selector::Socket_selector() : ud_()
    {
    }
    
    Socket_selector::~Socket_selector()
    {
    }
    
    void Socket_selector::bind_port(int port, Socket_dispatch* dispatch)
    {
        struct addrinfo *info = get_address_info(NULL, port);
        int sock;
        struct addrinfo* iter;
        for (iter = info; iter; iter = iter->ai_next)
        {
            if (-1 == (sock = ::socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)))
            {
                sock = NULL;
                Log::warning.log("Unable to open socket: [%d][%s].") << errno << strerror(errno) << Log::end;
                continue;
            }
            
            int opt_on = 1;
            if (-1 == ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_on, sizeof(int)))
            {
                throw new Exception("Unable to set options on socket",
                                        strerror(errno));
            }
            
            if (-1 == ::bind(sock, iter->ai_addr, iter->ai_addrlen))
            {
                ::close(sock);
                sock = NULL;
                Log::emergency.log("Unable to bind: [%d][%s].") << errno << strerror(errno) << Log::end;
                continue;
            }
            
            break;
        }
        
        if (!iter)
        {
            throw new Exception("Unable to bind to any port.",
                                    "");
        }
        
        freeaddrinfo(info);
        
        if (-1 == ::listen(sock, 10))
        {
            throw new Exception("Unable to listen",
                                    strerror(errno));
        }
        
        dispatch->set_socket(sock);
        dispatch->set_mode(Socket_dispatch::k_listen);
        ud_.insert(std::pair<int, Socket_dispatch*>(sock, dispatch));
    }
    
    void Socket_selector::connect(const std::string& ip, int port, Socket_dispatch* dispatch)
    {
        struct addrinfo *info = get_address_info(ip.c_str(), port);
        
        int sock;
        struct addrinfo* iter;
        for (iter = info; iter; iter = iter->ai_next)
        {
            if (-1 == (sock = ::socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)))
            {
                sock = NULL;
                Log::warning.log("Unable to open socket: [%d][%s].") << errno << strerror(errno) << Log::end;
                continue;
            }
            
            if (-1 == ::connect(sock, iter->ai_addr, iter->ai_addrlen))
            {
                ::close(sock);
                sock = NULL;
                Log::emergency.log("Unable to bind: [%d][%s].") << errno << strerror(errno) << Log::end;
                continue;
            }
            
            break;
        }
        
        if (!iter)
        {
            throw new Exception("Unable to connect.",
                                "");
        }
        
        freeaddrinfo(info);
        
        dispatch->set_socket(sock);
        dispatch->set_mode(Socket_dispatch::k_communicate);
        ud_.insert(std::pair<int, Socket_dispatch*>(sock, dispatch));        
    }
    
    int Socket_selector::populate_sets(fd_set* rs, fd_set* ws)
    {
        FD_ZERO(rs);
        FD_ZERO(ws);
        
        int mx = 0;
        // Populate the sets correctly.
        for (std::map<int, Socket_dispatch*>::iterator iter = ud_.begin();
             ud_.end() != iter;
             ++iter)
        {
            if(iter->second->is_writing())
            {
                FD_SET(iter->first, ws);
            }
            else
            {
                FD_SET(iter->first, rs);
            }
            
            if (iter->first > mx)
            {
                mx = iter->first;
            }
        }
        return mx;
    }
    
    void Socket_selector::select(struct timeval* timeout)
    {
        fd_set rs;
        fd_set ws;
        
        int mx = populate_sets(&rs, &ws);
        
        if (-1 == ::select(mx + 1, &rs, &ws, NULL, timeout))
        {
            throw new Exception("select",
                                    strerror(errno));
        }
        
        std::list<Socket_dispatch*> add;
        std::list<int> remove;
        for (std::map<int, Socket_dispatch*>::iterator iter = ud_.begin();
             ud_.end() != iter;
             ++iter)
        {
            if (FD_ISSET(iter->first, &rs))
            {
                if (Socket_dispatch::k_listen == iter->second->mode())
                {
                    struct sockaddr_storage ra;
                    socklen_t ral = sizeof(struct sockaddr_storage);
                    int remote_sock = accept(iter->first,
                                             (struct sockaddr *)&ra,
                                             &ral);
                    if (-1 == remote_sock)
                    {
                        Log::warning.log("Unable to accept: [%d][%s].") << errno << strerror(errno) << Log::end;
                    }
                    else
                    {
                        char buff[INET6_ADDRSTRLEN];
                        inet_ntop(ra.ss_family,
                                  get_in_addr((struct sockaddr*)&ra),
                                  buff,
                                  INET6_ADDRSTRLEN);
                        Socket_dispatch* dispatch = iter->second->accept(remote_sock, buff);
                        add.push_back(dispatch);
                    }
                }
                else
                {
                    char buff[1024];
                    int nbytes = recv(iter->first, buff, 1024, 0);
                    if (0 < nbytes)
                    {
                        Log::info.log("Reading %d.") << nbytes << Log::end;
                        iter->second->read(buff, nbytes);
                    }
                    else
                    {
                        if (0 == nbytes)
                        {
                            Log::info.log("Broken connection.") << Log::end;
                        }
                        else
                        {
                            Log::warning.log("Unable to read: [%d][%s].") << errno << strerror(errno) << Log::end;
                        }
                        remove.push_back(iter->first);
                        iter->second->close();
                        delete iter->second;
                    }
                }
            }
            else if (FD_ISSET(iter->first, &ws))
            {
                int sz;
                const char* buff = iter->second->write(&sz);
                int sz2 = send(iter->first, buff, sz, 0);
                if (-1 == sz2)
                {
                    Log::warning.log("Unable to write: [%d][%s].") << errno << strerror(errno) << Log::end;
                }
                else
                {
                    iter->second->written(sz2);
                }
            }
        }
        for (std::list<int>::iterator r_i = remove.begin();
             remove.end() != r_i;
             ++r_i)
        {
            ud_.erase(*r_i);
        }
        
        for (std::list<Socket_dispatch*>::iterator a_i = add.begin();
             add.end() != a_i;
             ++a_i)
        {
            ud_.insert(std::pair<int, Socket_dispatch*>((*a_i)->socket(), (*a_i)));
        }
    }
    
    void Socket_selector::loop()
    {
        while (true)
        {
            this->select(0);
        }
    }
    
    Socket_dispatch::Socket_dispatch() : is_w_(false), s_(0), m_(k_communicate), out_(0), out_offset_(0), out_sz_(0)
    {
    }
    
    Socket_dispatch::~Socket_dispatch()
    {
        if (out_)
        {
            delete[] out_;
        }
    }
    
    
    const char* Socket_dispatch::write(int* sz)
    {
        *sz = out_sz_ - out_offset_;
        return out_ + out_offset_;
    }
    
    void Socket_dispatch::written(int sz)
    {
        out_offset_ += sz;
        if (out_offset_ == out_sz_)
        {
            delete[] out_;
            out_sz_ = 0;
            out_offset_ = 0;
            out_ = 0;
            is_w_ = false;
        }
    }
    
    void Socket_dispatch::close()
    {
        ::close(s_);
    }
    
    void Socket_dispatch::add_bytes(const char* buffer, int sz)
    {
        char* ptr = new char[sz + out_sz_];
        if (out_)
        {
            memcpy(ptr, out_, out_sz_);
            delete[] out_;
        }
        memcpy(ptr + out_sz_, buffer, sz);
        out_ = ptr;
        out_sz_ += sz;
        is_w_ = true;
    }
}; // namespace lj