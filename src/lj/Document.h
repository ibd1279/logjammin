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

#include <cstdint>
#include <string>

void testEncrypt_friendly();

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
     \endcode
     \par The "_" element.
     The underscore element is a collection of metadata associated with
     this document. Information stored in this element is used by the document
     object to maintain state, version history, etc.
     \par The "version" element.
     The version element is a fixed attribute for all lj::Documents. Only one
     version exists at this time, so this value should always be 100.
     \par The "." element.
     The dot element is the data for this document. This is the user generated
     component of the document.
     \par Encryption
     Encryption and decryption of specific fields is done through the
     \c lj::Document::encrypt() and \c lj::Document::decrypt() methods.
     Encrypted fields are removed from the document, and stored under a "#"
     element. Additional information necessary to decrypt the fields is stored
     under the "_" element.
     \note
     The document object provides a thin utility wrapper around
     a lj::bson::Node. In reality it tracks the root node and the path
     to the current node. Dereferencing a Document object will return the
     current node as read-only, allowing it to be easily used with the
     existing lj::bson methods. A document also keeps track of its
     modified state. Modifications to a document must be done through the
     document interface.
     \author Jason Watson
     \version 1.0
     \sa lj::bson::Node
     */

    class Document
    {
    public:
        static const size_t k_key_size; //!< Number of bytes required for the encryption key.

        // grant the unit test function access.
        friend void ::testEncrypt_friendly();

        //! Default constructor.
        Document();

        //! Wrap a lj::bson::Node pointer.
        /*!
         \param doc The pointer for the document.
         \param is_document True if the pointer is a document object,
            and false if the document is just data.
         */
        Document(lj::bson::Node* doc,
                bool is_document);

        //! Destructor.
        ~Document();

        //! Removed.
        /*!
         Copying documents is not allowed.
         \param orig The original.
         */
        Document(const lj::Document& orig) = delete;

        //! Removed.
        /*!
         Copying documents is not allowed.
         \param orig The original.
         */
        lj::Document& operator=(const lj::Document& orig) = delete;

        //! Get the parent document identifier.
        /*!
         \return The parent identifier.
         */
        inline lj::Uuid parent() const
        {
            return lj::bson::as_uuid(doc_->nav("_/parent"));
        }

        //! Get the document vector clock.
        /*!
         \return The document vector clock.
         */
        inline const lj::bson::Node& vclock() const
        {
            return doc_->nav("_/vclock");
        }

        //! Get the document version.
        /*!
         \return The document version.
         */
        inline int32_t version() const
        {
            return lj::bson::as_int32(doc_->nav("version"));
        }

        //! Get the document key.
        /*!
         \return The document key.
         */
        inline uint64_t key() const
        {
            return lj::bson::as_uint64(doc_->nav("_/key"));
        }

        //! Get the document identifier.
        /*!
         \return The document identifier.
         */
        inline lj::Uuid id() const
        {
            return lj::bson::as_uuid(doc_->nav("_/id"));
        }

        //! Get the suppressed flag.
        /*!
         \return True if the document is suppressed, false otherwise.
         */
        inline bool suppress() const
        {
            return lj::bson::as_boolean(doc_->nav("_/flag/suppressed"));
        }

        //! Get the dirty flag.
        /*!
         \return True if the document is dirty, false otherwise.
         */
        inline bool dirty() const
        {
            return dirty_;
        }

        //! Get the data document.
        /*!
         \return The data node.
         */
        inline const lj::bson::Node& get() const
        {
            return doc_->nav(".");
        }

        //! Get the data document.
        /*!
         \param path The path in the document to get.
         \return The data node.
         */
        inline const lj::bson::Node& get(const std::string& path) const
        {
            return doc_->nav(".").nav(path);
        }

        //! Wash the dirty flag off the object.
        /*!
         A record is dirty/tainted if it has been modified after creation.
         This method is used to clear the dirty flag and treat the object
         as if it has not been modified.
         */
        void wash();

        //! Update the keys for this document.
        /*!
         This is useful for changing the ID of an existing lj::Document. This
         maintains the parent relationships.
         \par
         The vclock for the object is cleared as part of rekeying. This is
         because the rekeyed object is considered in a pristine state.
         \param server The server for the vclock changes.
         \param k The new key to use for rekeying the IDs of the document.
         */
        void rekey(const lj::Uuid& server,
                const uint64_t k);

        //! Create a new branched child of this document.
        /*!
         This is primarily useful for duplicating data. This maintains
         the parent relationships. This is the closest thing to copying
         allowed on a document.
         \param server The server for the vclock changes.
         \param k The new key for the branched object.
         \return The new child.
         \sa rekey
         */
        lj::Document* branch(const lj::Uuid& server,
                const uint64_t k);

        //! Encrypt a document.
        /*!
         \par
         Encrypted fields are removed from the general document and stored in
         hidden fields.
         \param server The id of the server modifying the document.
         \param key The key to use when encrypting.
         \param key_size The size of the key data.
         \param key_name Name to associate with the encrypted data.
         \param paths The paths to encrypt.
         */
        void encrypt(const lj::Uuid& server,
                const uint8_t* key,
                int key_size,
                const std::string& key_name,
                const std::vector<std::string>& paths = std::vector<std::string>());

        //! decrypt a document.
        void decrypt(const uint8_t* key,
                int key_size,
                const std::string& key_name);

        //! Suppress a document.
        void suppress(const lj::Uuid& server,
                const bool s);

        //! Change a value in the document.
        void set(const lj::Uuid& server,
                const std::string& path,
                lj::bson::Node* value);

        //! Change a value in the document.
        void push(const lj::Uuid& server,
                const std::string& path,
                lj::bson::Node* value);

        //! Increment a integer value in the document.
        void increment(const lj::Uuid& server,
                const std::string path,
                int amount);

        //! Convert the document to a string.
        inline operator std::string() const
        {
            return lj::bson::as_pretty_json(*doc_);
        }

    private:
        void seed();
        void taint(const lj::Uuid& server);
        lj::bson::Node* doc_;
        bool dirty_;
    };
}
