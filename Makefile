CC = gcc

CFLAGS = -Wall -Wextra

SDLFLAGS = -IC:/msys64/ucrt64/include/SDL2 -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
NETFLAGS = -lws2_32

# Cliente: GUI con SDL + sockets nativos (Winsock) + hilos (CreateThread).
# Embebe el modulo servidor (server.c + physics.c) para poder convertirse en
# host durante la migracion punto a punto.
CLIENT_SRC = src/main.c \
src/game.c \
src/player.c \
src/input.c \
src/ball.c \
src/render.c \
src/network.c \
src/server.c \
src/physics.c

# Servidor independiente: SIN SDL, solo sockets nativos + hilos + fisica
SERVER_SRC = src/server_main.c \
src/server.c \
src/game.c \
src/player.c \
src/ball.c \
src/physics.c

CLIENT_OUT = build/futbolito.exe
SERVER_OUT = build/futbolito_server.exe

all: client server

client:
	$(CC) $(CLIENT_SRC) -o $(CLIENT_OUT) $(CFLAGS) $(SDLFLAGS) $(NETFLAGS)

server:
	$(CC) $(SERVER_SRC) -o $(SERVER_OUT) $(CFLAGS) $(NETFLAGS)

run: client
	./$(CLIENT_OUT)

run-server: server
	./$(SERVER_OUT)

clean:
	rm -f $(CLIENT_OUT) $(SERVER_OUT)
