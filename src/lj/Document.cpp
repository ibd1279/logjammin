/*!
 \file lj/Document.cpp
 \brief LJ Bson Document implementation
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

#include "lj/Document.h"

#include "cryptopp/aes.h"
#include "cryptopp/eax.h"
#include "cryptopp/filters.h"
#include "cryptopp/osrng.h"
#include "cryptopp/secblock.h"

namespace lj
{
    const size_t Document::k_key_size = CryptoPP::AES::MAX_KEYLENGTH;
    Document::Document() : doc_(NULL), dirty_(true)
    {
        seed();
    }

    Document::Document(lj::bson::Node* doc,
            bool is_document) :
            doc_(NULL),
            dirty_(true)
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

    Document::~Document()
    {
        if (doc_)
        {
            delete doc_;
        }
    }

    void Document::wash()
    {
        dirty_ = false;
    }

    void Document::rekey(const lj::Uuid& server,
            const uint64_t k)
    {
        // Get the old key value before we modify anything.
        const uint64_t old_key = key();

        // parent relationships are updated in the taint method.
        taint(server);
        doc_->set_child("_/key", lj::bson::new_uint64(k));
        doc_->set_child("_/id", lj::bson::new_uuid(lj::Uuid(k)));

        // We only reset the vclock if key has actually changed.
        if (k != old_key)
        {
            doc_->set_child("_/vclock", new lj::bson::Node());
        }
    }

    lj::Document* Document::branch(const lj::Uuid& server,
            const uint64_t k)
    {
        // duplicate the document.
        lj::bson::Node* data = new lj::bson::Node(*doc_);
        lj::Document* child = new lj::Document(data, true);

        // rekey the new document. sets the parent. 
        child->wash();
        child->rekey(server, k);
        return child;
    }
    
    namespace
    {
        template <typename T>
        struct Auto_free_array
        {
            Auto_free_array()
            {
                ptr = nullptr;
            }
            ~Auto_free_array()
            {
                if (ptr)
                {
                    delete[] ptr;
                }
            }
            T* ptr;
        };

        const std::string k_crypt_vector("_/encrypted/vector");
        const std::string k_crypt_data("#");
    }; // namespace (anonymous)

    void Document::encrypt(const lj::Uuid& server,
            uint8_t* key,
            int key_size,
            const std::string& key_name,
            const std::vector<std::string>& paths)
    {
        // Only accept 256bit keys.
        if (CryptoPP::AES::MAX_KEYLENGTH != key_size)
        {
            throw LJ__Exception("encrypt key must 256bits");
        }

        // Create the IV.
        CryptoPP::AutoSeededRandomPool rng;
        byte iv[CryptoPP::AES::BLOCKSIZE * 16];
        rng.GenerateBlock(iv, sizeof(iv));

        // Setup the encrypter.
        CryptoPP::EAX<CryptoPP::AES>::Encryption enc;
        enc.SetKeyWithIV(key, key_size, iv, sizeof(iv));

        // Using something to ensure that the data pointer is freed.
        Auto_free_array<uint8_t> data;
        size_t data_sz = 0;

        // Extract the requested fields to encrypt.
        if (paths.empty())
        {
            // Empty paths, so encrypt everything.
            data.ptr = doc_->nav(".").to_binary(&data_sz);
        }
        else
        {
            // In order to encrypt specific paths, we have to
            // copy the paths out of the current document.
            lj::bson::Node tmp;
            for (auto iter = paths.begin();
                    paths.end() != iter;
                    ++iter)
            {
                tmp.nav(".").set_child(*iter,
                        new lj::bson::Node(doc_->nav(".").nav(*iter)));
            }
            data.ptr = tmp.to_binary(&data_sz);
        }

        try
        {
            // Encrypt the data.
            std::string ct;
            CryptoPP::ArraySource(data.ptr, data_sz, true,
                    new CryptoPP::AuthenticatedEncryptionFilter(enc,
                            new CryptoPP::StringSink(ct)));

            // Store the encrypted document.
            lj::bson::Node* encrypted_data = lj::bson::new_binary(
                    (const uint8_t*)ct.data(), ct.size(),
                    lj::bson::Binary_type::k_bin_user_defined);
            lj::bson::Node* ivector = lj::bson::new_binary(
                    (const uint8_t*)iv, sizeof(iv),
                    lj::bson::Binary_type::k_bin_user_defined);

            // Document is unmodified up to this point. Now we add the
            // encrypted data. This is added before removing any data
            // incase an exception is thrown.
            taint(server);
            doc_->nav(k_crypt_vector).set_child(key_name, ivector);
            doc_->nav(k_crypt_data).set_child(key_name, encrypted_data);
        }
        catch (CryptoPP::Exception& ex)
        {
            throw LJ__Exception(ex.what());
        }

        // Remove the encrypted fields and update tracking.
        if (paths.empty())
        {
            // encrypted the whole document, so nothing to track.
            doc_->set_child(".", NULL);
        }
        else
        {
            // track the specific fields that have been encrypted.
            for (auto iter = paths.begin();
                    paths.end() != iter;
                    ++iter)
            {
                doc_->nav(".").set_child(*iter, NULL);
            }
        }
    }

    void Document::decrypt(uint8_t* key,
            int key_size,
            const std::string& key_name)
    {
        // Only accept 256bit keys.
        if (key_size != CryptoPP::AES::MAX_KEYLENGTH)
        {
            throw LJ__Exception("decrypt key must 256bits");
        }

        // Get the IV.
        const lj::bson::Node& ivector =
                doc_->nav(k_crypt_vector).nav(key_name);
        lj::bson::Binary_type bt;
        uint32_t iv_size;
        const uint8_t* iv = lj::bson::as_binary(ivector,
                &bt,
                &iv_size);

        // Setup the decrypter.
        CryptoPP::EAX<CryptoPP::AES>::Decryption dec;
        dec.SetKeyWithIV(key, key_size, iv, iv_size);

        // Get the document to decrypt.
        const lj::bson::Node& data_node =
                doc_->nav(k_crypt_data).nav(key_name);
        uint32_t data_size;
        const uint8_t* data = lj::bson::as_binary(data_node,
                &bt,
                &data_size);

        try
        {
            // Decrypt the document.
            std::string value;
            CryptoPP::ArraySource(data,
                    data_size,
                    true,
                    new CryptoPP::AuthenticatedDecryptionFilter(dec,
                            new CryptoPP::StringSink(value)));

            // Rebuild the document.
            lj::bson::Node changes(lj::bson::Type::k_document,
                    (const uint8_t*)value.data());
            lj::bson::combine(doc_->nav("."), changes);
        }
        catch (CryptoPP::Exception& ex)
        {
            throw LJ__Exception(ex.what());
        }

        doc_->nav(k_crypt_data).set_child(key_name, NULL);
        doc_->nav(k_crypt_vector).set_child(key_name, NULL);
    }
        
    void Document::suppress(const lj::Uuid& server,
            const bool s)
    {
        taint(server);
        doc_->set_child("_/flag/suppressed", lj::bson::new_boolean(s));
    }

    void Document::set(const lj::Uuid& server,
            const std::string& path,
            lj::bson::Node* value)
    {
        taint(server);
        doc_->nav(".").set_child(path, value);
    }

    void Document::push(const lj::Uuid& server,
            const std::string& path,
            lj::bson::Node* value)
    {
        taint(server);
        doc_->nav(".").push_child(path, value);
    }

    void Document::increment(const lj::Uuid& server,
            const std::string path,
            int amount)
    {
        taint(server);
        lj::bson::increment(doc_->nav(".").nav(path), amount);
    }

    void Document::seed()
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
        doc_->set_child("_/key", lj::bson::new_null());
        doc_->set_child("_/id", lj::bson::new_null());
        doc_->set_child("version", lj::bson::new_int32(100));
        doc_->set_child(".", new lj::bson::Node());
    }
        
    void Document::taint(const lj::Uuid& server)
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
}
