#pragma once
/*!
 \file lj/Document.h
 \brief LJ Bson Document header.
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

#include "lj/Bson.h"
#include "lj/Uuid.h"

#include "crypto++/aes.h"
#include "crypto++/eax.h"
#include "crypto++/filters.h"
#include "crypto++/osrng.h"
#include "crypto++/secblock.h"

#include <cstdint>
#include <string>

namespace lj
{
    //! Bson Document
    /*!
     \par
     A bson document represents a bson document structure and maintains
     specific metadata about the bson document structure. At a high level the
     v1 document looks like this.
     \code
     {
       "_" : {},
       "version" : 100,
       "." : {}
     }
     \encode
     \par The "_" element.
     The underscore element is a collection of metadata associated with
     this document. Information stored in this element is used by the document
     object to maintain state, version history, etc.
     \par The "version" element.
     The version element is a fixed attribute for all lj::Documents. Only one
     version exists at this time, so this value should always be 1.
     \par The "." element.
     The dot element is the data for this document. This is the user generated
     component of the document.
     
     \author Jason Watson
     \version 1.0
     \date June 11, 2011
     */
    class Document
    {
    public:
        Document() : doc_(NULL), dirty_(true)
        {
            seed();
        }
        Document(lj::bson::Node* doc, bool is_document) : doc_(NULL), dirty_(true)
        {
            if (is_document)
            {
                doc_ = doc;
                dirty_ = false;
            }
            else
            {
                seed();
                doc_->set_child(".", doc);
            }
        }
        ~Document()
        {
            if (doc_)
            {
                delete doc_;
            }
        }
        Document(const lj::Document& orig) = delete;
        lj::Document& operator=(const lj::Document& orig) = delete;
        
        inline lj::Uuid parent() const
        {
            return lj::bson::as_uuid(doc_->nav("_/parent"));
        }
        inline const lj::bson::Node& vclock() const
        {
            return doc_->nav("_/vclock");
        }
        inline int32_t version() const
        {
            return lj::bson::as_int32(doc_->nav("version"));
        }
        inline uint64_t key() const
        {
            return lj::bson::as_uint64(doc_->nav("_/key"));
        }
        inline lj::Uuid id() const
        {
            return lj::bson::as_uuid(doc_->nav("_/id"));
        }
        inline bool suppress() const
        {
            return lj::bson::as_boolean(doc_->nav("_/flag/suppressed"));
        }
        inline bool dirty() const
        {
            return dirty_;
        }
        inline bool encrypted() const
        {
            bool flag = lj::bson::as_boolean(doc_->nav("_/flag/encrypted"));
            return flag || !doc_->exists(".") || doc_->exists("#");
        }
        inline const lj::bson::Node& get(const std::string& path) const
        {
            if (encrypted())
            {
                throw LJ__Exception("not allowed on encrypted doc.");
            }
            return doc_->nav(".").nav(path);
        }

        void wash()
        {
            dirty_ = false;
        }
        void rekey(const lj::Uuid& server, const uint64_t k)
        {
            taint(server);
            doc_->set_child("_/key", lj::bson::new_uint64(k));
            doc_->set_child("_/id", lj::bson::new_uuid(lj::Uuid(k)));
            doc_->set_child("_/vclock", new lj::bson::Node());
        }

        void encrypt(uint8_t* key, int key_size)
        {
            // Only accept 256bit keys.
            if (key_size != CryptoPP::AES::MAX_KEYLENGTH)
            {
                throw LJ__Exception("encrypt key must 256bits");
            }
            if (encrypted())
            {
                throw LJ__Exception("double encryption requested.");
            }

            // Create the IV.
            CryptoPP::AutoSeededRandomPool rng;
            byte iv[CryptoPP::AES::BLOCKSIZE * 16];
            rng.GenerateBlock(iv, sizeof(iv));

            // Setup the encrypter.
            CryptoPP::EAX<CryptoPP::AES>::Encryption enc;
            enc.SetKeyWithIV(key, key_size, iv, sizeof(iv));

            // Encrypt the document.
            uint8_t* data = doc_->to_binary();
            try
            {
                std::string ct;
                CryptoPP::ArraySource(data, doc_->size(), true,
                        new CryptoPP::AuthenticatedEncryptionFilter(enc,
                                new CryptoPP::StringSink(ct)));

                // Rebuild the document.
                doc_->set_child("_/flag/encrypted",
                        lj::bson::new_boolean(true));
                doc_->set_child(".", NULL);
                doc_->set_child("#", lj::bson::new_binary(
                        (const uint8_t*)ct.data(), ct.size(),
                        lj::bson::Binary_type::k_bin_user_defined));
                doc_->set_child("_/vector", lj::bson::new_binary(
                        (const uint8_t*)iv, sizeof(iv),
                        lj::bson::Binary_type::k_bin_user_defined));
            }
            catch (CryptoPP::Exception& ex)
            {
                throw LJ__Exception(ex.what());
            }

            // clean up.
            delete[] data;
        }

        void decrypt(uint8_t* key, int key_size)
        {
            // Only accept 256bit keys.
            if (key_size != CryptoPP::AES::MAX_KEYLENGTH)
            {
                throw LJ__Exception("decrypt key must 256bits");
            }
            if (!encrypted())
            {
                throw LJ__Exception("double decryption requested.");
            }

            // Get the IV.
            lj::bson::Binary_type bt;
            uint32_t iv_size;
            const uint8_t* iv = lj::bson::as_binary(doc_->nav("_/vector"),
                    &bt,
                    &iv_size);

            // Setup the decrypter.
            CryptoPP::EAX<CryptoPP::AES>::Decryption dec;
            dec.SetKeyWithIV(key, key_size, iv, iv_size);

            // decrypt the document.
            uint32_t data_size;
            const uint8_t* data = lj::bson::as_binary(doc_->nav("#"),
                    &bt,
                    &data_size);
            try
            {
                std::string value;
                CryptoPP::ArraySource(data,
                        data_size,
                        true,
                        new CryptoPP::AuthenticatedDecryptionFilter(dec,
                                new CryptoPP::StringSink(value)));

                // Rebuilding the document is going to invalidate these
                // pointers.
                data = NULL;
                iv = NULL;

                // Rebuild the document.
                doc_->set_value(lj::bson::Type::k_document,
                        (const uint8_t*)value.data());
            }
            catch (CryptoPP::Exception& ex)
            {
                throw LJ__Exception(ex.what());
            }
        }
        
        void suppress(const lj::Uuid& server, const bool s)
        {
            if (encrypted())
            {
                throw LJ__Exception("not allowed on encrypted doc.");
            }
            taint(server);
            doc_->set_child("_/flag/suppressed", lj::bson::new_boolean(s));
        }
        void set(const lj::Uuid& server, const std::string& path, lj::bson::Node* value)
        {
            if (encrypted())
            {
                throw LJ__Exception("not allowed on encrypted doc.");
            }
            taint(server);
            doc_->nav(".").set_child(path, value);
        }
        void push(const lj::Uuid& server, const std::string& path, lj::bson::Node* value)
        {
            if (encrypted())
            {
                throw LJ__Exception("not allowed on encrypted doc.");
            }
            taint(server);
            doc_->nav(".").nav(path) << value;
        }
        void increment(const lj::Uuid& server, const std::string path, int amount)
        {
            if (encrypted())
            {
                throw LJ__Exception("not allowed on encrypted doc.");
            }
            taint(server);
            lj::bson::increment(doc_->nav(".").nav(path), amount);
        }
        operator std::string() const
        {
            return lj::bson::as_pretty_json(*doc_);
        }

    private:
        void seed()
        {
            if (doc_)
            {
                delete doc_;
            }
            doc_ = new lj::bson::Node();
            dirty_ = true;
            
            doc_->set_child("_/parent", lj::bson::new_null());
            doc_->set_child("_/vclock", new lj::bson::Node());
            doc_->set_child("_/flag/suppressed", lj::bson::new_boolean(false));
            doc_->set_child("_/flag/encrypted", lj::bson::new_boolean(false));
            doc_->set_child("_/key", lj::bson::new_null());
            doc_->set_child("_/id", lj::bson::new_null());
            doc_->set_child("version", lj::bson::new_int32(100));
            doc_->set_child(".", new lj::bson::Node());
        }
        
        void taint(const lj::Uuid& server)
        {
            if (!dirty_)
            {
                // Update flags.
                dirty_ = true;
                
                // Create new revision ID.
                doc_->set_child("_/parent", new lj::bson::Node(doc_->nav("_/id")));
                doc_->set_child("_/id", lj::bson::new_uuid(lj::Uuid(key())));
                
                // Update the vclock
                lj::bson::increment(doc_->nav("_/vclock").nav(server), 1);
            }
        }
        lj::bson::Node* doc_;
        bool dirty_;
    };
}
