
#include <lua.h>                               /* Always include this */
#include <lauxlib.h>                           /* Always include this */
#include <lualib.h>                            /* Always include this */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_error.h>
#include <aerospike/as_key.h>
#include <aerospike/as_record.h>
#include <aerospike/as_record_iterator.h>
#include <aerospike/as_iterator.h>
#include <aerospike/as_status.h>
#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_list.h>
#include <aerospike/as_map.h>


static as_record add_bins_to_rec(lua_State *L, int index, int numBins)
{
       as_record rec;
       as_record_init(&rec, numBins);

    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, index);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2))
    {
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        const char *binName = lua_tostring(L, -1);

        // add to record
        if (lua_isnumber(L, -2)){
        	int intValue = lua_tointeger(L, -2);
        	as_record_set_int64(&rec, binName, intValue);

        } else if (lua_isstring(L, -2)){
        	const char *value = lua_tostring(L, -2);
        	as_record_set_str(&rec, binName, value);
        } else if (lua_istable(L, -2)){
	    	// make a as_list and populate it
        	as_arraylist *list = as_arraylist_new(3, 3);
            
        	lua_pushvalue(L, -2);
        	lua_pushnil(L);
        	    // This is needed for it to even get the first value
        	    while (lua_next(L, -2))
        	    {
        	    	lua_pushvalue(L, -2);
        	    	//const char *key = lua_tostring(L, -1);
        	    	const char *value = lua_tostring(L, -2);
        	    	// populate the as_list
        	    	as_arraylist_append_str(list, value);
        	    	//printf("%s => %s\n", key, value);
        	        lua_pop(L, 2);
        	    }
        	lua_pop(L, 1);
            
	    	// put the list in a bin
        	as_record_set_list(&rec, binName, (as_list*)as_val_reserve(list));
        }
        // pop value + copy of key, leaving original key
        lua_pop(L, 2);
        // stack now contains: -1 => key; -2 => table
    }

    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
    return rec;
}



static as_operations add_bins_to_increment(lua_State *L, int index, int numBins)
{
	as_operations ops;
	as_operations_init(&ops, numBins);

    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, index);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2))
    {
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        const char *binName = lua_tostring(L, -1);
        int intValue = lua_tointeger(L, -2);
        
        //printf("Bin:%s, value:%d\n", binName, intValue);
        
    	//add an operation for each bin
    	as_operations_add_incr(&ops, binName, intValue);
        // pop value + copy of key, leaving original key
        lua_pop(L, 2);
        // stack now contains: -1 => key; -2 => table
    }

    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
    return ops;
}


static int connect(lua_State *L){
	const char *hostName = luaL_checkstring(L, 1);
	int port = lua_tointeger(L, 2);
	as_error err;

	// Configuration for the client.
	as_config config;
	as_config_init(&config);

	// Add a seed host for cluster discovery.
	config.hosts[0].addr = hostName;
	config.hosts[0].port = port;

	// The Aerospike client instance, initialized with the configuration.
	aerospike as;
	aerospike_init(&as, &config);

	// Connect to the cluster.
	aerospike_connect(&as, &err);

	/* Push the return */
	lua_pushnumber(L, err.code);
	lua_pushstring(L, err.message);
	lua_pushlightuserdata(L, &as);
	return 3;
}

