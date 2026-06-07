ifeq ($(OS),Windows_NT)
    PLATFORM := windows
else
    $(error Only the Windows target is implemented)
endif

TARGET := $(PLATFORM)

MC_REPO   := https://github.com/AnatolyRybchych/c.git
MC_COMMIT := 4922127d8f053d320f1e6816310cc27d21566444
MCDIR     := external/c
MC_STAMP  := $(MCDIR)/.commit.$(MC_COMMIT)

MC_PKGS   := core geometry graphics net wm
MC_INC    := $(addprefix -I$(abspath $(MCDIR))/package/,$(addsuffix /include,$(MC_PKGS) os))
MC_LIBS   := $(foreach p,$(MC_PKGS),$(MCDIR)/package/$(p)/lib$(p).a)
MC_CFLAGS := -std=c11 -Wall -Wextra $(MC_INC)

CFLAGS := -std=c11 -Wall -Wextra -Iinclude -Isrc $(MC_INC)
LDLIBS := -lole32 -loleaut32 -lruntimeobject -luuid

BUILDDIR := build
BINDIR   := bin
BIN      := $(BINDIR)/uniwm.exe

SRC := src/main.c \
       $(wildcard src/wm/*.c) \
       $(wildcard src/target/$(TARGET)/*.c)
OBJ := $(addprefix $(BUILDDIR)/,$(notdir $(SRC:.c=.o)))

vpath %.c src src/wm src/target/$(TARGET)

include rules.mk

.PHONY: all clean
all: $(BIN)

$(BIN): $(OBJ) $(MC_LIBS) | $(BINDIR)
	$(LD) $(CFLAGS) -o $@ $(OBJ) -Wl,--start-group $(MC_LIBS) -Wl,--end-group $(LDLIBS)

$(OBJ): | $(MC_STAMP)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

define MC_BUILD_PKG
$(MCDIR)/package/$(1)/lib$(1).a: $(MC_STAMP)
	$$(MAKE) -C $(MCDIR)/package/$(1) CC=$$(CC) AR=$$(AR) CFLAGS="$$(MC_CFLAGS)"
endef
$(foreach p,$(MC_PKGS),$(eval $(call MC_BUILD_PKG,$(p))))

$(MC_STAMP):
	$(GIT) init -q $(MCDIR)
	$(GIT) -C $(MCDIR) config core.autocrlf false
	$(GIT) -C $(MCDIR) fetch -q --depth 1 $(MC_REPO) $(MC_COMMIT)
	$(GIT) -C $(MCDIR) checkout -q --detach FETCH_HEAD
	$(TOUCH) $@

$(BUILDDIR) $(BINDIR):
	$(MKDIR) $@

clean:
	-$(RM) $(BUILDDIR)
	-$(RM) $(BINDIR)
