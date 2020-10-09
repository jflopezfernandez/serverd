
vpath %.c src

RM       := rm -f
RMDIR    := rm -rf

CC       := gcc
CFLAGS   := -std=c17 -Wall -Wextra -Wpedantic -Og -ggdb -g3
CPPFLAGS := -Iinclude -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -DENABLE_JEMALLOC
LDFLAGS  := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
LIBS     := -ljemalloc

SRCS     := $(notdir $(wildcard src/*.c))
OBJS     := $(patsubst %.c,%.o,$(SRCS))

TARGET   := serverd

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $^

.PHONY: docs
docs: html

.PHONY: html
html: docs/Doxyfile
	doxygen $^

.PHONY: install
install: $(TARGET)
	install --owner=root --group=root --mode=u+rwx,g+rx,g-w,o+rx,o-w -T ./serverd /usr/local/bin/serverd
	[ -d /etc/serverd ] || mkdir -p /etc/serverd/
	install --owner=root --group=root --mode=u+rw,u-x,g+r,g-wx,o+r,o-wx -T ./samples/conf/serverd.conf /etc/serverd/serverd.conf
	[ -d /srv/http ] && install --owner=jflopezfernandez --group=http --mode=u+rw,u-x,g+r,g-wx,o+r,o-wx -T ./samples/site/index.html /srv/http/index.html

.PHONY: uninstall
uninstall:
	[ -e /usr/local/bin/serverd ] && rm -f /usr/local/bin/serverd || true
	[ -d /etc/serverd/ ] && rm -rf /etc/serverd/ || true
	[ -e /srv/http/index.html ] && rm -f /srv/http/index.html || true

.PHONY: clean
clean:
	$(RM) $(OBJS) $(TARGET)
