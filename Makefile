SRCDIR=src
OBJDIR=obj
LIBDIR=lib
BINDIR=bin
TESTDIR=tests
$(shell mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR) >/dev/null)

# components (subdirectories of src); the header names are unique
# over all components
COMPONENTS = core ds io seq
COMPONENT_DIRS = $(addprefix $(SRCDIR)/,$(COMPONENTS))
vpath %.c $(COMPONENT_DIRS)

SRCS=$(wildcard $(addsuffix /*.c,$(COMPONENT_DIRS)))
OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS)))
HEADERS=$(wildcard $(addsuffix /*.h,$(COMPONENT_DIRS)))
INCLUDES=$(addprefix -I,$(COMPONENT_DIRS)) -Iext/khash

LIB = $(LIBDIR)/libcbaia.a

# zlib: vendored (default; compiled into a separate library) or the system
# library (make SYSTEM_LIBS=1)
ZLIBDIR = ext/zlib
ifeq ($(SYSTEM_LIBS),1)
ZLIB =
ZLIB_LDLIBS = -lz
else
ZLIB = $(LIBDIR)/libcbaia-zlib.a
ZLIB_LDLIBS = $(ZLIB)
ZLIB_OBJS = $(patsubst $(ZLIBDIR)/%.c,$(OBJDIR)/zlib/%.o,\
              $(wildcard $(ZLIBDIR)/*.c))
INCLUDES += -I$(ZLIBDIR)
endif
TESTS = $(BINDIR)/cbaia-tests

PREFIX ?= /usr/local

.PHONY: default lib test fuzz manual install uninstall clean cleanup cleandep

default: lib

lib: $(LIB) $(ZLIB)

CFLAGS += -Wall -Wextra -std=gnu11 $(INCLUDES)
LDLIBS += $(ZLIB_LDLIBS) -lm

DEBUGFLAGS_DEBUG = -gdwarf-3 -g3 -O0
DEBUGFLAGS_NDEBUG = -O3 -DNDEBUG
DEBUGFLAGS_PROF = -no-pie -pg $(DEBUGFLAGS_DEBUG)
DEBUGFLAGS_SANITIZE = -fsanitize=address,undefined $(DEBUGFLAGS_DEBUG)
# select the DEBUGFLAGS settings to use:
DEBUGFLAGS=$(DEBUGFLAGS_NDEBUG)

# header dependency tracking recipy modified from:
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(DEBUGFLAGS) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

# the objects are rebuilt when the compilation flags change (e.g. the
# DEBUGFLAGS of a project vendoring the library): the flags are saved in
# FLAGS_STAMP, which is rewritten (thus newer than the objects) only if
# they differ from those of the previous build
FLAGS_STAMP = $(OBJDIR)/.flags
FLAGS_NOW = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(DEBUGFLAGS)
$(shell printf '%s\n' '$(subst ','\'',$(FLAGS_NOW))' | \
  cmp -s - $(FLAGS_STAMP) || \
  printf '%s\n' '$(subst ','\'',$(FLAGS_NOW))' > $(FLAGS_STAMP))

$(OBJDIR)/%.o : %.c
$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d $(FLAGS_STAMP)
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(notdir $(basename $(SRCS)))))

# end of header dependency tracking

$(LIB): $(OBJS)
	rm -f $@
	$(AR) rcs $@ $^

ZLIB_CFLAGS = -O3 -DHAVE_UNISTD_H -DHAVE_STDARG_H
ifeq ($(shell uname -s),Linux)
ZLIB_CFLAGS += -D_LARGEFILE64_SOURCE=1
endif
# faster deflate (about 2 times, for level 6) on the architectures allowing
# unaligned memory accesses, as in the zlib packages of Debian/Ubuntu
ifneq ($(filter x86_64 i386 i686 aarch64 arm64,$(shell uname -m)),)
ZLIB_CFLAGS += -DUNALIGNED_OK
endif

$(OBJDIR)/zlib/%.o: $(ZLIBDIR)/%.c
	@mkdir -p $(OBJDIR)/zlib
	$(CC) $(ZLIB_CFLAGS) -c -o $@ $<

$(LIBDIR)/libcbaia-zlib.a: $(ZLIB_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

$(BINDIR)/%: $(TESTDIR)/%.c $(LIB) $(ZLIB) $(HEADERS)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ $< $(LIB) $(LDLIBS)

# the tests are run under valgrind, if available (e.g. not on recent macOS)
VALGRIND = $(if $(shell command -v valgrind),valgrind --error-exitcode=1 -q \
           --leak-check=full --errors-for-leak-kinds=definite)

test: $(TESTS)
	$(VALGRIND) $(BINDIR)/cbaia-tests -q
	$(MAKE) -C docs/manual/examples check

# fuzzing harnesses (libFuzzer, clang), see docs/manual/testing.rst
fuzz:
	tests/fuzz/build.sh

# user manual (PDF), from docs/manual (requires Sphinx and LaTeX)
SPHINXBUILD ?= sphinx-build
manual:
	$(SPHINXBUILD) -M latexpdf docs/manual docs/manual/_build -q
	mkdir -p manual
	cp docs/manual/_build/latex/libcbaia.pdf manual/libcbaia-manual.pdf

install: $(LIB) $(ZLIB)
	mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/cbaia
	cp $(LIB) $(ZLIB) $(DESTDIR)$(PREFIX)/lib/
	cp $(HEADERS) ext/khash/khash.h $(DESTDIR)$(PREFIX)/include/cbaia/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libcbaia.a \
	  $(DESTDIR)$(PREFIX)/lib/libcbaia-zlib.a
	rm -rf $(DESTDIR)$(PREFIX)/include/cbaia

cleanup: clean
	rm -rf $(BINDIR) $(LIBDIR)

clean: cleandep
	rm -rf $(OBJDIR)

cleandep:
	rm -rf $(DEPDIR)
