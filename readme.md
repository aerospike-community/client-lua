#Calling Aerospike from Lua
##Problem
You are using Lua and would like to make calls to an Aerospike database. But Aerospike does not provide a Lua Client API.

You may be using Lua within application servers like Nginx, BarracudaDrive, Mako, OpenResty, Kepler, etc - similar to how Javascript is used inside Node.
##Solution
The solution is conceptually simple: Call the Aerospike C API from Lua. There is extensive Lua documentation and many books and blogs that describe calling C from Lua. This example demonstrates how to wrap the Aerospike C client API so it can be called from Lua.

The source code for this solution is available on GitHub, and the README.md 
http://github.com/aerospike/client-lua. 

The package also requires the Aerospike C client, which is available at http://github.com/aersopike/aerospike-client-c or from http://aerospike.com . You will need to build the client and have the Aerospike headers and libraries available in the search path.

In this example we will wrap 5 basic Aerospike key value operations:
Connect - connect to an Aerospike cluster
Get - read a record from the Aerospike cluster
Put - write a record to the Aerospike cluster
Increment - increment a bin value in a record
Disconnect - disconnect from the Aerospike cluster

You can use this example to wrap other Aerospike operations in the C library. This C library must be installed on the Lua library path for Lua to find it. 

There is a lot of information at Lua users wiki that describes calling C from Lua. Chapter 24 of “Programming in Lua” describes in detail how this is done.

In general, you’ll need the Lua development environment, and thus lua.h in the include path. You can include the Lua 5.0 or 5.1 environment from source, or install a Lua development package with your package manager.
###Build Instructions
Run the build script located in the root directory of the repository to build the library "as_lua.so"

For Linux
```
./build_linux.sh
```
For OS X
```
./build_osx.sh
```
The shared library “as_lua.so” has dependencies on these libraries:
```
aerospike
ssl
crypto
pthread
rt
lua
m
```
IMPORTANT: The shared library as_lua.so should be placed in the Lua library path:
```
./as_lua.so
/usr/local/lib/lua/5.1/as_lua.so
/usr/lib/lua/5.1/as_lua.so
```
###Lua application to calling Aerospike

This example includes a simple Lua program that exercises each function implemented in the library. You will find code in the file “main.lua” in the “test” subdirectory.

Some things to remember:
This is a C library that calls the Aerospike 3 C Client API. 
This project has all the same library dependencies as for the Aerospike C Client API. So make sure you can link to them. See the C Client development guide for specific details. 


##Discussion
The functions that will be exposed to Lua need to be defined in the following code:
```c
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
```
This function is called by the require statement in Lua. When require is called Lua will look for a library named “as_lua.so” on its library path. Be sure to put as_lua.so in one of the folowing directories, depending on your Lua installation:
```
./as_lua.so
/usr/local/lib/lua/5.1/as_lua.so
/usr/lib/lua/5.1/as_lua.so
```
The C function lua_open_aerospike is called by the Lua function require “as_lua”. At the start of you Lua application you should have code like this:
```lua
local as = require "as_lua"
``` 
Connect
To connect to an Aerospike cluster you need to supply one or more “seed nodes” and ports.  When a client application connects to a cluster, one name or IP address and port is sufficient. The Aerospike intelligent client will discover all the nodes in the cluster and become a 1st class observer of them. Additional nodes can be supplied at connection time in case the node you have specified is actually down.

Our Lua function will take one seed node address and port, and return a handle to a cluster.
```lua
err, message, cluster = as.connect("localhost", 3000)
print("connected", err, message)
```
Connect must be called before you call any other Aerospike function. 
Connect will return 3 values, in the above example, the variables err and message will contain the error code and error message, and the variable cluster will be a handle to the Aerospike cluster. You will use cluster handle in subsequent Aerospike function calls.

The C code to implement this function:
```c
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
```
Lua uses its own stack mechanism to pass parameters between Lua and C. You will note at the start of this function parameters are taken from the stack
```c
const char *hostName = luaL_checkstring(L, 1);
int port = lua_tointeger(L, 2);
```
and at the end of the function the return values are pushed onto the stack
```c
lua_pushnumber(L, err.code);
lua_pushstring(L, err.message);
lua_pushlightuserdata(L, &as);
return 3;
```
Note that there are 3 return values: 
	error number
	error message
	cluster pointer
Disconnect
When your Lua application has completed using Aerospike, usually at the end of the Lua code, it should disconnect from the Aerospike cluster. This frees resources in the process, things like socket connections, file descriptors, etc.

Our Lua function to disconnect from the cluster will take one parameter that is the cluster handle.
```lua
  err, message = as.disconnect(cluster)
  print("disconnected", err, message)
```
The C code to implement this function:
```c
static int disconnect(lua_State *L){
	aerospike* as = lua_touserdata(L, 1);
	as_error err;
	aerospike_close(as, &err);
	lua_pushnumber(L, err.code);
	lua_pushstring(L, err.message);
	return 2;
}
```
You should be seeing a pattern emerging in the way the parameters are passed to the function and how values are returned. The actual code that interacts with Aerospike is trivial. Most of the work in this function is to obtain the pointer to the cluster and return 2 values to Lua
Get
To read a record from Aerospike you use the get function. The cluster handle, obtained from the connect function, is the first argument and is the reference the get function uses to interact with the cluster.

The next 3 parameters are namespace, set and key, in that order. These values uniquely identify the record.
```lua
  err, message, record = as.get(cluster, "test", "test", "peter001")
  print("read record", err, message, record.uid, record.name, record.dob)
```
The get function returns 3 values. The 1st is the error code, the second is the error message if one exists, the 3rd is a table containing the record values. 

You will note that this function reads all the Bins in the record and is the easiest to implement. A better implementation would be to offer the capability to read a subset of bins. I will leave that activity up to the reader.

Put
To write data to Aerospike you use the put function. The cluster handle, obtained from the connect function, is the first argument and is the reference the put function uses to interact with the cluster.

The next 3 parameters are namespace, set and key, in that order. These values uniquely identify the record.

The 5th parameters is a table containing the Bin names and values. This is the actual data you want to store in Aerospike.
```lua
  local bins = {}
  bins["uid"] = "peter001"
  bins["name"] = "Peter"
  bins["dob"] = 19800101
  
  err, message = as.put(cluster, "test", "test", "peter001", 3, bins)
  print("saved record", err, message)
```
The put function returns 2 values. The 1st is the error code, the second is the error message if one exists. 

This function illustrates the complexity of passing parameters and return values between Lua and C. The complexity is in obtaining the values in table parameter. The function add_bins_to_rec takes the passed-in table, containing the bin names and values, and creates bin values and populates a as_record structure. This structure is returned to the caller.  

The tricky part is obtaining the individual elements from the table. This function manipulates the Lua stack to iterate through the elements in the table and creates Bin values.
```c
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
        const char *key = lua_tostring(L, -1);

        // add to record
        if (lua_isnumber(L, -2)){
        	int intValue = lua_tointeger(L, -2);
        	as_record_set_int64(&rec, key, intValue);

        } else if (lua_isstring(L, -2)){
        	const char *value = lua_tostring(L, -2);
        	as_record_set_str(&rec, key, value);
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
```
Increment
The increment function is quite similar to the put function in that it changes the value of one or more Bins in a record. It is useful when your application uses counters, and it also removes the need to read the value, increment the value in your application and rewrite it.
```lua
bins = {}
bins["counter"] = 1
err, message = as.increment(cluster, "test", "test", "peter003", 1, bins)
print("incremented record", err, message)
```

