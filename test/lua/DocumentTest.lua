-- Document Testing.

-- construct
doc = Document:new()
doc:set("test", Bson:new('"foobar"'))
doc:set("str", Bson:new('"Original foo"'))
doc:set("bool/true", Bson:new('true'))
doc:set("bool/false", Bson:new('false'))

ASSERT(doc:dirty() == true)
ASSERT(doc:id() == Uuid:new(nil))
ASSERT(doc:key() == 0)
ASSERT(doc:parent() == Uuid:new(nil))
ASSERT(doc:vclock():as_string() == "{}")
ASSERT(doc:suppress() == false)

-- rekey the document.
doc:rekey(100)
ASSERT(doc:dirty() == true)
ASSERT(doc:id() ~= Uuid:new(nil))
ASSERT(doc:key() == 100)
ASSERT(doc:parent() == Uuid:new(nil))
ASSERT(doc:vclock():as_string() == "{}")
ASSERT(doc:suppress() == false)

-- mark the document as not dirty.
doc:wash()
ASSERT(doc:dirty() == false)
ASSERT(doc:id() ~= Uuid:new(nil))
ASSERT(doc:key() == 100)
ASSERT(doc:parent() == Uuid:new(nil))
ASSERT(doc:vclock():as_string() == "{}")
ASSERT(doc:suppress() == false)

-- branch the document (copy and rekey)
doc2 = doc:branch();
ASSERT(doc2:dirty() == true)
ASSERT(doc2:id() ~= doc:id())
ASSERT(doc2:key() == doc:key())
ASSERT(doc2:parent() == doc:id())
ASSERT(doc2:vclock():as_string() == '{"{00000000-0000-0000-0000-000000000000}":1}')
ASSERT(doc2:suppress() == false)

-- branch the document
doc2 = doc:branch(200);
ASSERT(doc2:dirty() == true)
ASSERT(doc2:id() ~= doc:id())
ASSERT(doc2:key() ~= doc:key())
ASSERT(doc2:key() == 200)
ASSERT(doc2:parent() == doc:id())
ASSERT(doc2:vclock():as_string() == '{}')
ASSERT(doc2:suppress() == false)

-- encrypt the document
ASSERT(doc2:exists('str') == true)
ASSERT(doc2:exists('bool/false') == true)
doc2:encrypt('test', 'str', 'bool/false')
ASSERT(doc2:exists('str') == false)
ASSERT(doc2:exists('bool/false') == false)
doc2:decrypt('test')
ASSERT(doc2:exists('str') == true)
ASSERT(doc2:exists('bool/false') == true)
