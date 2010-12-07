lj_storage_init('role')
lj_storage_index('role', 'name', 'hash', 'lex')
lj_storage_index('role', 'name', 'text', 'lex')
lj_storage_index('role', 'allowed', 'tag', 'lex')
lj_storage_index('role', 'create/by', 'tree', 'int64')

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

send_set(db.role:none():contains('name', 'role 100'):mode_and():tagged('allowed', 'action/1000'))
