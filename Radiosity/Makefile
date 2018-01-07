APP_NAME=rad
APP_SRCS=src/*.cpp
CFLAGS=-g -O2 -lGLEW -lglfw -lGL -pthread
CC=g++

all: $(SERVER_SRCS)
	$(CC) -o $(APP_NAME) $(APP_SRCS) $(CFLAGS)

clean:
	/bin/rm -f rad
