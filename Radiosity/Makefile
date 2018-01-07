APP_NAME=rad
APP_SRCS=src/*.cpp
CFLAGS=-g -O2 -lGLEW -lglfw -lGL
CC=g++

all: $(SERVER_SRCS)
	$(CC) $(APP_SRCS) $(CFLAGS) -o $(APP_NAME) 

clean:
	/bin/rm -f rad
