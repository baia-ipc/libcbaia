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
CBAIA_INCLUDES = $(addprefix -I$(CBAIA_DIR)/src/,$(CBAIA_COMPONENTS))
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

$(CBAIA_LIBS): $(wildcard $(CBAIA_DIR)/src/*/*.c $(CBAIA_DIR)/src/*/*.h)
	$(MAKE) -C $(CBAIA_DIR) lib SYSTEM_LIBS=$(SYSTEM_LIBS) \
	  $(if $(DEBUGFLAGS),DEBUGFLAGS='$(DEBUGFLAGS)')

cbaia-clean:
	$(MAKE) -C $(CBAIA_DIR) cleanup
