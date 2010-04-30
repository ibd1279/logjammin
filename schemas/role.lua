role_cfg = sc_new("role")
sc_add_index(role_cfg, "hash", "name", "name", "lex")
sc_add_index(role_cfg, "tree", "allowed", "allowed", "lex")
sc_add_index(role_cfg, "text", "allowed", "allowed", "lex")
sc_add_index(role_cfg, "text", "name", "name", "lex")
sc_add_index(role_cfg, "tag", "allowed", "allowed", "lex")
sc_add_index(role_cfg, "tag", "name", "name", "lex")
sc_add_unique(role_cfg, "allowed")
sc_save("role", role_cfg)

role_default = BSONNode:new()
role_default:nav("name"):set("default")
role_default:nav("allowed/0"):set("auth/login")
role_default:nav("allowed/1"):set("auth/reset_password")
role_default:nav("allowed/2"):set("project/view")

role_admin = BSONNode:new()
role_admin:nav("name"):set("admin")
role_admin:nav("allowed/0"):set("auth/login")
role_admin:nav("allowed/1"):set("auth/reset_password")
role_admin:nav("allowed/2"):set("project/view")
role_admin:nav("allowed/3"):set("project/edit")
role_admin:nav("allowed/4"):set("user/view")
role_admin:nav("allowed/5"):set("user/edit")

role = Storage:new("role")
role:place(role_default)
role:place(role_admin)

r1 = role:filter("name", "default"):first()
r2 = role:filter("name", "admin"):first()
r3 = role:filter("name", "first"):first()

print(r1)
print(r2)
print(r3)

for h, r in ipairs(role:all():records()) do print(r) end
for h, r in ipairs(role:tagged("allowed", "auth/login"):records()) do print(r) end
for h, r in ipairs(role:tagged("allowed", "auth/login"):filter("name", "admin"):records()) do print(r) end
for h, r in ipairs(role:tagged("allowed", "auth/login"):filter("name", "admin"):filter("name", "default"):records()) do print(r) end
for h, r in ipairs(role:tagged("allowed", "user/edit"):records()) do print(r) end
