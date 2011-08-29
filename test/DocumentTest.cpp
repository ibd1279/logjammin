/* 
 * File:   lj__DocumentTest.cpp
 * Author: jwatson
 *
 * Created on Jun 13, 2011, 10:28:02 PM
 */

#include "testhelper.h"
#include "lj/Document.h"
#include "scrypt/scrypt.h"

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

void testEncrypt()
{
    sample_data data;
    lj::Document doc(new lj::bson::Node(data.doc), false);

    uint8_t dk[CryptoPP::AES::MAX_KEYLENGTH];
    try
    {
        CryptoPP::AutoSeededRandomPool rng;

        uint8_t salt[CryptoPP::AES::MAX_KEYLENGTH];
        rng.GenerateBlock(salt, sizeof(salt));

        std::string password = "some random string the user must provide.";
        crypto_scrypt((uint8_t*)password.data(), password.size(),
                salt, sizeof(salt), 1 << 10, 8, 2, dk, sizeof(dk));
    }
    catch (CryptoPP::Exception& ex)
    {
        throw LJ__Exception(ex.what());
    }

    doc.wash();
    TEST_ASSERT(doc.encrypted() == false);
    TEST_ASSERT(doc.dirty() == false);
    doc.encrypt(data.server, dk, sizeof(dk));
    TEST_ASSERT(doc.encrypted() == true);
    TEST_ASSERT(doc.dirty() == true);
    doc.wash();
    doc.decrypt(dk, sizeof(dk));
    TEST_ASSERT(doc.encrypted() == false);
    TEST_ASSERT(doc.dirty() == false);
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testIncrement),
        PREPARE_TEST(testRekey),
        PREPARE_TEST(testSuppress),
        PREPARE_TEST(testVersion),
        PREPARE_TEST(testWash),
        PREPARE_TEST(testEncrypt),
        {0, ""}
    };
    return Test_util::runner("lj::Document", tests);
}

