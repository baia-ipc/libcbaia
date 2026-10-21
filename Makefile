SRCDIR=src
OBJDIR=obj
LIBDIR=lib
BINDIR=bin
TESTDIR=tests
$(shell mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR) >/dev/null)

SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
HEADERS=$(wildcard $(SRCDIR)/*.h)

LIB = $(LIBDIR)/libcbaia.a
TESTS = $(BINDIR)/cbaia-tests

PREFIX ?= /usr/local

.PHONY: default lib test install uninstall clean cleanup cleandep

default: lib

lib: $(LIB)

CFLAGS += -Wall -Wextra -std=gnu11
LDLIBS += -lz -lm

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

$(OBJDIR)/%.o : $(SRCDIR)/%.c
$(OBJDIR)/%.o : $(SRCDIR)/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(notdir $(basename $(SRCS)))))

# end of header dependency tracking

$(LIB): $(OBJS)
	rm -f $@
	$(AR) rcs $@ $^

$(BINDIR)/%: $(TESTDIR)/%.c $(LIB) $(HEADERS)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -I$(SRCDIR) -o $@ $< $(LIB) $(LDLIBS)

VALGRIND = valgrind --error-exitcode=1 -q --leak-check=full \
           --errors-for-leak-kinds=definite

test: $(TESTS)
	$(VALGRIND) $(BINDIR)/cbaia-tests -q

install: $(LIB)
	mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/cbaia
	cp $(LIB) $(DESTDIR)$(PREFIX)/lib/
	cp $(HEADERS) $(DESTDIR)$(PREFIX)/include/cbaia/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libcbaia.a
	rm -rf $(DESTDIR)$(PREFIX)/include/cbaia

cleanup: clean
	rm -rf $(BINDIR) $(LIBDIR)

clean: cleandep
	rm -rf $(OBJDIR)

cleandep:
	rm -rf $(DEPDIR)
