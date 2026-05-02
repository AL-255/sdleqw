CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c99
INCLUDE := -Isrc
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

SRCS := \
  src/bitmap.c \
  src/font.c \
  src/hp_font.c \
  src/hp_menu.c \
  src/expr.c \
  src/render.c \
  src/eqw.c \
  src/main.c

OBJS := $(SRCS:.c=.o)
BIN  := sdleqw

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(SDL_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)

screenshot.pgm: $(BIN)
	./$(BIN) --render screenshot.pgm

test: test_keys
	./test_keys

test_keys: src/test_keys.c src/expr.c src/eqw.c src/render.c src/font.c src/bitmap.c
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

.PHONY: all clean test
