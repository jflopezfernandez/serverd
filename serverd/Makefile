
vpath %.c src

RM := rm -f
RMDIR := rm -rf

CC := gcc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Og -ggdb
CPPFLAGS := -D_GNU_SOURCE -D_FORTIFY_SOURCE=2
LDFLAGS := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
LIBS :=

SRCS := $(notdir $(wildcard src/*.c))
OBJS := $(patsubst %.c,%.o,$(SRCS))

TARGET := serverd

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	$(RM) $(OBJS) $(TARGET)
