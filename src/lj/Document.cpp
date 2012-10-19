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

#include "lj/Base64.h"
#include "lj/Document.h"
#include "lj/Wiper.h"

extern "C"
{
#include "nettle/aes.h"
#include "nettle/gcm.h"
#include "nettle/yarrow.h"
#include "nettle/memxor.h"
#include "Base64.h"
}

#include <fstream>
#include <iostream>

namespace lj
{
    const size_t Document::k_key_size = AES_MAX_KEY_SIZE;

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
        const std::string k_crypt_vector("_/encrypted/vector");
        const std::string k_crypt_auth("_/encrypted/auth");
        const std::string k_crypt_data("#");
    }; // namespace (anonymous)

    void Document::encrypt(const lj::Uuid& server,
            const uint8_t* key,
            int key_size,
            const std::string& key_name,
            const std::vector<std::string>& paths)
    {
        // Only accept 256bit keys.
        if (k_key_size != key_size)
        {
            throw LJ__Exception("Encrypt key must be 256bits.");
        }

        // Create the source data for the application.
        size_t source_size;
        std::unique_ptr < uint8_t[] > source;
        if (paths.empty())
        {
            // An empty paths list means we should encrypt everything.
            source.reset(doc_->nav(".").to_binary(&source_size));
        }
        else
        {
            // In order to encrypt specific paths, we have to
            // copy the paths out of the current document.
            // The paths will be removed from the document after we successfully
            // encrypt them.
            lj::bson::Node tmp;
            for (auto iter = paths.begin();
                    paths.end() != iter;
                    ++iter)
            {
                lj::bson::Node* ptr =
                        new lj::bson::Node(doc_->nav(".").nav(*iter));
                tmp.nav(".").set_child(*iter, ptr);
            }
            source.reset(tmp.to_binary(&source_size));
        }

        // Generate an initialization vector for the crypto.
        uint8_t iv[GCM_IV_SIZE];
        std::fstream rnd("/dev/random", std::ios_base::in);
        rnd.read(reinterpret_cast<char*>(iv), GCM_IV_SIZE);

        // Prepare all the data structures for the AES crypto in GCM mode.
        struct aes_ctx cipher_ctx;
        aes_set_encrypt_key(&cipher_ctx, key_size, key);
        struct gcm_key auth_key;
        gcm_set_key(&auth_key, &cipher_ctx, (nettle_crypt_func*) & aes_encrypt);
        struct gcm_ctx auth_ctx;
        gcm_set_iv(&auth_ctx, &auth_key, GCM_IV_SIZE, iv);

        // Perform the actual encryption.
        std::unique_ptr < uint8_t[] > destination(new uint8_t[source_size]);
        gcm_encrypt(&auth_ctx,
                &auth_key,
                &cipher_ctx,
                (nettle_crypt_func*) & aes_encrypt,
                source_size,
                destination.get(),
                source.get());
        lj::Wiper < uint8_t[]>::wipe(source, source_size);

        // Extract the authentication information.
        uint8_t auth_tag[GCM_BLOCK_SIZE];
        gcm_digest(&auth_ctx, &auth_key, &cipher_ctx, (nettle_crypt_func*) & aes_encrypt, GCM_BLOCK_SIZE, auth_tag);

        // Create bson Nodes for data necessary for decryption.
        lj::bson::Node* encrypted_node = lj::bson::new_binary(
                destination.get(),
                source_size,
                lj::bson::Binary_type::k_bin_user_defined);
        lj::bson::Node* authentication_node = lj::bson::new_binary(
                auth_tag,
                GCM_BLOCK_SIZE,
                lj::bson::Binary_type::k_bin_user_defined);
        lj::bson::Node* ivector_node = lj::bson::new_binary(
                iv,
                GCM_IV_SIZE,
                lj::bson::Binary_type::k_bin_user_defined);

        // Wipe the temporary memory areas clean
        lj::Wiper<uint8_t[]>::wipe(destination, source_size);
        lj::Wiper<uint8_t[]>::wipe(auth_tag, GCM_BLOCK_SIZE);
        lj::Wiper<uint8_t[]>::wipe(iv, GCM_IV_SIZE);

        // Document is unmodified up to this point. Now we add the
        // encrypted data. This is added before removing any data
        // incase an exception is thrown.
        taint(server);
        doc_->nav(k_crypt_data).set_child(key_name, encrypted_node);
        doc_->nav(k_crypt_auth).set_child(key_name, authentication_node);
        doc_->nav(k_crypt_vector).set_child(key_name, ivector_node);

        // Remove the paths that were just encrypted.
        if (paths.empty())
        {
            doc_->set_child(".", nullptr);
        }
        else
        {
            for (auto iter = paths.begin();
                    paths.end() != iter;
                    ++iter)
            {
                doc_->nav(".").set_child(*iter, nullptr);
            }
        }
    }

    void Document::decrypt(const uint8_t* key,
            int key_size,
            const std::string& key_name)
    {
        // Only accept 256bit keys.
        if (k_key_size != key_size)
        {
            throw LJ__Exception("Decrypt key must be 256bits.");
        }

        // Get the document to decrypt.
        const lj::bson::Node& source_node =
                doc_->nav(k_crypt_data).nav(key_name);
        lj::bson::Binary_type bt;
        uint32_t source_size;
        const uint8_t* source = lj::bson::as_binary(source_node,
                &bt,
                &source_size);

        // Get the initialization vector for the encrypted data.
        const lj::bson::Node& ivector_node =
                doc_->nav(k_crypt_vector).nav(key_name);
        uint32_t iv_size;
        const uint8_t* iv = lj::bson::as_binary(ivector_node,
                &bt,
                &iv_size);

        // Check to make sure the initialization vector is the correct length.
        if (GCM_IV_SIZE != iv_size)
        {
            throw LJ__Exception("Initialization vector for this encrypted data is incorrect.");
        }

        // Prepare all the data structures for the AES crypto in GCM mode.
        struct aes_ctx cipher_ctx;
        aes_set_encrypt_key(&cipher_ctx, key_size, key);
        struct gcm_key auth_key;
        gcm_set_key(&auth_key, &cipher_ctx, (nettle_crypt_func*) & aes_encrypt);
        struct gcm_ctx auth_ctx;
        gcm_set_iv(&auth_ctx, &auth_key, GCM_IV_SIZE, iv);

        // Perform the actual encryption.
        std::unique_ptr < uint8_t[] > destination(new uint8_t[source_size]);
        gcm_decrypt(&auth_ctx,
                &auth_key,
                &cipher_ctx,
                (nettle_crypt_func*) & aes_encrypt,
                source_size,
                destination.get(),
                source);

        // Extract the authentication information.
        uint8_t auth_tag[GCM_BLOCK_SIZE];
        gcm_digest(&auth_ctx, &auth_key, &cipher_ctx, (nettle_crypt_func*) & aes_encrypt, GCM_BLOCK_SIZE, auth_tag);

        // Get the  authentication vector from the encryption.
        const lj::bson::Node& authentication_node =
                doc_->nav(k_crypt_auth).nav(key_name);
        uint32_t authentication_size;
        const uint8_t* original_auth_tag = lj::bson::as_binary(authentication_node,
                &bt,
                &authentication_size);

        // Check to make sure the authentication tag is the correct size.
        if (GCM_BLOCK_SIZE != authentication_size)
        {
            throw LJ__Exception("Authentication tag for this encrypted data is incorrect.");
        }

        // Compare the authentication tag from decryption to the authentication tag from encryption.
        if (memcmp(auth_tag, original_auth_tag, GCM_BLOCK_SIZE) != 0)
        {
            throw LJ__Exception("Authentication tags did not match. Data may be corrupted.");
        }

        // The destination should now contain whatever was originally encrypted.
        // Turn the data back into a bson document
        lj::bson::Node changes(lj::bson::Type::k_document,
                destination.get());

        // Clean up anything sensitive from memory.
        lj::Wiper < uint8_t[]>::wipe(destination, source_size);
        lj::Wiper < uint8_t[]>::wipe(auth_tag, GCM_BLOCK_SIZE);

        // try to combine the documents.  This may throw an exception if things are
        // messed up.
        lj::bson::combine(doc_->nav("."), changes.nav("."));

        // Remove the encrypted data from the document.
        doc_->nav(k_crypt_vector).set_child(key_name, nullptr);
        doc_->nav(k_crypt_auth).set_child(key_name, nullptr);
        doc_->nav(k_crypt_data).set_child(key_name, nullptr);
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
}
