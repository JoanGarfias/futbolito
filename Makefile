CC = gcc

CFLAGS = -Wall -Wextra

SDLFLAGS = -IC:/msys64/ucrt64/include/SDL2 -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lws2_32

SRC = src/main.c \
src/game.c \
src/player.c \
src/input.c \
src/ball.c \
src/physics.c \
src/render.c \
src/network.c

OUT = build/futbolito.exe

all:
	$(CC) $(SRC) -o $(OUT) $(CFLAGS) $(SDLFLAGS)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)