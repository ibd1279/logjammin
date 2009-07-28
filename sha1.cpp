/*
 *  sha1.cpp
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 */

#include "sha1.h"

namespace {
    inline unsigned circular_shift(int bits, unsigned word) {
        return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32-bits));
    }
};

namespace crypto {
    SHA1::SHA1() {
        reset();
    }

    SHA1::~SHA1() {
    }

    void SHA1::reset() {
        _length_low = 0;
        _length_high = 0;
        _message_block_index = 0;
        
        _h[0] = 0x67452301;
        _h[1] = 0xEFCDAB89;
        _h[2] = 0x98BADCFE;
        _h[3] = 0x10325476;
        _h[4] = 0xC3D2E1F0;
        
        _computed    = false;
        _corrupted   = false;
    }
    
    bool SHA1::result(unsigned *message_digest_array) {
        int i;
        
        if(_corrupted) {
            return false;
        }
        
        if(!_computed) {
            pad_message();
            _computed = true;
        }
        
        for(i = 0; i < 5; i++) {
            message_digest_array[i] = _h[i];
        }
        
        return true;
    }

    void SHA1::input(const unsigned char *message_array, unsigned length) {
        if(!length) {
            return;
        }
        
        if(_computed || _corrupted) {
            _corrupted = true;
            return;
        }
        
        while(length-- && !_corrupted) {
            _message_block[_message_block_index++] = (*message_array & 0xFF);
            
            _length_low += 8;
            _length_low &= 0xFFFFFFFF;
            if (_length_low == 0) {
                _length_high++;
                _length_high &= 0xFFFFFFFF;
                if (_length_high == 0) {
                    _corrupted = true;
                }
            }
            
            if (_message_block_index == 64) {
                process_message_block();
            }
            
            message_array++;
        }
    }
    
    void SHA1::input(const char *message_array, unsigned length) {
        input((unsigned char *) message_array, length);
    }
    
    void SHA1::input(unsigned char message_element) {
        input(&message_element, 1);
    }
    
    void SHA1::input(char message_element) {
        input((unsigned char *) &message_element, 1);
    }
    
    SHA1& SHA1::operator<<(const char *message_array) {
        while(message_array) {
            input(message_array++, 1);
        }
        return *this;
    }
    
    SHA1& SHA1::operator<<(const unsigned char *message_array) {
        while(message_array) {
            input(message_array++, 1);
        }
        
        return *this;
    }
    
    SHA1& SHA1::operator<<(const char message_element) {
        input(&message_element, 1);
        return *this;
    }
    
    SHA1& SHA1::operator<<(const unsigned char message_element)
    {
        input(&message_element, 1);
        return *this;
    }
    
    SHA1& SHA1::operator<<(const std::string &message_element) {
        input(message_element.c_str(), message_element.size());
        return *this;
    }
    
    void SHA1::process_message_block() {
        const unsigned K[] = {
            0x5A827999,
            0x6ED9EBA1,
            0x8F1BBCDC,
            0xCA62C1D6
        };
        int t;
        unsigned temp;
        unsigned W[80];
        unsigned A, B, C, D, E;
        
        /*
         *  Initialize the first 16 words in the array W
         */
        for(t = 0; t < 16; t++) {
            W[t] = ((unsigned) _message_block[t * 4]) << 24;
            W[t] |= ((unsigned) _message_block[t * 4 + 1]) << 16;
            W[t] |= ((unsigned) _message_block[t * 4 + 2]) << 8;
            W[t] |= ((unsigned) _message_block[t * 4 + 3]);
        }
        
        for(t = 16; t < 80; t++) {
            W[t] = circular_shift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
        }
        
        A = _h[0];
        B = _h[1];
        C = _h[2];
        D = _h[3];
        E = _h[4];
        
        for(t = 0; t < 20; t++) {
            temp = circular_shift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = circular_shift(30,B);
            B = A;
            A = temp;
        }
        
        for(t = 20; t < 40; t++) {
            temp = circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = circular_shift(30,B);
            B = A;
            A = temp;
        }
        
        for(t = 40; t < 60; t++) {
            temp = circular_shift(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = circular_shift(30,B);
            B = A;
            A = temp;
        }
        
        for(t = 60; t < 80; t++) {
            temp = circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = circular_shift(30,B);
            B = A;
            A = temp;
        }
        
        _h[0] = (_h[0] + A) & 0xFFFFFFFF;
        _h[1] = (_h[1] + B) & 0xFFFFFFFF;
        _h[2] = (_h[2] + C) & 0xFFFFFFFF;
        _h[3] = (_h[3] + D) & 0xFFFFFFFF;
        _h[4] = (_h[4] + E) & 0xFFFFFFFF;
        
        _message_block_index = 0;
    }
    
    void SHA1::pad_message() {

        if (_message_block_index > 55) {
            _message_block[_message_block_index++] = 0x80;
            while(_message_block_index < 64) {
                _message_block[_message_block_index++] = 0;
            }
            
            process_message_block();
            
            while(_message_block_index < 56) {
                _message_block[_message_block_index++] = 0;
            }
        } else {
            _message_block[_message_block_index++] = 0x80;
            while(_message_block_index < 56) {
                _message_block[_message_block_index++] = 0;
            }
        }
        
        /*
         *  Store the message length as the last 8 octets
         */
        _message_block[56] = (_length_high >> 24) & 0xFF;
        _message_block[57] = (_length_high >> 16) & 0xFF;
        _message_block[58] = (_length_high >> 8) & 0xFF;
        _message_block[59] = (_length_high) & 0xFF;
        _message_block[60] = (_length_low >> 24) & 0xFF;
        _message_block[61] = (_length_low >> 16) & 0xFF;
        _message_block[62] = (_length_low >> 8) & 0xFF;
        _message_block[63] = (_length_low) & 0xFF;
        
        process_message_block();
    }
};
