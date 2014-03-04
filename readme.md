#Aerospike Lua Client example

This project conains the files necassary to build the Lua client
library examples to provide 5 basic operations on the Aerospike databse 
servers.

These operations are:
* connect - connect to a cluster
* disconnect - disconnect from a cluster
* get - read a record
* put - write a record
* increment - increment a bin value 

This example is implemented in C as a library (.so) and wraps the Aerospike 3 C client.
It therefore has all the dependencies of the Aerospike C client.

##Build
To build the debug target, navigate to the "Debug" directory then run the makefile
To build the release target, navigate to the "Release" directory and run the makefile

##Usage
The target library needs to be on the Lua path to be available to the Lua application.
 
##Example
Located in the test sub directory the file 'main.lua' demonstrates how to use the library.
