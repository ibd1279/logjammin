#pragma once
/*!
 \file Sockets.h
 \brief LJ networking header
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

#include <map>

namespace lj
{
    class Socket_dispatch;
    
    //! Socket read/write selector class.
    /*!
     \par
     Wraps the functionality for binding a socket and reading/writing
     from sockets.
     \author Jason Watson
     \version 1.0
     \date May 12, 2010
     */
    class Socket_selector {
    public:
        //! Create a new socket selector.
        Socket_selector();
        
        //! destroy a socket selector.
        ~Socket_selector();
        
        //! Bind a port for accepting connections.
        /*!
         \param port The port to listen on.
         \param dispatch The object that will handle dispatching events.
         */
        void bind_port(int port, Socket_dispatch* dispatch);
        
        //! Connect to a remote host.
        /*!
         \param ip The remote IP address.
         \param port The port.
         \param dispatch The object that will handle dispatching events.
         */
        void connect(const std::string& ip, int port, Socket_dispatch* dispatch);
        
        //! Perform a select operaation on all open sockets.
        /*!
         \param timeout How long to poll.
         */
        void select(struct timeval* timeout);
        
        //! Perform a select operation on all open sockets.
        void loop();
        
    private:
        //! Hidden copy constructor.
        /*!
         \param o Other.
         */
        Socket_selector(const Socket_selector& o);
        
        //! Hidden assignment operator.
        /*!
         \param o Other.
         \return The object.
         */
        Socket_selector& operator=(const Socket_selector& o);
        
        //! Populate the selector sets.
        /*!
         \param r The read socket set to populate.
         \param w The write socket set to populate.
         \return The max socket id.
         */
        int populate_sets(fd_set* r, fd_set* w);
        
        //! map for relating user data to socket handles.
        std::map<int, Socket_dispatch*> ud_;
    };
    
    //! Socket Dispatcher base class.
    /*!
     \par
     Implementations of this interface are used to handle events from the
     Socket_selector.
     \author Jason Watson
     \version 1.0
     \date May 12, 2010
     */
    class Socket_dispatch {
    public:
        //! What mode is this dispatcher expected to function in.
        enum Socket_mode
        {
            k_listen,           //!< Listen for incoming connections.
            k_communicate       //!< Communicate acorss the connection.
        };
        
        //! destructor.
        virtual ~Socket_dispatch()
        {
        }
        
        //! Set the socket for this dispatcher.
        /*!
         \param s The socket.
         */
        virtual void set_socket(int s) = 0;
        
        //! Get the socket for this dispatcher.
        /*!
         \return The socket.
         */
        virtual int socket() = 0;
        
        //! Set the Socket_mode for this dispatcher.
        /*!
         \param m The mode.
         */
        virtual void set_mode(Socket_mode m) = 0;
        
        //! Get the Socket_mode for this dispatcher.
        /*!
         \return The mode.
         */
        virtual Socket_mode mode() = 0;
        
        //! Get if this dispatcher wants to read or write.
        /*!
         \return True for writing, false for reading.
         */
        virtual bool is_writing() = 0;
        
        //! Create a new dispatcher for an accepted socket.
        /*!
         \param socket The socket that was accepted.
         \param ip The remote address in presentation format.
         \return The new dispatcher for the accepted socket.
         */
        virtual Socket_dispatch* accept(int socket, char* ip) = 0;
        
        //! Read bytes from the socket.
        /*!
         \param b The buffer of bytes read.
         \param sz The number of bytes in the buffer.
         */
        virtual void read(const char* b, int sz) = 0;
        
        //! Write bytes to the socket.
        /*!
         \param sz Pointer to store the size of the buffer.
         \return The buffer of bytes.
         */
        virtual const char* write(int* sz) = 0;
        
        //! Record how many of the bytes were actually written.
        /*!
         \param sz The number of written bytes.
         */
        virtual void written(int sz) = 0;
        
        //! Close the dispatcher.
        virtual void close() = 0;
    };    
}; // namespace lj