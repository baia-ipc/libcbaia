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
#   bin/prog: src/prog.c $(CBAIA_LIB) ...
#   	$(CC) ... $(CBAIA_LIB) $(LDLIBS)
#
# The library is built (by the Makefile of libcbaia, with the DEBUGFLAGS
# of the project, if defined) when its sources change.
#

CBAIA_COMPONENTS = core ds io seq
CBAIA_INCLUDES = $(addprefix -I$(CBAIA_DIR)/src/,$(CBAIA_COMPONENTS))
CBAIA_LIB = $(CBAIA_DIR)/lib/libcbaia.a
CBAIA_LDLIBS = -lz -lm

$(CBAIA_LIB): $(wildcard $(CBAIA_DIR)/src/*/*.c $(CBAIA_DIR)/src/*/*.h)
	$(MAKE) -C $(CBAIA_DIR) lib \
	  $(if $(DEBUGFLAGS),DEBUGFLAGS='$(DEBUGFLAGS)')

cbaia-clean:
	$(MAKE) -C $(CBAIA_DIR) cleanup
