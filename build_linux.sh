gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/as_lua.d" -MT"src/as_lua.d" -o "./src/as_lua.o" "src/as_lua.c" -std=gnu99 -g -rdynamic -Wall -fno-common -fno-strict-aliasing -fPIC -DMARCH_x86_64 -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_GNU_SOURCE
gcc -shared -o "as_lua.so"  ./src/as_lua.o -laerospike -lssl -lcrypto -lpthread -lrt -lm
