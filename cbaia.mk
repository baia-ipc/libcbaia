#
# Makefile fragment for the projects using a copy of libcbaia (vendoring).
#
# Usage, in the Makefile of the project:
#
#   CBAIA_DIR = ext/libcbaia
#   include $(CBAIA_DIR)/cbaia.mk
#
#   CFLAGS += $(CBAIA_INCLUDES)
#   LDLIBS += $(CBAIA_LDLIBS)
#   bin/prog: src/prog.c $(CBAIA_LIBS) ...
#   	$(CC) ... $(CBAIA_LIB) $(LDLIBS)
#
# The library is built (by the Makefile of libcbaia, with the DEBUGFLAGS
# of the project, if defined) when its sources change.
#
# zlib: by default, the copy included in libcbaia is used; with
# SYSTEM_LIBS=1 (e.g. make SYSTEM_LIBS=1), the system library.
#

CBAIA_COMPONENTS = core ds io seq
CBAIA_INCLUDES = $(addprefix -I$(CBAIA_DIR)/src/,$(CBAIA_COMPONENTS)) \
  -I$(CBAIA_DIR)/ext/khash
CBAIA_LIB = $(CBAIA_DIR)/lib/libcbaia.a
ifeq ($(SYSTEM_LIBS),1)
CBAIA_LDLIBS = -lz -lm
CBAIA_ZLIB =
else
CBAIA_INCLUDES += -I$(CBAIA_DIR)/ext/zlib
CBAIA_ZLIB = $(CBAIA_DIR)/lib/libcbaia-zlib.a
CBAIA_LDLIBS = $(CBAIA_ZLIB) -lm
endif
# all libraries built by libcbaia (prerequisites of the programs)
CBAIA_LIBS = $(CBAIA_LIB) $(CBAIA_ZLIB)

CBAIA_MAKE_LIB = $(MAKE) -C $(CBAIA_DIR) lib SYSTEM_LIBS=$(SYSTEM_LIBS) \
  $(if $(DEBUGFLAGS),DEBUGFLAGS='$(DEBUGFLAGS)')

$(CBAIA_LIB): $(wildcard $(CBAIA_DIR)/src/*/*.c $(CBAIA_DIR)/src/*/*.h)
	$(CBAIA_MAKE_LIB)

# the zlib library is built after the main one (with make -j, two runs of
# the sub-make at the same time would conflict); its recipe is also needed
# when only it is missing (e.g. after a build with SYSTEM_LIBS=1)
ifneq ($(CBAIA_ZLIB),)
$(CBAIA_ZLIB): $(CBAIA_LIB)
	$(CBAIA_MAKE_LIB)
endif

cbaia-clean:
	$(MAKE) -C $(CBAIA_DIR) cleanup
