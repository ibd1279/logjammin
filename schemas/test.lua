test_cfg = sc_new("test")
sc_add_index(test_cfg, "hash", "name", "name", "lex")
sc_add_index(test_cfg, "tree", "allowed", "allowed", "lex")
sc_add_index(test_cfg, "text", "allowed", "allowed", "lex")
sc_add_index(test_cfg, "text", "name", "name", "lex")
sc_add_index(test_cfg, "tag", "allowed", "allowed", "lex")
sc_add_index(test_cfg, "tag", "name", "name", "lex")
sc_add_nested(test_cfg, "allowed")
sc_save("test", test_cfg)
print(test_cfg)

test_default = Bson:new()
test_default:nav("name"):set("default")
test_default:nav("allowed"):push("auth/login")
test_default:nav("allowed"):push("auth/reset_password")
test_default:nav("allowed"):push("project/view")
print(test_default)

test_leader = Bson:new()
test_leader:nav("name"):set("leader")
test_leader:nav("allowed"):push("auth/login")
test_leader:nav("allowed"):push("auth/reset_password")
test_leader:nav("allowed"):push("project/view")
test_leader:nav("allowed"):push("project/edit")
test_leader:nav("allowed"):push("backog/view")
test_leader:nav("allowed"):push("backog/edit")

test_admin = Bson:new()
test_admin:nav("name"):set("admin")
test_admin:nav("allowed"):push("auth/login")
test_admin:nav("allowed"):push("auth/reset_password")
test_admin:nav("allowed"):push("project/view")
test_admin:nav("allowed"):push("project/edit")
test_admin:nav("allowed"):push("backog/view")
test_admin:nav("allowed"):push("backog/edit")
test_admin:nav("allowed"):push("user/view")
test_admin:nav("allowed"):push("user/edit")

test = Storage:new("test")
test:place(test_default)
test:place(test_admin)
test:place(test_leader)

r1 = test:all():filter("name", "default"):first()
r2 = test:all():filter("name", "admin"):first()
r3 = test:all():filter("name", "first"):first()
r4 = test:all():filter("name", "leader"):first();

print(r1)
print(r2)
print(r3)
print(r4)

for h, r in ipairs(test:all():records()) do print(r) end
for h, r in ipairs(test:all():tagged("allowed", "auth/login"):records()) do print(r) end
for h, r in ipairs(test:all():tagged("allowed", "auth/login"):filter("name", "admin"):records()) do print(r) end
for h, r in ipairs(test:all():tagged("allowed", "auth/login"):filter("name", "admin"):filter("name", "default"):records()) do print(r) end
for h, r in ipairs(test:all():tagged("allowed", "user/edit"):records()) do print(r) end

test:remove(test_leader);
r4 = test:all():filter("name", "leader"):first();
print(r4)
