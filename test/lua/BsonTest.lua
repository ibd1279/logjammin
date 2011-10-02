-- Command_language_lua provides three methods.

-- construct
obj1 = Bson:new()
ASSERT(obj1:as_string() == "{}")

-- test all the setters
obj1:set_null("test/null")
obj1:set_array("test/array")
obj1:set_boolean("test/bool/false", false)
obj1:set_boolean("test/bool/true", true)
obj1:set_string("test/string", "a test string")
obj1:set_int32("test/int32", 2147483640)
obj1:set_int64("test/int64", 4294967300)
obj1:set_uuid("test/uuid", Uuid:new("{100f0e0d-0c0b-0a09-0807-060504030201}"))

-- test as_string
ASSERT(obj1:as_string() == '{"test":{"array":[], "bool":{"false":0, "true":1}, "int32":2147483640, "int64":4294967300, "null":null, "string":"a test string", "uuid":{100f0e0d-0c0b-0a09-0807-060504030201}/1157159078456959122}}')

-- test tostring
expected = [=[{
  "test":{
    "array":[],
    "bool":{
      "false":0,
      "true":1
    },
    "int32":2147483640,
    "int64":4294967300,
    "null":null,
    "string":"a test string",
    "uuid":"{100f0e0d-0c0b-0a09-0807-060504030201}/1157159078456959122"
  }
}]=]
print(obj1)
ASSERT(RESPONSE:path("output/0"):as_string() == expected)
result = tostring(obj1)
ASSERT(result == expected)

-- test type and path
ASSERT(obj1:type() == 'document')
ASSERT(obj1:path('test/null'):type() == 'null')
ASSERT(obj1:path('test/array'):type() == 'array')
ASSERT(obj1:path('test/bool/false'):type() == 'boolean')
ASSERT(obj1:path('test/string'):type() == 'string')
ASSERT(obj1:path('test/int32'):type() == 'int32')
ASSERT(obj1:path('test/int64'):type() == 'int64')
ASSERT(obj1:path('test/uuid'):type() == 'binary')

-- test clone, path, and nullify
obj2 = obj1:clone()
ASSERT(obj2:type() == 'document')
ASSERT(obj2:as_string() == '{"test":{"array":[], "bool":{"false":0, "true":1}, "int32":2147483640, "int64":4294967300, "null":null, "string":"a test string", "uuid":{100f0e0d-0c0b-0a09-0807-060504030201}/1157159078456959122}}')
obj2:nullify()
ASSERT(obj2:type() == 'null')
ASSERT(obj2:as_string() == 'null')

obj1:set_string('test/null', 'Delete me')
obj2 = obj1:path('test/null')
ASSERT(obj2:as_string() == 'Delete me')
ASSERT(tostring(obj1) ~= expected)
obj2:nullify()
ASSERT(obj2:as_string() == 'null')
ASSERT(tostring(obj1) == expected)

-- test index
expected = [=[{
  "array":[],
  "bool":{
    "false":0,
    "true":1
  },
  "int32":2147483640,
  "int64":4294967300,
  "null":null,
  "string":"a test string",
  "uuid":"{100f0e0d-0c0b-0a09-0807-060504030201}/1157159078456959122"
}]=]
ASSERT(tostring(obj1['test']) == expected)
ASSERT(tostring(obj1.test) == expected);

-- test read-only and most of the "as_*" methods.
obj3 = obj1:clone_immutable('test')
ASSERT(tostring(obj3) == expected)
u1 = Uuid:new("{100f0e0d-0c0b-0a09-0807-060504030201}")
ASSERT(obj3.uuid:as_uuid() == u1)
ASSERT(obj3.int64:as_number() == 4294967300)
ASSERT(obj3.int32:as_number() == 2147483640)
ASSERT(obj3.bool['false']:as_boolean() == false)
ASSERT(obj3:as_nil() == nil)

-- test the constructor
obj4 = Bson:new(obj1.test)
ASSERT(tostring(obj4) == expected)
obj4:set_string('string', 'another test string')
ASSERT(tostring(obj4) ~= expected)
obj4 = Bson_ro:new(obj4)
ASSERT(tostring(obj4) ~= expected)
obj5 = Bson:new('{"foo":"testing","bar":100}')
ASSERT(obj5:as_string() == '{"bar":100, "foo":"testing"}')