static int disconnect(lua_State *L){
	aerospike* as = lua_touserdata(L, 1);
	as_error err;
	aerospike_close(as, &err);
	lua_pushnumber(L, err.code);
	lua_pushstring(L, err.message);
	return 2;
}
static int get(lua_State *L){
	//printf("-get-\n");
	aerospike* as = lua_touserdata(L, 1);
	const char* nameSpace = luaL_checkstring(L, 2);
	const char* set = luaL_checkstring(L, 3);
	const char* keyString = luaL_checkstring(L, 4);
	//printf("key-:%s\n", keyString);
	as_record* rec = NULL;
	as_key key;
	as_error err;
	as_key_init(&key, nameSpace, set, keyString);

	// Read the test record from the database.
	aerospike_key_get(as, &err, NULL, &key, &rec);

	// Push the error code
	lua_pushnumber(L, err.code);

	// Push the error message
	lua_pushstring(L, err.message);

	// Create an new table and push it
	if ( err.code == AEROSPIKE_OK){
        
		lua_newtable(L); /* create table to hold Bins read */
		/*
		 * iterate through bin and add the bin name
		 * and value to the table
		 */
		as_record_iterator it;
		as_record_iterator_init(&it, rec);

		while (as_record_iterator_has_next(&it)) {
		    as_bin *bin        = as_record_iterator_next(&it);
		    as_val *value      = (as_val*)as_bin_get_value(bin);
            char * binName = as_bin_get_name(bin);
            
		    int bin_type = as_val_type(value); //Bin Type

		    switch (bin_type){
		    case AS_INTEGER:
                   
		    	//printf("--integer-%s-\n", binName);
			    lua_pushstring(L, binName); //Bin name
		    	lua_pushnumber(L, as_integer_get(as_integer_fromval(value)));
		    	//printf("--integer-end-\n");
		    	break;
		    case AS_DOUBLE:
                   
		    	//printf("--double-%s-\n", binName);
			    lua_pushstring(L, binName); //Bin name
		    	lua_pushnumber(L, as_double_get(as_double_fromval(value)));
		    	//printf("--double-end-\n");
		    	break;
		    case AS_STRING:
		    	//printf("--string-%s-\n", binName);
			    lua_pushstring(L, binName); //Bin name
		    	lua_pushstring(L, as_val_tostring(value));
		    	//printf("--string-end-\n");
		    	break;
		    case AS_LIST:
		    	//printf("--list-%s-\n", binName);
			    lua_pushstring(L, binName); //Bin name
		    	// Iterate through arraylist populating table
		    	as_list* p_list = as_list_fromval(value);
		    	as_arraylist_iterator it;
		    	as_arraylist_iterator_init(&it, (const as_arraylist*)p_list);
                    
                // create a Lua inner table table for the "List"
		    	lua_newtable(L);
                    
		    	int count = 0;
		    	// See if the elements match what we expect.
		    	while (as_arraylist_iterator_has_next(&it)) {
		    		const as_val* p_val = as_arraylist_iterator_next(&it);
		    		//Assume string
		    		char* p_str = as_val_tostring(p_val);
                    lua_pushnumber(L, count); // table[i]
			    	lua_pushstring(L, p_str); //Value
                    //printf("%d => %s\n", count, p_str);
			    	count++;
			    	lua_settable(L, -3);
		    	}
                //printf("--list-end-\n");
                break;
		    }
		    //printf("--settable-\n");
		    lua_settable(L, -3);
		    //printf("--settable-end-\n");
		}
	}
	as_record_destroy(rec);
	as_key_destroy(&key);
	//printf("-get-end-\n");
	return 3;
}



static int put(lua_State *L){

	//Cluster
	aerospike* as = lua_touserdata(L, 1);

	//Namespace
	const char* nameSpace = luaL_checkstring(L, 2);

	//Set
	const char* set = luaL_checkstring(L, 3);

	//Key as string
	const char* keyString = luaL_checkstring(L, 4);

	// Number of bins.
	const int numBins = lua_tointeger(L, 5);

	//Bins
	as_record rec = add_bins_to_rec(L, 6, numBins);

	//const as_record * test = &rec;
	//if (as_val_type(as_record_get(test, "animals")) == AS_LIST)
	//	printf("correct list\n");
	//else
	//	printf("not a list\n");

	// Create key
	as_key key;
	as_error err;
	as_key_init(&key, nameSpace, set, keyString);

	// Write record
	aerospike_key_put(as, &err, NULL, &key, &rec);
	as_key_destroy(&key);
    as_record_destroy(&rec);
	// Return status
	lua_pushnumber(L, err.code);
	lua_pushstring(L, err.message);
	return 2;

}


static int increment(lua_State *L){
	as_error err;
	aerospike* as = lua_touserdata(L, 1);
	const char* nameSpace = luaL_checkstring(L, 2);
	const char* set = luaL_checkstring(L, 3);
	const char* keyString = luaL_checkstring(L, 4);
	const int numBins = lua_tointeger(L, 5);

	as_operations ops = add_bins_to_increment(L, 6, numBins);

	as_key key;
	as_key_init(&key, nameSpace, set, keyString);
	// Apply the operations. Since the record does not exist, it will be created
	// and the bins initialized with the ops' integer values.
	aerospike_key_operate(as, &err, NULL, &key, &ops, NULL);

	as_operations_destroy(&ops);
	as_key_destroy(&key);

	lua_pushnumber(L, err.code);
	lua_pushstring(L, err.message);
	return 2;
}

static const struct luaL_Reg as_client [] = {
		{"connect", connect},
		{"disconnect", disconnect},
		{"get", get},
		{"put", put},
		{"increment", increment},
		{NULL, NULL}
};

extern int luaopen_as_lua(lua_State *L){
	luaL_register(L, "as_lua", as_client);
	return 0;
}

