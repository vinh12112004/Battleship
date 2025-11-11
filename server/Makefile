# Makefile cho MSYS2 MinGW 64-bit

CC = gcc

# Compile flags
CFLAGS = -Iinclude \
         -Isrc \
         -I"C:/msys64/mingw64/include/mongoc-2.1.0" \
         -I"C:/msys64/mingw64/include/bson-2.1.0" \
         -I"C:/msys64/mingw64/include" \
         -Wall -Wextra -g

# Link flags
LIBS = -L"C:/msys64/mingw64/lib" \
       -lmongoc2 -lbson2 -lssl -lcrypto -lcjson \
       -lws2_32 -lshlwapi -lDnsapi -lcrypt32 -lzstd

# ------------------------
# Source files
# ------------------------

# Server
SERVER_SRC = src/main.c \
             src/auth/*.c \
             src/database/*.c \
             src/game/*.c \
             src/matchmaking/*.c \
             src/network/*.c \
             src/utils/*.c \

# Test client
TEST_SRC = src/test/test_client.c \
           src/auth/*.c \
           src/database/*.c \
           src/network/*.c \
           src/game/*.c \
           src/matchmaking/*.c \
           src/utils/*.c \

# Output executables
SERVER_OUT = server.exe
TEST_OUT   = test_client.exe

# ------------------------
# Targets
# ------------------------

all: server test_client

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_OUT) $(LIBS)

test_client: $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_OUT) $(LIBS)

clean:
	rm -f $(SERVER_OUT) $(TEST_OUT)
