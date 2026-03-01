CC      = gcc
CFLAGS  = -Wall -Wextra -O2 \
          $(shell pkg-config --cflags libavformat libavcodec libavutil \
                                      libswresample libdrm alsa)
LDFLAGS = $(shell pkg-config --libs libavformat libavcodec libavutil \
                                     libswresample libdrm alsa)

SRCS    = src/main.c src/queue.c src/demux.c src/audio.c src/vdec.c src/drm.c
BIN     = zeroplay

PREFIX  ?= /usr/local

.PHONY: all install uninstall clean

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN) $(LDFLAGS) -lpthread

install: $(BIN)
	install -Dm755 $(BIN) $(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(PREFIX)/bin/$(BIN)

clean:
	rm -f $(BIN)
