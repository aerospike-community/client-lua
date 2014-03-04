#Aerospike Lua Client example
This project conains the files necassary to build the Lua client
library to provide 5 basic operations on the Aerospike databse servers.

These operations are:
* connect - connect to a cluster
* disconnect - disconnect from a cluster
* get - read a record
* put - write a record
* increment - increment a bin value 

This example is implemented in C as a library (.so) and wraps the Aerospike 3 C client.
It therefore has all the dependencies of the Aerospike C client, and following
the Aerospike C client build process is required.

##Build
To build the debug target, navigate to the "Debug" directory then run the makefile
To build the release target, navigate to the "Release" directory and run the makefile

Note: At the time of writing the Aerospike C API code only supports Linux, so your Lua application 
that uses Aerospike will only work on Linux.

##Usage
The target library needs to be on the Lua path to be available to the Lua application.
 
##Example
Located in the test sub directory the file 'main.lua' demonstrates how to use the library.

#Details
There is a lot of information at Lua users wiki that describes calling C from Lua. Chapter 24 of “Programming in Lua” describes in detail how this is done.

The functions that will be exposed to Lua need to be defined in the following code:

```
static const struct luaL_Reg as_client [] = {
        {"connect", connect},
        {"disconnect", disconnect},
        {"get", get},
        {"put", put},
        {"increment", increment},
        {NULL, NULL}
};

extern int luaopen_aerospike(lua_State *L){
    luaL_register(L, "aerospike", as_client);
    return 0;
}
```

This function is called by the require statement in Lua. When require is called Lua will look for a library named “aerospike.so” on it’s library path. Be sure to put aerospike.so in one of the folowing directories:

```
./aerospike.so
/usr/local/lib/lua/5.1/aerospike.so
/usr/lib/lua/5.1/aerospike.so
```

The C function lua_open_aerospike is called by the Lua function require “aerospike”. At the start of you Lua application you should have code like this:

```lua
local as = require "aerospike"
```
 
##Connect
To connect to an Aerospike cluster you need to supply one or more “seed nodes” and ports.  When a client application connects to a cluster, one name or IP address and port is sufficient. The Aerospike intelligent client will discover all the nodes in the cluster and become a 1st class observer of them. Additional nodes can be supplied at connection time in case the node you have specified is actually down.

Our Lua function will take one seed node address and port, and return a handle to a cluster.

```lua
  err, message, cluster = as.connect("localhost", 3000)
  print("connected", err, message)
```

Connect must be called before you call any other Aerospike function. 
Connect will return 3 values, in the above example, the variables err and message will contain the error code and error message, and the variable cluster will be a handle to the Aerospike cluster. You will use cluster handle in subsequent Aerospike function calls.

The C code to implement this function:

```C
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

Lua uses it’s own stack mechanism to pass parameters between Lua and C. You will note at the start of this function parameters are taken from the stack

```C
const char *hostName = luaL_checkstring(L, 1);
int port = lua_tointeger(L, 2);
```

and at the end of the function the return values are pushed onto the stack

```C
lua_pushnumber(L, err.code);
lua_pushstring(L, err.message);
lua_pushlightuserdata(L, &as);
return 3;
```

Note that there are 3 return values: 
```
    error number
    error message
    cluster pointer
```

##Disconnect
When your Lua application has completed using Aerospike, usually at the end of the Lua code, 
it should disconnect from the Aerospike cluster. This frees resources in the process, 
things like socket connections, file descriptors, etc.

Our Lua function to disconnect from the cluster will take one parameter that is the cluster handle.

```lua
  err, message = as.disconnect(cluster)
  print("disconnected", err, message)
```

The C code to implement this function:

static int disconnect(lua_State *L){
    aerospike* as = lua_touserdata(L, 1);
    as_error err;
    aerospike_close(as, &err);
    lua_pushnumber(L, err.code);
    lua_pushstring(L, err.message);
    return 2;
}

You should be seeing a pattern emerging in the way the parameters are passed to the function 
and how values are returned. The actual code that interacts with Aerospike is trivial. 
Most of the work in this function is to obtain the pointer to the cluster and return 2 values to Lua.

##Get
To read a record from Aerospike you use the get function. The cluster handle, obtained from the connect function, is the first argument and is the reference the get function uses to interact with the cluster.

The next 3 parameters are namespace, set and key, in that order. These values uniquely identify the record.

```lua
  err, message, record = as.get(cluster, "test", "test", "peter001")
  print("read record", err, message, record.uid, record.name, record.dob)
```

The get function returns 3 values. The 1st is the error code, the second is the error message if one exists, the 3rd is a table containing the record values. 

You will note that this function reads all the Bins in the record and is the easiest to implement. A better implementation would be to offer the capability to read a subset of bins. I will leave that activity up to the reader.

##Put
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

```C
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

##Increment
The increment function is quite similar to the put function in that it changes the value of one or more Bins in a record. It is useful when your application uses counters, and it also removes the need to read the value, increment the value in your application and rewrite it.

```
bins = {}
bins["counter"] = 1
err, message = as.increment(cluster, "test", "test", "peter003", 1, bins)
print("incremented record", err, message)
```

#Putting it all together
##Compiling and linking the C wrapper
To compile the C wrapper you will need the following gcc flags:

```
-std=gnu99 -g -rdynamic -Wall -fno-common -fno-strict-aliasing 
-fPIC -DMARCH_$(ARCH) -D_FILE_OFFSET_BITS=64 
-D_REENTRANT -D_GNU_SOURCE -DMEM_COUNT
```

Your linkage target should be a shared library “aerospike.so” and should include the following dependent libraries:
```
aerospike
ssl
crypto
pthread
rt
lua
m
```

IMPORTANT: The shared library aerospike.so should be placed in the Lua library path.
Lua application to calling Aerospike

This example includes a simple Lua program that exercises each function implemented in the library. 
You will find code in the file “main.lua” in the “test” subdirectory.
