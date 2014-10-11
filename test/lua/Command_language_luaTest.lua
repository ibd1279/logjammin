-- Command_language_lua provides three methods.

-- print
print("hello world")
print("\"Foo Bar\"")
print("\\test\\")

-- and assert.
ASSERT(RESPONSE:path("output/0"):as_string() == "hello world")
ASSERT(RESPONSE:path("output/1"):as_string() == "\"Foo Bar\"")
ASSERT(RESPONSE:path("output/2"):as_string() == "\\test\\")
