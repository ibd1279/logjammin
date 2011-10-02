-- Command_language_lua provides three methods.

-- construct and lots of tostring testing and key.
obj1 = Uuid:new()
ASSERT(tostring(obj1):len() == 38)
obj2 = Uuid:new(nil)
ASSERT(tostring(obj2) == '{00000000-0000-0000-0000-000000000000}')
obj3 = Uuid:new(12345)
ASSERT(tostring(obj3):len() == 38)
ASSERT(obj3:key() == 12345)
obj4 = Uuid:new('{0fabdfc8-282e-4dca-80cc-48d8e0f26572}')
ASSERT(tostring(obj4) == '{0fabdfc8-282e-4dca-80cc-48d8e0f26572}')
obj5 = Uuid:new(obj4)
ASSERT(tostring(obj4) == tostring(obj5))

-- equality testing.
obj6 = Uuid:new(1000000)
obj7 = Uuid:new(2000000)

ASSERT(obj6 == obj6)
ASSERT(obj6 < obj7)
ASSERT(obj6 <= obj6)
ASSERT(not (obj6 < obj6))
ASSERT(not (obj7 < obj6))
ASSERT(not (obj7 <= obj6))
ASSERT(obj7 > obj6)
ASSERT(obj6 >= obj6)
ASSERT(not (obj6 > obj6))
ASSERT(not (obj6 > obj7))
ASSERT(not (obj6 >= obj7))
