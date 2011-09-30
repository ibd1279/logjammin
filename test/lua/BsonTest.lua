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
io.write(RESPONSE['output']:as_string() .. '\n')
io.write(tostring(obj1.test) .. '\n')
