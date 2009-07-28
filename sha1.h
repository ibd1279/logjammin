#pragma once
/*
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 */
#include <string>

namespace crypto {
    class SHA1 {
        public:
            SHA1();
            virtual ~SHA1();
            
            /*
             *  Re-initialize the class
             */
            void reset();
            
            /*
             *  Returns the message digest
             */
            bool result(unsigned *message_digest_array);
            
            /*
             *  Provide input to SHA1
             */
            void input( const unsigned char *message_array,
                       unsigned            length);
            void input( const char  *message_array,
                       unsigned    length);
            void input(unsigned char message_element);
            void input(char message_element);
            SHA1& operator<<(const char *message_array);
            SHA1& operator<<(const unsigned char *message_array);
            SHA1& operator<<(const char message_element);
            SHA1& operator<<(const unsigned char message_element);
            SHA1& operator<<(const std::string &message_element);
            
        private:
            
            /*
             *  Process the next 512 bits of the message
             */
            void process_message_block();
            
            /*
             *  Pads the current message block to 512 bits
             */
            void pad_message();
                        
            unsigned _h[5];                      // Message digest buffers
            
            unsigned _length_low;                // Message length in bits
            unsigned _length_high;               // Message length in bits
            
            unsigned char _message_block[64];    // 512-bit message blocks
            int _message_block_index;            // Index into message block array
            
            bool _computed;                      // Is the digest computed?
            bool _corrupted;                     // Is the message digest corruped?
            
    };
};
