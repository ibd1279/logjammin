/* 
 * File:   BsonTest.cpp
 * Author: jwatson
 *
 * Created on Jun 1, 2011, 11:22:46 PM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include <sstream>
#include "test/BsonTest_driver.h"

struct sample_doc
{
    lj::bson::Node root;
    
    sample_doc()
    {
        uint8_t data[8] = {10, 10, 10, 10, 10, 10, 10, 10};
        root.set_child("str", lj::bson::new_string("original foo"));
        root.set_child("int", lj::bson::new_int64(0x7777777777LL));
        root.set_child("uint", lj::bson::new_uint64(0xFF77777777ULL));
        root.set_child("null", lj::bson::new_null());
        root.set_child("uuid", lj::bson::new_uuid(lj::Uuid()));
        root.set_child("bool/false", lj::bson::new_boolean(false));
        root.set_child("bool/true", lj::bson::new_boolean(true));
        root.set_child("bin", lj::bson::new_binary(data, 8, lj::bson::Binary_type::k_bin_user_defined));
        root.set_child("annoying\\/path", lj::bson::new_string("Not a nested node"));
        root["array"] = lj::bson::Node(lj::bson::Type::k_array, NULL);
        root["array"] << lj::bson::new_int32(100);
        root["array"] << lj::bson::new_int32(200);
        root["array"] << lj::bson::new_int32(300);
        root.push_child("array", lj::bson::new_int32(400));
        root.push_child("array", lj::bson::new_int32(500));
    }
};

void testCopy_from()
{
    sample_doc doc;
    lj::bson::Node o;
    o.copy_from(doc.root);
    TEST_ASSERT(lj::bson::as_string(doc.root).compare(lj::bson::as_string(o)) == 0);
}

void testAssignment()
{
    sample_doc doc;
    lj::bson::Node o;

    o = doc.root;
    TEST_ASSERT(lj::bson::as_string(doc.root).compare(lj::bson::as_string(o)) == 0);
    
    try
    {
        o.set_value(lj::bson::Type::k_null, NULL);
        o.set_value(lj::bson::Type::k_document, NULL);
        o.set_value(lj::bson::Type::k_array, NULL);
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        TEST_FAILED("Null, document, and array should support null");
    }

    try
    {
        o.set_value(lj::bson::Type::k_int32, NULL);
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        // test was successful. this should throw an error.
    }
}

void testAssignmentCrossTypes()
{
    // default constructor makes documents.
    lj::bson::Node o;
    sample_doc doc;

    o = doc.root["str"];
    TEST_ASSERT(lj::bson::as_string(doc.root["str"]).compare(lj::bson::as_string(o)) == 0);
}

void testIstreamExtraction()
{
    sample_doc doc;
    lj::bson::Node o;
    std::stringstream ss(std::stringstream::in | std::stringstream::out
            | std::stringstream::binary);
    char* bytes = reinterpret_cast<char*>(doc.root.to_binary());
    ss.write(bytes, doc.root.size());
    delete[] bytes;

    ss >> o;

    TEST_ASSERT(lj::bson::as_string(doc.root).compare(lj::bson::as_string(o)) == 0);
}
void testNullify()
{
    sample_doc doc;
    doc.root["array"].nullify();
    TEST_ASSERT(lj::bson::as_string(doc.root["array"]).compare("null") == 0);
}
void testPath()
{
    sample_doc doc;
    const sample_doc* cdoc = &doc;
    
    // Const test case.
    const lj::bson::Node* ptr = cdoc->root.path("some/unknown/path");
    TEST_ASSERT(NULL == ptr);
    
    // mutable test case.
    doc.root.path("some/unknown/path");
    ptr = cdoc->root.path("some/unknown/path");
    TEST_ASSERT(NULL != ptr);
    TEST_ASSERT(lj::bson::Type::k_document == ptr->type());
    TEST_ASSERT(lj::bson::as_string(*ptr).compare("{}") == 0);
    
}
void testPath2()
{
    // testing path support for navigating array indices.
    sample_doc doc;
    sample_doc doc2;
    
    doc.root.set_child("array", new lj::bson::Node(lj::bson::Type::k_array, NULL));
    
    doc.root["array"] << lj::bson::Node();
    doc.root["array"] << lj::bson::Node();
    doc.root["array"] << lj::bson::Node();
    doc.root["array"] << lj::bson::Node();
    
    doc.root.set_child("array/2/data", new lj::bson::Node(doc2.root));
    
    TEST_ASSERT(lj::bson::as_string(*(doc.root.path("array/2/data/bool"))).compare(lj::bson::as_string(*(doc.root.path("bool")))) == 0);
    TEST_ASSERT(lj::bson::as_string(*(doc.root.path("array/3"))).compare(lj::bson::as_string(*(doc.root.path("array/0")))) == 0);
}
void testSize()
{
    sample_doc doc;
    
    TEST_ASSERT(1 == doc.root["bool/true"].size());
    TEST_ASSERT(17 == doc.root["str"].size());
    TEST_ASSERT(8 == doc.root["int"].size());
    TEST_ASSERT(0 == doc.root["null"].size());
    TEST_ASSERT(21 == doc.root["uuid"].size());
    TEST_ASSERT(13 == doc.root["bin"].size());
    TEST_ASSERT(40 == doc.root["array"].size());
    TEST_ASSERT(20 == doc.root["bool"].size());
}
void testExists()
{
    sample_doc doc;
    
    TEST_ASSERT(!doc.root.exists("some/unknown/path"));
    doc.root.path("some/unknown/path");
    TEST_ASSERT(doc.root.exists("some/unknown/path"));
    doc.root.set_child("some/unknown/path", NULL);
    TEST_ASSERT(!doc.root.exists("some/unknown/path"));
}
void testNav()
{
    sample_doc doc;
    const sample_doc* cdoc = &doc;
    
    // Const test case.
    try
    {
        cdoc->root.nav("some/unknown/path");
        TEST_FAILED("Const nav should thrown an exception for unknown paths.");
    }
    catch (lj::bson::Bson_path_exception& ex)
    {
        std::cout << ex.str() << std::endl;
    }
    
    // mutable test case.
    try
    {
        doc.root.nav("some/unknown/path");
        const lj::bson::Node& cn = cdoc->root.nav("some/unknown/path");
        
        TEST_ASSERT(lj::bson::Type::k_document == cn.type());
        TEST_ASSERT(lj::bson::as_string(cn).compare("{}") == 0);
    }
    catch (lj::bson::Bson_path_exception& ex)
    {
        TEST_FAILED("Const nav should not throw an exception for a node that exists.");
    }
    
}
void testNav2()
{
    // testing path support for navigating array indices.
    sample_doc doc;
    sample_doc doc2;
    
    doc.root.set_child("array", new lj::bson::Node(lj::bson::Type::k_array, NULL));
    
    doc.root["array"] << lj::bson::Node();
    doc.root["array"] << lj::bson::Node();
    doc.root["array"] << lj::bson::Node();
    doc.root["array"] << lj::bson::Node();
    
    doc.root.set_child("array/2/data", new lj::bson::Node(doc2.root));
    
    TEST_ASSERT(lj::bson::as_string(doc.root.nav("array/2/data/bool")).compare(lj::bson::as_string(doc.root.nav("bool"))) == 0);
    TEST_ASSERT(lj::bson::as_string(doc.root.nav("array/3")).compare(lj::bson::as_string(doc.root.nav("array/0"))) == 0);
}
void testTo_binary()
{
    sample_doc doc;
    
    uint8_t* bytes = doc.root.to_binary();
    lj::bson::Node n(lj::bson::Type::k_document, bytes);
    
    TEST_ASSERT(lj::bson::as_string(doc.root).compare(lj::bson::as_string(n)) == 0);
}
void testTo_map()
{
    sample_doc doc;
    lj::bson::Node& n = doc.root["bool"];
    bool t = false, f = false, o = false;
    
    for (auto iter = n.to_map().begin(); n.to_map().end() != iter; ++iter)
    {
        if (iter->first.compare("true") == 0 && lj::bson::as_boolean(*(iter->second)) == true)
        {
            t = true;
        }
        else if (iter->first.compare("false") == 0 && lj::bson::as_boolean(*(iter->second)) == false)
        {
            f = true;
        }
        else
        {
            o = true;
        }
    }
    
    TEST_ASSERT(t && f && !o);
    
    try
    {
        doc.root["int"].to_map();
        TEST_FAILED("Non-document types should not allow to_map.");
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        std::cout << ex.str() << std::endl;
    }
}
void testTo_value()
{
    sample_doc doc;
    TEST_ASSERT(doc.root["int"].to_value());
    TEST_ASSERT(doc.root["bool/true"].to_value());
    TEST_ASSERT(doc.root["uuid"].to_value());
    TEST_ASSERT(doc.root["null"].to_value() == NULL);
    TEST_ASSERT(doc.root["bin"].to_value());
    
    try
    {
        doc.root["array"].to_value();
        TEST_FAILED("Array types should not allow to_value.");
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        std::cout << ex.str() << std::endl;
    }
    
    try
    {
        doc.root["bool"].to_value();
        TEST_FAILED("Document types should not allow to_value.");
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        std::cout << ex.str() << std::endl;
    }
}
void testTo_vector()
{
    sample_doc doc;
    lj::bson::Node& n = doc.root["array"];
    
    TEST_ASSERT(5 == n.to_vector().size());
    
    int h = 1;
    for (auto iter = n.to_vector().begin(); n.to_vector().end() != iter; ++iter, ++h)
    {
        TEST_ASSERT(lj::bson::as_int64(*(*iter)) == h * 100);
    }
    
    try
    {
        doc.root["int"].to_vector();
        TEST_FAILED("Non-array types should not allow to_vector.");
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        std::cout << ex.str() << std::endl;
    }
}
void testType()
{
    sample_doc doc;
    
    TEST_ASSERT(lj::bson::Type::k_boolean == doc.root["bool/true"].type());
    TEST_ASSERT(lj::bson::Type::k_string == doc.root["str"].type());
    TEST_ASSERT(lj::bson::Type::k_int64 == doc.root["int"].type());
    TEST_ASSERT(lj::bson::Type::k_null == doc.root["null"].type());
    TEST_ASSERT(lj::bson::Type::k_binary == doc.root["uuid"].type());
    TEST_ASSERT(lj::bson::Type::k_binary == doc.root["bin"].type());
    TEST_ASSERT(lj::bson::Type::k_array == doc.root["array"].type());
    TEST_ASSERT(lj::bson::Type::k_document == doc.root["bool"].type());
}

void testParse()
{
    lj::Log::debug.enable();
    const std::string simple_array("[\n  \"1\",\n  \"hello\",\n  \"3\"\n]");
    lj::bson::Node* result = lj::bson::parse_string(simple_array);
    TEST_ASSERT(simple_array.compare(lj::bson::as_pretty_json(*result)) == 0);

    const std::string complex_array("[\n\
  [\n\
    \"1\",\n\
    [\n\
      \"hello\"\n\
    ],\n\
    \"3\"\n\
  ],\n\
  [\n\
    \"4\",\n\
    5\n\
  ]\n\
]");
    result = lj::bson::parse_string(complex_array);
    TEST_ASSERT(complex_array.compare(lj::bson::as_pretty_json(*result)) == 0);

    const std::string simple_document("{\n\
  \"foo\": 500,\n\
  \"bar\": false,\n\
  \"bool\": TRUE,\n\
  \'nil\': null,\n\
  \"str\": \'Some string.\'\n\
}");
    const std::string doc1_expected("{\n\
  \"bar\":0,\n\
  \"bool\":1,\n\
  \"foo\":500,\n\
  \"nil\":null,\n\
  \"str\":\"Some string.\"\n\
}");
    result = lj::bson::parse_string(simple_document);
    TEST_ASSERT(doc1_expected.compare(lj::bson::as_pretty_json(*result)) == 0);

    const std::string complex_document("{\n\
  \"foo\": 500,\n\
  \"bar\": false,\n\
  \"bool\": TRUE,\n\
  \'nil\': null,\n\
  \"str\": \'Some string.\',\n\
  \"nested\": [ { \"tmp\": {}, \"breakme\": [], \"comment\": null },\n\
                { \"tmp\": { \"a\": \'b\' }, \'breakme\': [1,2,3,4,5], \"comment\": \"this is annoying to create\" }],\n\
  \"escape\": \'We don\\\'t need no\\nstinking escapes.\'\n\
}");

    const std::string doc2_expected("{\n\
  \"bar\":0,\n\
  \"bool\":1,\n\
  \"escape\":\"We don\'t need no\n\
stinking escapes.\",\n\
  \"foo\":500,\n\
  \"nested\":[\n\
    {\n\
      \"breakme\":[],\n\
      \"comment\":null,\n\
      \"tmp\":{}\n\
    },\n\
    {\n\
      \"breakme\":[\n\
        1,\n\
        2,\n\
        3,\n\
        4,\n\
        5\n\
      ],\n\
      \"comment\":\"this is annoying to create\",\n\
      \"tmp\":{\n\
        \"a\":\"b\"\n\
      }\n\
    }\n\
  ],\n\
  \"nil\":null,\n\
  \"str\":\"Some string.\"\n\
}");
    result = lj::bson::parse_string(complex_document);
    TEST_ASSERT(doc2_expected.compare(lj::bson::as_pretty_json(*result)) == 0);
}
void testAs_binary()
{
    sample_doc doc;
    lj::bson::Binary_type t;
    uint32_t sz;
    
    try
    {
        lj::bson::as_binary(doc.root["int"], &t, &sz);
        TEST_FAILED("as_binary should not work on non-binary types.");
    }
    catch (lj::bson::Bson_type_exception& ex)
    {
        std::cout << ex.str() << std::endl;
    }
    
    const uint8_t* ptr = lj::bson::as_binary(doc.root["uuid"], &t, &sz);
    TEST_ASSERT(ptr == doc.root["uuid"].to_value() + 5);
    std::cout << (int)t << std::endl;
    TEST_ASSERT(t == lj::bson::Binary_type::k_bin_uuid);
    TEST_ASSERT(sz == 16);
    
    ptr = lj::bson::as_binary(doc.root["bin"], &t, &sz);
    TEST_ASSERT(ptr == doc.root["bin"].to_value() + 5);
    TEST_ASSERT(t == lj::bson::Binary_type::k_bin_user_defined);
    TEST_ASSERT(sz == 8);
}
void testAs_boolean()
{
    sample_doc doc;
    
    doc.root.set_child("array", new lj::bson::Node(lj::bson::Type::k_array, NULL));
    doc.root["array"] << lj::bson::new_boolean(true) << lj::bson::new_int64(1) << lj::bson::new_int64(1024);
    doc.root["array"] << lj::bson::new_string("1") << lj::bson::new_string("true") << lj::bson::new_string("TRUE");
    for (auto iter = doc.root["array"].to_vector().begin(); doc.root["array"].to_vector().end() != iter; ++iter)
    {
        TEST_ASSERT(lj::bson::as_boolean(*(*iter)));
    }
    
    doc.root.set_child("array", new lj::bson::Node(lj::bson::Type::k_array, NULL));
    doc.root["array"] << lj::bson::new_boolean(false) << lj::bson::new_int64(0) << lj::bson::new_string("random true string that isn't the word true");
    doc.root["array"] << lj::bson::new_string("0") << lj::bson::new_string("") << lj::bson::new_string("true ");
    for (auto iter = doc.root["array"].to_vector().begin(); doc.root["array"].to_vector().end() != iter; ++iter)
    {
        TEST_ASSERT(!lj::bson::as_boolean(*(*iter)));
    }
}
void testAs_debug_string()
{
    sample_doc doc;
    doc.root.set_child("uuid", lj::bson::new_uuid(lj::Uuid("{2ae24c43-8cf9-4590-9d1a-fc5e8583a4bd}")));
    
    const std::string expected = "{(size-4)215\n\
  (type-1)string\"(key-14)annoying/path\":\"(size-4)18(value-18)Not a nested node\",\n\
  (type-1)array\"(key-6)array\":{(size-4)40\n\
    (type-1)int32\"(key-2)0\":(value-4)100,\n\
    (type-1)int32\"(key-2)1\":(value-4)200,\n\
    (type-1)int32\"(key-2)2\":(value-4)300,\n\
    (type-1)int32\"(key-2)3\":(value-4)400,\n\
    (type-1)int32\"(key-2)4\":(value-4)500\n\
  (null-1)0},\n\
  (type-1)binary\"(key-4)bin\":(size-4)8(bin-type-1)user-defined(value-8)CgoKCgoKCgo=,\n\
  (type-1)document\"(key-5)bool\":{(size-4)20\n\
    (type-1)boolean\"(key-6)false\":(value-1)0,\n\
    (type-1)boolean\"(key-5)true\":(value-1)1\n\
  (null-1)0},\n\
  (type-1)int64\"(key-4)int\":(value-8)513105426295,\n\
  (type-1)null\"(key-5)null\":(value-0),\n\
  (type-1)string\"(key-4)str\":\"(size-4)13(value-13)original foo\",\n\
  (type-1)int64\"(key-5)uint\":(value-8)1097220978551,\n\
  (type-1)binary\"(key-5)uuid\":(size-4)16(bin-type-1)uuid(value-16){2ae24c43-8cf9-4590-9d1a-fc5e8583a4bd}/3090116147341252871\n\
(null-1)0}";
    
    TEST_ASSERT(expected.compare(lj::bson::as_debug_string(doc.root)) == 0);
}
void testAs_string()
{
    sample_doc doc;
    doc.root.set_child("uuid", lj::bson::new_uuid(lj::Uuid("{2ae24c43-8cf9-4590-9d1a-fc5e8583a4bd}")));
    
    const std::string expected = "{\"annoying/path\":\"Not a nested node\", \"array\":[\"0\":100, \"1\":200, \"2\":300, \"3\":400, \"4\":500], \"bin\":CgoKCgoKCgo=, \"bool\":{\"false\":0, \"true\":1}, \"int\":513105426295, \"null\":null, \"str\":\"original foo\", \"uint\":1097220978551, \"uuid\":{2ae24c43-8cf9-4590-9d1a-fc5e8583a4bd}/3090116147341252871}";
    TEST_ASSERT(expected.compare(lj::bson::as_string(doc.root)) == 0);
}
void testAs_pretty_json()
{
    sample_doc doc;
    doc.root.set_child("uuid", lj::bson::new_uuid(lj::Uuid("{2ae24c43-8cf9-4590-9d1a-fc5e8583a4bd}")));
    
    const std::string expected = "{\n\
  \"annoying/path\":\"Not a nested node\",\n\
  \"array\":[\n\
    100,\n\
    200,\n\
    300,\n\
    400,\n\
    500\n\
  ],\n\
  \"bin\":\"CgoKCgoKCgo=\",\n\
  \"bool\":{\n\
    \"false\":0,\n\
    \"true\":1\n\
  },\n\
  \"int\":513105426295,\n\
  \"null\":null,\n\
  \"str\":\"original foo\",\n\
  \"uint\":1097220978551,\n\
  \"uuid\":\"{2ae24c43-8cf9-4590-9d1a-fc5e8583a4bd}/3090116147341252871\"\n\
}";
    
    TEST_ASSERT(expected.compare(lj::bson::as_pretty_json(doc.root)) == 0);
}
void testAs_int32()
{
    sample_doc doc;
    
    TEST_ASSERT(lj::bson::as_int32(doc.root["array/0"]) == 100);
}
void testAs_int64()
{
    sample_doc doc;
    
    TEST_ASSERT(lj::bson::as_int64(doc.root["int"]) == 0x7777777777LL);
}
void testAs_uint64()
{
    sample_doc doc;
    
    TEST_ASSERT(lj::bson::as_uint64(doc.root["uint"]) == 0xFF77777777ULL);
}
void testAs_uuid()
{
    sample_doc doc;
    
    lj::Uuid uuid(lj::bson::as_uuid(doc.root["uuid"]));
    
    lj::bson::Binary_type bt;
    uint32_t sz;
    const uint8_t* ptr = lj::bson::as_binary(doc.root["uuid"], &bt, &sz);
    
    lj::Uuid uuid2(ptr);
    
    TEST_ASSERT(uuid == uuid2);
}
void testEscape_path()
{
    const std::string input("annoying/path");
    const std::string expected("annoying\\/path");
    std::string output(lj::bson::escape_path(input));
    
    TEST_ASSERT(expected.compare(output) == 0);
}
void testIncrement()
{
    sample_doc doc;
    
    TEST_ASSERT(lj::bson::as_uint64(doc.root["uint"]) == 0xFF77777777ULL);
    lj::bson::increment(doc.root["uint"], 8);
    TEST_ASSERT(lj::bson::as_uint64(doc.root["uint"]) == 0xFF7777777FULL);
    
}
void testBinary_type_string()
{
    TEST_ASSERT(lj::bson::binary_type_string(lj::bson::Binary_type::k_bin_generic).compare("generic") == 0);
    TEST_ASSERT(lj::bson::binary_type_string(lj::bson::Binary_type::k_bin_function).compare("function") == 0);
    TEST_ASSERT(lj::bson::binary_type_string(lj::bson::Binary_type::k_bin_binary).compare("binary (old)") == 0);
    TEST_ASSERT(lj::bson::binary_type_string(lj::bson::Binary_type::k_bin_uuid).compare("uuid") == 0);
    TEST_ASSERT(lj::bson::binary_type_string(lj::bson::Binary_type::k_bin_md5).compare("md5") == 0);
    TEST_ASSERT(lj::bson::binary_type_string(lj::bson::Binary_type::k_bin_user_defined).compare("user-defined") == 0);
}
void testType_string()
{
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_document).compare("document") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_array).compare("array") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_int32).compare("int32") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_int64).compare("int64") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_timestamp).compare("timestamp") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_double).compare("double") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_boolean).compare("boolean") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_null).compare("null") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_string).compare("string") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_binary).compare("binary") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_binary_document).compare("binary-document") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_datetime).compare("unknown") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_javascript).compare("unknown") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_minkey).compare("unknown") == 0);
    TEST_ASSERT(lj::bson::type_string(lj::bson::Type::k_maxkey).compare("unknown") == 0);
}
void testType_is_native()
{
    // native types.
    TEST_ASSERT(lj::bson::type_is_native(lj::bson::Type::k_int32));
    TEST_ASSERT(lj::bson::type_is_native(lj::bson::Type::k_int64));
    TEST_ASSERT(lj::bson::type_is_native(lj::bson::Type::k_timestamp));
    TEST_ASSERT(lj::bson::type_is_native(lj::bson::Type::k_double));
    TEST_ASSERT(lj::bson::type_is_native(lj::bson::Type::k_boolean));
    TEST_ASSERT(lj::bson::type_is_native(lj::bson::Type::k_null));
    
    // Non-native types.
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_string));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_document));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_array));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_binary));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_binary_document));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_datetime));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_javascript));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_minkey));
    TEST_ASSERT(!lj::bson::type_is_native(lj::bson::Type::k_maxkey));
}
void testType_is_nested()
{
    // nested types.
    TEST_ASSERT(lj::bson::type_is_nested(lj::bson::Type::k_document));
    TEST_ASSERT(lj::bson::type_is_nested(lj::bson::Type::k_array));
    
    // Non-nested types.
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_int32));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_int64));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_timestamp));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_double));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_boolean));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_null));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_string));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_binary));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_binary_document));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_datetime));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_javascript));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_minkey));
    TEST_ASSERT(!lj::bson::type_is_nested(lj::bson::Type::k_maxkey));
}
void testType_is_number()
{
    // number types.
    TEST_ASSERT(lj::bson::type_is_number(lj::bson::Type::k_int32));
    TEST_ASSERT(lj::bson::type_is_number(lj::bson::Type::k_int64));
    TEST_ASSERT(lj::bson::type_is_number(lj::bson::Type::k_timestamp));
    TEST_ASSERT(lj::bson::type_is_number(lj::bson::Type::k_double));
    
    // Non-number types.
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_document));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_array));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_boolean));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_null));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_string));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_binary));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_binary_document));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_datetime));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_javascript));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_minkey));
    TEST_ASSERT(!lj::bson::type_is_number(lj::bson::Type::k_maxkey));
}
void testType_is_quotable()
{
    // nested types.
    TEST_ASSERT(lj::bson::type_is_quotable(lj::bson::Type::k_string));
    
    // Non-nested types.
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_document));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_array));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_int32));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_int64));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_timestamp));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_double));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_boolean));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_null));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_binary));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_binary_document));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_datetime));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_javascript));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_minkey));
    TEST_ASSERT(!lj::bson::type_is_quotable(lj::bson::Type::k_maxkey));
}
void testType_is_value()
{
    // value types.
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_int32));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_int64));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_timestamp));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_double));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_boolean));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_null));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_string));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_binary));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_binary_document));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_datetime));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_javascript));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_minkey));
    TEST_ASSERT(lj::bson::type_is_value(lj::bson::Type::k_maxkey));
    
    // non-value types.
    TEST_ASSERT(!lj::bson::type_is_value(lj::bson::Type::k_document));
    TEST_ASSERT(!lj::bson::type_is_value(lj::bson::Type::k_array));
}
void testType_min_size()
{
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_document) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_array) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_int32) == 4);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_int64) == 8);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_timestamp) == 8);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_double) == 8);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_boolean) == 1);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_null) == 0);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_string) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_binary) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_binary_document) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_datetime) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_javascript) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_minkey) == 5);
    TEST_ASSERT(lj::bson::type_min_size(lj::bson::Type::k_maxkey) == 5);
}
int main(int argc, char** argv)
{
    return Test_util::runner("lj::bson", tests);
}

