-- Document Testing.

-- construct
doc = Document:new()
doc:set("test", Bson:new('"foobar"'))
ASSERT(doc:dirty() == true)
ASSERT(doc:id() == Uuid:new(nil))
ASSERT(doc:key() == 0)
ASSERT(doc:parent() == Uuid:new(nil))
ASSERT(doc:vclock():as_string() == "{}")
ASSERT(doc:suppress() == false)
ASSERT(doc:encrypted() == false)

-- rekey the document.
doc:rekey(100)
ASSERT(doc:dirty() == true)
ASSERT(doc:id() ~= Uuid:new(nil))
ASSERT(doc:key() == 100)
ASSERT(doc:parent() == Uuid:new(nil))
ASSERT(doc:vclock():as_string() == "{}")
ASSERT(doc:suppress() == false)
ASSERT(doc:encrypted() == false)

-- mark the document as not dirty.
doc:wash()
ASSERT(doc:dirty() == false)
ASSERT(doc:id() ~= Uuid:new(nil))
ASSERT(doc:key() == 100)
ASSERT(doc:parent() == Uuid:new(nil))
ASSERT(doc:vclock():as_string() == "{}")
ASSERT(doc:suppress() == false)
ASSERT(doc:encrypted() == false)

-- branch the document (copy and rekey)
doc2 = doc:branch();
ASSERT(doc2:dirty() == true)
ASSERT(doc2:id() ~= doc:id())
ASSERT(doc2:key() == doc:key())
ASSERT(doc2:parent() == doc:id())
ASSERT(doc2:vclock():as_string() == '{"{00000000-0000-0000-0000-000000000000}":1}')
ASSERT(doc2:suppress() == false)
ASSERT(doc2:encrypted() == false)

-- branch the document
doc2 = doc:branch(200);
ASSERT(doc2:dirty() == true)
ASSERT(doc2:id() ~= doc:id())
ASSERT(doc2:key() ~= doc:key())
ASSERT(doc2:key() == 200)
ASSERT(doc2:parent() == doc:id())
ASSERT(doc2:vclock():as_string() == '{}')
ASSERT(doc2:suppress() == false)
ASSERT(doc2:encrypted() == false)

-- encrypt the document
doc2:encrypt('test')
ASSERT(doc2:encrypted() == true)
doc2:decrypt('test')
ASSERT(doc2:encrypted() == false)
