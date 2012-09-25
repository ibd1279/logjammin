/*!
 \file test/DocumentTest.cpp
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

#include "testhelper.h"
#include "lj/Document.h"
#include "lj/Exception.h"
#include "lj/Wiper.h"
#include "scrypt/scrypt.h"
#include "test/DocumentTest_driver.h"
#include <fstream>
#include <unistd.h>

struct sample_data
{
    lj::bson::Node doc;
    lj::Uuid server;

    sample_data() : doc(), server(lj::Uuid::k_ns_dns, "example.com", 11)
    {
        uint8_t data[8] = {10, 10, 10, 10, 10, 10, 10, 10};
        doc.set_child("str", lj::bson::new_string("original foo"));
        doc.set_child("int", lj::bson::new_int64(0x7777777777LL));
        doc.set_child("uint", lj::bson::new_uint64(0xFF77777777ULL));
        doc.set_child("null", lj::bson::new_null());
        doc.set_child("uuid", lj::bson::new_uuid(lj::Uuid()));
        doc.set_child("bool/false", lj::bson::new_boolean(false));
        doc.set_child("bool/true", lj::bson::new_boolean(true));
        doc.set_child("bin", lj::bson::new_binary(data, 8, lj::bson::Binary_type::k_bin_user_defined));
        doc.set_child("annoying\\/path", lj::bson::new_string("Not a nested node"));
        doc["array"] = lj::bson::Node(lj::bson::Type::k_array, NULL);
        doc["array"] << lj::bson::new_int32(100);
        doc["array"] << lj::bson::new_int32(200);
        doc["array"] << lj::bson::new_int32(300);
        doc.push_child("array", lj::bson::new_int32(400));
        doc.push_child("array", lj::bson::new_int32(500));
    }
};

void testIncrement()
{
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc), false);
    doc.wash();
    doc.increment(data.server, "int", 1);
    TEST_ASSERT(lj::bson::as_int64(doc.get("int")) == 0x7777777778LL);
    TEST_ASSERT(doc.dirty());
}

void testRekey()
{
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc["bool"]), false);
    TEST_ASSERT(doc.dirty());
    TEST_ASSERT(doc.key() == 0);
    TEST_ASSERT(doc.parent() == lj::Uuid::k_nil);
    TEST_ASSERT(doc.id() == lj::Uuid::k_nil);
    TEST_ASSERT(!doc.vclock().exists((std::string)data.server));
    doc.rekey(data.server, 100);
    TEST_ASSERT(doc.dirty());
    TEST_ASSERT(doc.key() == 100);
    TEST_ASSERT(doc.parent() == lj::Uuid::k_nil);
    TEST_ASSERT((uint64_t)doc.id() == 100);
    TEST_ASSERT(!doc.vclock().exists((std::string)data.server));
    doc.wash();
    const lj::Uuid expected_parent(doc.id());
    doc.rekey(data.server, 200);
    TEST_ASSERT(doc.dirty());
    TEST_ASSERT(doc.key() == 200);
    TEST_ASSERT(doc.parent() == expected_parent);
    TEST_ASSERT((uint64_t)doc.id() == 200);
    TEST_ASSERT(!doc.vclock().exists((std::string)data.server));
}

void testBranch()
{
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc["bool"]), false);
    doc.rekey(data.server, 100);
    doc.wash();
    const lj::Uuid expected_parent1(doc.id());
    lj::Document* doc2 = doc.branch(data.server, 200);
    doc2->wash();
    TEST_ASSERT(doc2->key() == 200);
    TEST_ASSERT(doc2->parent() == expected_parent1);
    TEST_ASSERT(static_cast<uint64_t>(doc2->id()) == 200);
    delete doc2;
}

void testSuppress()
{
    sample_data data;
    lj::Document doc;
    TEST_ASSERT(doc.suppress() == false);
    doc.wash();
    doc.suppress(data.server, true);
    TEST_ASSERT(doc.dirty());
    TEST_ASSERT(doc.suppress() == true);
    doc.suppress(data.server, false);
    TEST_ASSERT(doc.suppress() == false);
}

void testVersion()
{
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc), false);
    TEST_ASSERT(doc.version() == 100);
    lj::Document doc2;
    TEST_ASSERT(doc.version() == 100);
}

void testWash()
{
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc), false);
    const std::string path("bool/maybe");

    TEST_ASSERT(doc.dirty() == true);
    doc.wash();
    TEST_ASSERT(doc.dirty() == false);
    TEST_ASSERT(!doc.vclock().exists((std::string)data.server));
    doc.set(data.server, path, lj::bson::new_boolean(false));
    TEST_ASSERT(doc.dirty() == true);
    TEST_ASSERT(doc.vclock().exists((std::string)data.server));
    TEST_ASSERT(lj::bson::as_uint64(doc.vclock().nav((std::string)data.server)) == 1);
    doc.wash();
    TEST_ASSERT(doc.dirty() == false);
    TEST_ASSERT(doc.vclock().exists((std::string)data.server));
    TEST_ASSERT(lj::bson::as_uint64(doc.vclock().nav((std::string)data.server)) == 1);
    doc.set(data.server, path, lj::bson::new_boolean(true));
    TEST_ASSERT(doc.dirty() == true);
    TEST_ASSERT(doc.vclock().exists((std::string)data.server));
    TEST_ASSERT(lj::bson::as_uint64(doc.vclock().nav((std::string)data.server)) == 2);
}

void testEncrypt_friendly()
{
    // Take the sample data and create a document.
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc), false);

    // Create the derived key for encryption and decryption.
    std::unique_ptr < uint8_t[] > dk(new uint8_t[lj::Document::k_key_size]);
    std::unique_ptr < uint8_t[] > salt(new uint8_t[lj::Document::k_key_size]);
    std::fstream rnd("/dev/urandom", std::ios_base::in);
    rnd.read((char*)salt.get(), lj::Document::k_key_size);

    // fill dk with the digested password.
    std::string password = "some random string the user must provide.";
    crypto_scrypt((uint8_t*)password.data(), password.size(),
            salt.get(), lj::Document::k_key_size, 1 << 10, 8, 2, dk.get(), lj::Document::k_key_size);

    // What fields are we going to encrypt?
    std::vector<std::string> paths;
    paths.push_back(std::string("str"));
    paths.push_back(std::string("bool/false"));

    // Test to make sure those fields exist before we encrypt
    TEST_ASSERT(doc.get("bool").exists("false") == true);
    TEST_ASSERT(doc.get().exists("str") == true);
    TEST_ASSERT(lj::bson::as_string(doc.get("str")).compare(
            "original foo") == 0);

    // Encrypt the paths.
    doc.encrypt(data.server,
            dk.get(),
            lj::Document::k_key_size,
            std::string("test"),
            paths);

    // Test that they are gone.
    TEST_ASSERT(doc.get("bool").exists("false") == false);
    TEST_ASSERT(doc.get().exists("str") == false);

    // this is a complicated series of steps to circumvent the invariant
    // aspects of the document class in order to test a failure condition.
    lj::bson::Node& encrypted_node = doc.doc_->nav("#/test");
    lj::bson::Binary_type bt;
    uint32_t source_size;
    const uint8_t* source = lj::bson::as_binary(encrypted_node,
            &bt,
            &source_size);
    uint8_t* illegal_source = const_cast<uint8_t*>(source);
    illegal_source[9] = illegal_source[9] - 1;

    // Try to unsuccessfully decrypt the document.
    try
    {
        doc.decrypt(dk.get(),
                lj::Document::k_key_size,
                std::string("test"));
        TEST_FAILED("decryption should have failed because of the corrupted data.");
    }
    catch (lj::Exception& ex)
    {
    }

    // Try to decrypt the document.
    illegal_source[9] = illegal_source[9] + 1;
    try
    {
        doc.decrypt(dk.get(),
                lj::Document::k_key_size,
                std::string("test"));
    }
    catch (lj::Exception& ex)
    {
        TEST_FAILED(ex.str());
    }

    // Test to make sure those returned.
    TEST_ASSERT(doc.get("bool").exists("false") == true);
    TEST_ASSERT(doc.get().exists("str") == true);
    TEST_ASSERT(lj::bson::as_string(doc.get("str")).compare(
            "original foo") == 0);
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Document", tests);
}

