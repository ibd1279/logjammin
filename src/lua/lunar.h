#pragma once

/*
 Taken from http://lua-users.org/wiki/SimplerCppBinding with slight modifications.
 */

#include <string>
#include "lua.hpp"

namespace lua
{

    //! Lua integration class.
    template <typename T> class Lunar {
        typedef struct {
            T *pT;
        } userdataType;
    public:
        //! Member function pointer definition.
        typedef int (T::*mfp)(lua_State *L);

        //! Method registration type.
        struct RegType
        {
            const char *name; //!< The method name in lua.
            mfp mfunc; //!< The member function pointer.
        };

        //! Register this class in the Lua state.
        /*!
         \param L The lua state.
         */
        static void Register(lua_State *L)
        {
            lua_pushglobaltable(L); // replacement for deprecated LUA_GLOBALSINDEX
            int globaltable = lua_gettop(L);
            lua_newtable(L);
            int mt = lua_gettop(L);
            lua_newtable(L);
            int methods = lua_gettop(L);
            luaL_newmetatable(L, T::LUNAR_CLASS_NAME);
            int metatable = lua_gettop(L);

            // store method table in globals so that
            // scripts can add functions written in Lua.
            lua_pushvalue(L, methods);
            set(L, globaltable, T::LUNAR_CLASS_NAME);

            // hide metatable from Lua getmetatable()
            lua_pushvalue(L, methods);
            set(L, metatable, "__metatable");

            lua_pushvalue(L, methods);
            set(L, metatable, "__index");

            lua_pushcfunction(L, tostring_T);
            set(L, metatable, "__tostring");

            lua_pushcfunction(L, gc_T);
            set(L, metatable, "__gc");

            // stack: {mM, m, M}

            lua_pushvalue(L, mt);           // mt for method table
            lua_pushcfunction(L, new_T);
            lua_pushvalue(L, -1);           // dup new_T function
            set(L, methods, "new");         // add new_T to method table
            set(L, -3, "__call");           // mt.__call = new_T
            lua_setmetatable(L, methods);

            // stack: {mM, m, M}

            // fill method table with methods from class T
            for (RegType *l = T::LUNAR_METHODS; l->name; l++) {
                std::string tmp(l->name);
                lua_pushstring(L, l->name);
                lua_pushlightuserdata(L, (void*)l);

                if (tmp.compare("__index") == 0)
                {
                    lua_pushvalue(L, methods);
                    lua_pushcclosure(L, index_T, 2);
                    lua_settable(L, metatable);
                }
                else if(tmp.substr(0, 2).compare("__") == 0)
                {
                    lua_pushcclosure(L, thunk, 1);
                    lua_settable(L, metatable);
                }
                else
                {
                    lua_pushcclosure(L, thunk, 1);
                    lua_settable(L, methods);
                }
            }

            lua_pop(L, 4);  // pop global table, metatable and method table
        }

        //! Call named lua method from userdata method table
        /*!
         \param L The lua state.
         \param method The method name to call.
         \param nargs Number of args to the function.
         \param nresults The number of results expected to be returned.
         \param errfunc The location in the stack of the error handling function.
         \return Number of results.
         */
        static int call(lua_State *L,
                const char *method,
                int nargs = 0,
                int nresults = LUA_MULTRET,
                int errfunc = 0)
        {
            int base = lua_gettop(L) - nargs;  // userdata index
            if (!luaL_checkudata(L, base, T::LUNAR_CLASS_NAME))
            {
                lua_settop(L, base - 1);           // drop userdata and args
                lua_pushfstring(L,
                        "not a valid %s userdata",
                        T::LUNAR_CLASS_NAME);
                return -1;
            }

            lua_pushstring(L, method);         // method name
            lua_gettable(L, base);             // get method from userdata
            if (lua_isnil(L, -1))              // no method?
            {
                lua_settop(L, base-1);         // drop userdata and args
                lua_pushfstring(L,
                        "%s missing method '%s'",
                        T::LUNAR_CLASS_NAME, method);
                return -1;
            }
            lua_insert(L, base);               // put method under userdata

            int status = lua_pcall(L,          // call method
                    1 + nargs,
                    nresults,
                    errfunc);

            if (status) {
                const char *msg = lua_tostring(L, -1);
                if (msg == NULL)
                {
                    msg = "(error with no message)";
                }
                lua_pushfstring(L,
                        "%s:%s status = %d\n%s",
                        T::LUNAR_CLASS_NAME,
                        method,
                        status,
                        msg);
                lua_remove(L, base);             // remove old message
                return -1;
            }
            return lua_gettop(L) - base + 1;     // number of results
        }

        //! push onto the Lua stack a userdata containing a pointer to T object
        /*!
         \param L The lua state.
         \param obj The object to push.
         \param gc True if Lua should delete the pointer, false otherwise.
         \return index of the userdata on the stack.
         */
        static int push(lua_State *L,
                T *obj,
                bool gc = false)
        {
            // handle null
            if (!obj)
            {
                lua_pushnil(L);
                return 0;
            }

            // lookup metatable in the Lua registry
            luaL_getmetatable(L, T::LUNAR_CLASS_NAME);
            if (lua_isnil(L, -1))
            {
                luaL_error(L,
                        "%s missing metatable",
                        T::LUNAR_CLASS_NAME);
            }
            int mt = lua_gettop(L);
            subtable(L, mt, "userdata", "v");
            void* ptr = pushuserdata(L, obj, sizeof(userdataType));
            userdataType *ud = static_cast<userdataType*>(ptr);

            if (ud)
            {
                ud->pT = obj;  // store pointer to object in userdata
                lua_pushvalue(L, mt);
                lua_setmetatable(L, -2);
                if (gc == false)
                {
                    lua_checkstack(L, 3);
                    subtable(L, mt, "do not trash", "k");
                    lua_pushvalue(L, -2);
                    lua_pushboolean(L, 1);
                    lua_settable(L, -3);
                    lua_pop(L, 1);
                }
            }
            lua_replace(L, mt);
            lua_settop(L, mt);
            return mt;  // index of userdata containing pointer to T object
        }

        //! get userdata from Lua stack and return pointer to T object
        /*!
         \param L The lua state.
         \param narg The location in the stack to check the type of.
         \return The pointer from the stack.
         */
        static T *check(lua_State *L, int narg)
        {
            void* ptr = luaL_checkudata(L, narg, T::LUNAR_CLASS_NAME);
            userdataType *ud = static_cast<userdataType*>(ptr);
            if(!ud) {
                // Where did we break.
                luaL_where(L, 0);
                std::string where(lua_tostring(L, -1));
                lua_pop(L, 1);

                // Get the provided type info.
                int received_type = lua_type(L, narg);
                const char* received_type_name = lua_typename(L, received_type);

                lua_pushfstring(L, "%s: Expected type %s, but got type %s.",
                        where.c_str(),
                        T::LUNAR_CLASS_NAME,
                        received_type_name);
            }
            return ud->pT;  // pointer to T object
        }

    private:
        static int index_T(lua_State *L)
        {
            // [obj, key]
            lua_pushvalue(L, -1); // [obj, key, key]
            lua_gettable(L, lua_upvalueindex(2)); // [obj, key, method]
            if (lua_isnil(L, -1)) // no method?
            {
                lua_pop(L, 1); // [obj, key]
                return thunk(L);
            }
            lua_insert(L, 1); // [method, obj, key]
            lua_pop(L, 2); // [method]
            return 1;
        }

        Lunar() = delete;  // hide default constructor

        // member function dispatcher
        static int thunk(lua_State *L)
        {
            // stack has userdata, followed by method args
            T *obj = check(L, 1);  // get 'self', or if you prefer, 'this'
            lua_remove(L, 1);  // remove self so member function args start at index 1
            // get member function from upvalue
            RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));
            return (obj->*(l->mfunc))(L);
        }

        // create a new T object and
        // push onto the Lua stack a userdata containing a pointer to T object
        static int new_T(lua_State *L)
        {
            lua_remove(L, 1);   // use classname:new(), instead of classname.new()
            T *obj = new T(L);  // call constructor for T objects
            push(L, obj, true); // gc_T will delete this object
            return 1;           // userdata containing pointer to T object
        }

        // garbage collection metamethod
        static int gc_T(lua_State *L)
        {
            if (luaL_getmetafield(L, 1, "do not trash"))
            {
                lua_pushvalue(L, 1);  // dup userdata
                lua_gettable(L, -2);
                if (!lua_isnil(L, -1)) return 0;  // do not delete object
            }
            void* ptr = lua_touserdata(L, 1);
            userdataType *ud = static_cast<userdataType*>(ptr);
            if (ud->pT)
            {
                // call destructor for T objects
                delete ud->pT;
                ud->pT = NULL;
            }
            return 0;
        }

        static int tostring_T (lua_State *L)
        {
            char buff[32];
            void* ptr = lua_touserdata(L, 1);
            userdataType *ud = static_cast<userdataType*>(ptr);
            sprintf(buff, "%p", (void*)ud->pT);
            lua_pushfstring(L, "%s (%s)", T::LUNAR_CLASS_NAME, buff);
            return 1;
        }

        static void set(lua_State *L, int table_index, const char *key)
        {
            lua_pushstring(L, key);
            lua_insert(L, -2);  // swap value and key
            lua_settable(L, table_index);
        }

        static void weaktable(lua_State *L, const char *mode)
        {
            lua_newtable(L);
            lua_pushvalue(L, -1);  // table is its own metatable
            lua_setmetatable(L, -2);
            lua_pushliteral(L, "__mode");
            lua_pushstring(L, mode);
            lua_settable(L, -3);   // metatable.__mode = mode
        }

        static void subtable(lua_State *L,
                int tindex,
                const char *name,
                const char *mode)
        {
            lua_pushstring(L, name);
            lua_gettable(L, tindex);
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);
                lua_checkstack(L, 3);
                weaktable(L, mode);
                lua_pushstring(L, name);
                lua_pushvalue(L, -2);
                lua_settable(L, tindex);
            }
        }

        static void *pushuserdata(lua_State *L, void *key, size_t sz)
        {
            void *ud = 0;
            lua_pushlightuserdata(L, key);
            lua_gettable(L, -2);     // lookup[key]
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);         // drop nil
                lua_checkstack(L, 3);
                ud = lua_newuserdata(L, sz);  // create new userdata
                lua_pushlightuserdata(L, key);
                lua_pushvalue(L, -2);  // dup userdata
                lua_settable(L, -4);   // lookup[key] = userdata
            }
            return ud;
        }
    };

    //! Convert a lua stack position into a c++ string.
    /*!
     \param L The lua state.
     \param offset The position in the stack.
     \return The C++ String.
     */
    inline std::string as_string(lua_State* L, int offset)
    {
        const char* ptr = luaL_checkstring(L, offset);
        if (!ptr)
        {
            return std::string();
        }
        size_t l = lua_rawlen(L, offset);
        return std::string(ptr, l);
    }
}; // namespace lua

#define LUNAR_METHOD(Class, Name) {#Name, &Class::Name}
