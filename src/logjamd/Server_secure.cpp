/*!
 \file logjamd/Server_secure.cpp
 \brief Logjam server networking implementation.
 \author Jason Watson

 Copyright (c) 2011, Jason Watson
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

#include "logjamd/Connection_secure.h"
#include "logjamd/Server_secure.h"
#include "logjam/Network_address_info.h"
#include "lj/Exception.h"
#include "lj/Streambuf_bsd.h"
#include "logjam/Client_socket.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern "C"
{
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
}

namespace
{
    const unsigned int k_dh_bits = 2048;
    const size_t k_buffer_in_size = 8196;
    const size_t k_buffer_out_size = 8196;
    
    std::iostream* connect_to_peer(const std::string& address, const lj::bson::Node& auth)
    {
        // Start by opening a connection to the peer.
        std::iostream* peer = nullptr;
        try
        {
            lj::log::format<lj::Debug>("Attempting to connect to peer %s.")
                    << address
                    << lj::log::end;
            peer = logjam::client::create_connection(
                    address,
                    "peer");
        }
        catch (lj::Exception ex)
        {
            lj::log::format<lj::Critical>("Unable to connect to peer %s: %s")
                    << address
                    << ex
                    << lj::log::end;
        }
        
        // If the peer is a valid object, lets authenticate.
        if (peer)
        {
            lj::bson::Node response;
            (*peer) << auth;
            peer->flush();
            (*peer) >> response;
            
            // Look to make sure the response was successful.
            if (!logjam::client::is_success(response))
            {
                lj::log::format<lj::Error>("Failed to Authenticate to peer %s: %s")
                        << address
                        << logjam::client::message(response)
                        << lj::log::end;
                delete peer;
                peer = nullptr;
            }
        }
        return peer;
    }
    
    class Connect_to_peers_work : public lj::Work
    {
    public:
        Connect_to_peers_work(bool* running, logjamd::Server_secure* server) : running_(running), server_(server)
        {
        }
        virtual ~Connect_to_peers_work()
        {
        }
        virtual void run() override
        {
            while (*running_)
            {
                log<lj::Debug>("Pausing for 1 minute.").end();
                std::this_thread::sleep_for(std::chrono::minutes(1));
                
                log<lj::Info>("Heartbeating %i peers.").end(server_->peers().size());
                lj::bson::Node auth(server_->cfg()["server/identity"]);
                for (auto iter = server_->peers().begin();
                        server_->peers().end() != iter;
                        ++iter)
                {
                    const std::string peer_name = (*iter).first;
                    std::iostream* peer = (*iter).second;
                    
                    try
                    {
                        // Establish a connection if one is not already established.
                        if (peer == nullptr)
                        {
                            log<lj::Debug>("Attempting to connect to %s peer.")
                                    << peer_name
                                    << lj::log::end;
                            peer = connect_to_peer(peer_name,
                                    auth);
                            
                            if (peer)
                            {
                                log<lj::Info>("Connected to %s peer.")
                                        << peer_name
                                        << lj::log::end;
                                (*iter).second = peer;
                            }
                            else
                            {
                                log<lj::Info>("Unable to establish a connection to %s peer.")
                                        << peer_name
                                        << lj::log::end;
                                (*iter).second = nullptr;
                            }
                        }
                        
                        // We have done what we can to establish a new
                        // so now we heartbeat if we can.
                        if (peer != nullptr) {
                            log<lj::Debug>("Attempting to heartbeat to %s peer.")
                                    << peer_name
                                    << lj::log::end;
                            
                            lj::bson::Node request;
                            request.set_child("command",
                                    lj::bson::new_string("heartbeat()"));
                            request.set_child("language",
                                    lj::bson::new_string("lua"));
                            
                            (*peer) << request;
                            
                            lj::bson::Node response;
                            (*peer) >> response;
                            
                            if (!logjam::client::is_success(response))
                            {
                                log<lj::Warning>("Unable to heartbeat to %s peer. Response logged at info level")
                                        << peer_name
                                        << lj::log::end;
                                log<lj::Info>("Response from %s peer: %s")
                                        << peer_name
                                        << lj::bson::as_json_string(response)
                                        << lj::log::end;
                                
                                (*iter).second = nullptr;
                                delete peer->rdbuf();
                                delete peer;
                            }
                        }
                    }
                    catch (const lj::Exception& ljex)
                    {
                        log<lj::Warning>("Unable to heartbeat to %s peer because of %s")
                                << peer_name
                                << ljex
                                << lj::log::end;
                        (*iter).second = nullptr;
                        if (peer != nullptr)
                        {
                            delete peer->rdbuf();
                            delete peer;
                        }
                    }
                    catch (const std::exception ex)
                    {
                        log<lj::Warning>("Unable to heartbeat to %s peer because of %s")
                                << peer_name
                                << ex.what()
                                << lj::log::end;
                        (*iter).second = nullptr;
                        if (peer != nullptr)
                        {
                            delete peer->rdbuf();
                            delete peer;
                        }
                    }
                    catch (...)
                    {
                        log<lj::Warning>("Unable to heartbeat to %s peer for some really weird reason that isn't a known exception type.")
                                << peer_name
                                << lj::log::end;
                        (*iter).second = nullptr;
                        if (peer != nullptr)
                        {
                            delete peer->rdbuf();
                            delete peer;
                        }
                    }
                }
            }
        }
        virtual void cleanup() override
        {
            // Since this work isn't creating a usable result, we can manage our own memory.
            delete this;
        }
                
        template <typename LVL>
        static lj::log::Logger& log(const std::string format) {
            std::string final_format("[Connect_to_peers_work] ");
            final_format.append(format);
            return lj::log::format<LVL>(final_format);
        }
    private:
        bool* running_;
        logjamd::Server_secure* server_;
    };
    
}

namespace logjamd
{
    Server_secure::Server_secure(lj::bson::Node* config) :
            logjamd::Server(config),
            io_(-1),
            running_(false),
            connections_(),
            peers_(),
            peers_thread_(nullptr),
            credentials_(),
            key_exchange_(k_dh_bits)
    {
    }

    Server_secure::~Server_secure()
    {
        shutdown();
        if (-1 < io_)
        {
            ::close(io_);
        }

        lj::log::format<lj::Debug>("Shutting down peers thread.");
        peers_thread_->join();
        delete peers_thread_;
        
        lj::log::format<lj::Debug>("Deleting all connections for server %p")
                << (const void*)this
                << lj::log::end;
        for (auto iter = connections_.begin();
                connections_.end() != iter;
                ++iter)
        {
            delete (*iter);
        }
        
        lj::log::format<lj::Debug>("Deleting all peers for server %p.")
                << (const void*)this
                << lj::log::end;
        for (auto iter = peers_.begin();
                peers_.end() != iter;
                ++iter)
        {
            delete (*iter).second->rdbuf();
            delete (*iter).second;
        }
    }

    void Server_secure::startup()
    {
        // Link the key exchange and the credentials.
        credentials_.configure_key_exchange(key_exchange_);

        // Figure out where we should be listening.
        std::string listen_on(lj::bson::as_string(cfg()["server/listen"]));
        lj::log::format<lj::Info>("Attempting to listen on \"%s\".")
                << listen_on
                << lj::log::end;

        logjam::Network_address_info info(listen_on,
                AI_PASSIVE,
                AF_UNSPEC,
                SOCK_STREAM,
                0);
        if (!info.next())
        {
            // we didn't get any address information back, so abort!
            throw LJ__Exception(info.error());
        }

        // Now create my socket descriptor for listening.
        io_ = ::socket(info.current().ai_family,
                info.current().ai_socktype,
                info.current().ai_protocol);
        if (0 > io_)
        {
            // Did not get a socket descriptor.
            throw LJ__Exception(strerror(errno));
        }

        int rc = ::bind(io_,
                info.current().ai_addr,
                info.current().ai_addrlen);
        if (0 > rc)
        {
            // Did not get a socket descriptor.
            throw LJ__Exception(strerror(errno));
        }

        rc = ::listen(io_, 5);
        if (0 > rc)
        {
            // did not bind the listener to a port.
            throw LJ__Exception(strerror(errno));
        }
        
        // Connect to the peers.
        const lj::bson::Node& cluster = cfg()["server/cluster"];
        for (auto iter = cluster.to_vector().begin();
                cluster.to_vector().end() != iter;
                ++iter)
        {
            const std::string peer_name = lj::bson::as_string(*(*iter));
            peers_.insert(Peer_map::value_type(peer_name, nullptr));
        }
        
        peers_thread_ = new lj::Thread();
        peers_thread_->run(new Connect_to_peers_work(&running_, this));
    }

    void Server_secure::listen()
    {
        running_ = true;
        while(running_)
        {
            // Accept a connection.
            struct sockaddr_storage remote_addr;
            socklen_t remote_addr_size = sizeof(struct sockaddr_storage);
            int client_socket = accept(io_,
                    (struct sockaddr *)&remote_addr,
                    &remote_addr_size);
            if (0 > client_socket)
            {
                // I had problems accepting that client.
                throw LJ__Exception(strerror(errno));
            }
            logjam::Network_connection client_connection(client_socket);

            // Create a buffer and a stream object.
            lj::medium::Socket* insecure_medium =
                    new lj::medium::Socket(client_connection.socket());
            lj::Streambuf_bsd<lj::medium::Socket>* insecure_buffer =
                    new lj::Streambuf_bsd<lj::medium::Socket>(insecure_medium, k_buffer_in_size, k_buffer_out_size);
            std::iostream* insecure_stream = new std::iostream(insecure_buffer);

            // Collect all the admin stuff we need for this connection.
            lj::bson::Node* connection_state = new lj::bson::Node();
            std::string remote_ip = logjam::Network_address_info::as_string(
                    (struct sockaddr*)&remote_addr);
            connection_state->set_child("client/address",
                    lj::bson::new_string(remote_ip));

            lj::log::format<lj::Info>("Accepted a connection form %s.")
                    << remote_ip
                    << lj::log::end;

            // Create the new server logjamd concept of a connection
            Connection_secure* connection = new Connection_secure(
                    this,
                    connection_state,
                    std::move(client_connection),
                    insecure_stream);

            // Kick off the thread for that connection.
            connection->start();

            // store a copy locally for management.
            connections_.push_back(connection);
        }
    }

    void Server_secure::shutdown()
    {
        running_ = false;
    }

    void Server_secure::detach(Connection* conn)
    {
        // remove this pointer from the collection of managed connections.
        Connection_secure* ptr = dynamic_cast<logjamd::Connection_secure*>(
                conn);
        connections_.remove(ptr);
    }

    std::unique_ptr<Server_secure::Session> Server_secure::new_session(int socket_descriptor)
    {
        // TODO convert this to use some form of authentication.
        // see http://www.gnu.org/software/gnutls/manual/gnutls.html#Echo-server-with-anonymous-authentication
        std::unique_ptr<Server_secure::Session> session(
                new Server_secure::Session(Server_secure::Session::k_server));
        session->credentials().set(&credentials_);
        session->set_cipher_priority("NORMAL:+ANON-ECDH:+ANON-DH");
        session->set_dh_prime_bits(key_exchange_.bits());
        session->set_socket(socket_descriptor);
        return session;
    }
    
    Server_secure::Peer_map& Server_secure::peers()
    {
        return peers_;
    }
};
