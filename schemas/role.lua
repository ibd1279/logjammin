role_cfg = sc_new("role")
sc_add_index(role_cfg, "hash", "name", "name", "lex")
sc_add_index(role_cfg, "tree", "allowed", "allowed", "lex")
sc_add_index(role_cfg, "text", "allowed", "allowed", "lex")
sc_add_index(role_cfg, "text", "name", "name", "lex")
sc_add_index(role_cfg, "tag", "allowed", "allowed", "lex")
sc_add_index(role_cfg, "tag", "name", "name", "lex")
sc_add_unique(role_cfg, "allowed")
sc_save("role", role_cfg)
print(role_cfg)

role_default = Bson:new()
role_default:nav("name"):set("default")
role_default:nav("allowed"):push("auth/login")
role_default:nav("allowed"):push("auth/reset_password")
role_default:nav("allowed"):push("project/view")
print(role_default)

role_leader = Bson:new()
role_leader:nav("name"):set("leader")
role_leader:nav("allowed"):push("auth/login")
role_leader:nav("allowed"):push("auth/reset_password")
role_leader:nav("allowed"):push("project/view")
role_leader:nav("allowed"):push("project/edit")
role_leader:nav("allowed"):push("backog/view")
role_leader:nav("allowed"):push("backog/edit")

role_admin = Bson:new()
role_admin:nav("name"):set("admin")
role_admin:nav("allowed/0"):set("auth/login")
role_admin:nav("allowed/1"):set("auth/reset_password")
role_admin:nav("allowed/2"):set("project/view")
role_admin:nav("allowed/3"):set("project/edit")
role_admin:nav("allowed/4"):set("backog/view")
role_admin:nav("allowed/5"):set("backog/edit")
role_admin:nav("allowed/6"):set("user/view")
role_admin:nav("allowed/7"):set("user/edit")

role = Storage:new("role")
role:place(role_default)
role:place(role_admin)
role:place(role_leader)

r1 = role:all():filter("name", "default"):first()
r2 = role:all():filter("name", "admin"):first()
r3 = role:all():filter("name", "first"):first()
r4 = role:all():filter("name", "leader"):first();

print(r1)
print(r2)
print(r3)
print(r4)

for h, r in ipairs(role:all():records()) do print(r) end
for h, r in ipairs(role:all():tagged("allowed", "auth/login"):records()) do print(r) end
for h, r in ipairs(role:all():tagged("allowed", "auth/login"):filter("name", "admin"):records()) do print(r) end
for h, r in ipairs(role:all():tagged("allowed", "auth/login"):filter("name", "admin"):filter("name", "default"):records()) do print(r) end
for h, r in ipairs(role:all():tagged("allowed", "user/edit"):records()) do print(r) end

role:remove(role_leader);
r4 = role:all():filter("name", "leader"):first();
print(r4)
