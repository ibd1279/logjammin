role = sc_new("role")
sc_add_index(role, "tree", "name", "name", "lex")
sc_add_unique(role, "name")
sc_save("role", role)
