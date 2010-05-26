
#include "lj/Bson.h"
#include "lj/lunar.h"
#include "lj/Storage.h"

#include <map>

namespace logjamd {
    void register_logjam_functions(lua_State *L);
    
    int connection_config_load(lua_State* L);
    int connection_config_save(lua_State* L);
    int connection_config_add_default_storage(lua_State* L);
    int connection_config_remove_default_storage(lua_State* L);
    
    //! Create a new lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     New configuration is populated with some default fields.
     \par
     Pops the storage name (lua string) off the stack.
     \par
     Pushes the new Lua_bson object onto the stack.
     \param L The lua state.
     \returns 1.
     */
    int storage_config_new(lua_State* L);
    
    //! Save a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Saves the configuration for the provided storage name.
     \par
     Pops the storage configuration document (Lua_bson) off the stack.
     \par
     Pops the storage name (lua string) off the stack.
     \param L The lua state.
     \returns 0
     */
    int storage_config_save(lua_State* L);
    
    //! Load a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Loads the configuration for the provided storage name.
     \par
     Pops the storage name (lua string) off the stack.
     \par
     Pushes the new Lua_bson object onto the stack.
     \param L The lua state.
     \returns 1.
     */
    int storage_config_load(lua_State* L);
    
    //! Add an index to a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Adds an index to the Configuration document.
     \par
     Pops the index comparison type (lua string) off the stack.
     \par
     Pops the field name to index (lua string) off the stack.
     \par
     Pops the name of the index (lua string) off the stack.
     \par
     Pops the index type off (lua string) off the stack.
     \par
     Pops the Storage configuration document (Lua_bson) off the stack.
     \param L The lua state.
     \returns 0.
     */
    int storage_config_add_index(lua_State* L);
    
    //! Add a nested field marker to a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Notifies the server that the provided field should index its children.
     \par
     Pops the field name to index (lua string) off the stack.
     \par
     Pops the Storage configuration document (Lua_bson) off the stack.
     \param L The lua state.
     \returns 0.
     */
    int storage_config_add_nested_field(lua_State* L);
    
    //! Send a response.
    /*!
     \par
     Populates the item field on the response.
     \par
     Expects a Lua_record_set object on top of the stack.
     \param L The lua state.
     \return 0
     */
    int send_response(lua_State* L);
};