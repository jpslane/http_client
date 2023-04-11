CC=gcc
CC_OPTS=
CC_LIBS=
CC_DEFINES=
CC_INCLUDES=
CC_ARGS=${CC_OPTS} ${CC_LIBS} ${CC_DEFINES} ${CC_INCLUDES}

.PHONY=clean

#target "all" depends on all others
all: client server talker listener

# client C depends on source file client.c, if that changes, make client will 
# rebuild the binary

client: client.c
	@${CC} ${CC_ARGS} -o client client.c
	
server: server.c
	@${CC} ${CC_ARGS} -o server server.c

talker: talker.c
	@${CC} ${CC_ARGS} -o talker talker.c

listener: listener.c
	@${CC} ${CC_ARGS} -o listener listener.c
	
clean:
	@rm -f server client talker listener *.o
