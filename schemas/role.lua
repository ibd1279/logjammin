role_cfg = sc_new("role")
sc_add_index(role_cfg, "hash", "name", "lex")
sc_add_index(role_cfg, "text", "name", "lex")
sc_add_index(role_cfg, "tag", "allowed", "lex")
sc_add_index(role_cfg, "tree", "create_by", "int64")
sc_save("role", role_cfg)

for i = 1, 10000 do
    r = Bson:new()
    r:nav('name'):set('role ' .. i)
    r:nav('allowed'):push('action/' .. i)
    r:nav('allowed'):push('user/login')
    r:nav('create/by'):set(1)
    r:nav('create/date'):set('2010-06-15')
    local random_string = "";
    for i = 1, math.random(1025) do
        random_string = random_string .. string.char(math.random(32,126))
    end
    r:nav('description'):set(random_string)
    r:nav('insert_id'):set(i)
    db.role:place(r)
end

