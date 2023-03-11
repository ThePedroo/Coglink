CC ?= gcc

PREFIX := /usr/local
LIBDIR := $(PREFIX)/lib
INCLUDEDIR := $(PREFIX)/include/coglink
DOCS_DIR = ../CoglinDocs

CFLAGS := -Wall -Wextra -Wpedantic
LDFLAGS := -std=gnu99 -I/usr/local/include
LFLAGS := -lcurl -ldiscord

all:
	@ sudo mkdir -p $(INCLUDEDIR)
	sudo cp include/* $(INCLUDEDIR)
	$(CC) lib/*.c -c -fPIC $(CFLAGS) -O3 $(LDFLAGS) $(LFLAGS)
	ar qv libcoglink.a *.o
	rm -rf *.o

gen_docs:
	doxygen Doxyfile
	rm -rf $(DOCS_DIR)/search
	mv $(DOCS_DIR)/html/* $(DOCS_DIR)
	rm -rf $(DOCS_DIR)/html

debug:
	@ sudo mkdir -p $(INCLUDEDIR)
	sudo cp include/* $(INCLUDEDIR)
	$(CC) lib/*.c -c -fPIC $(CFLAGS) -g $(LDFLAGS) $(LFLAGS)
	ar qv libcoglink.a *.o
	rm -rf *.o

install:
	sudo mv libcoglink.a $(LIBDIR)
