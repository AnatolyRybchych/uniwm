ifeq ($(OS),Windows_NT)
    PLATFORM := windows
else
    $(error Only the Windows target is implemented)
endif

TARGET := $(PLATFORM)

BUILDDIR := build
BINDIR   := bin

MC_REPO   := https://github.com/AnatolyRybchych/c.git
MC_BRANCH := main
MCDIR     := external/c
MC_STAMP  := $(MCDIR)/.branch.$(MC_BRANCH)

MC_PKGS   := core geometry graphics net os wm win32_wm
MC_INC    := $(addprefix -I$(abspath $(MCDIR))/package/,$(addsuffix /include,$(MC_PKGS)))
MC_LIBS   := $(foreach p,$(MC_PKGS),$(MCDIR)/package/$(p)/lib$(p).a)
MC_CFLAGS := -std=c11 -Wall -Wextra $(MC_INC)

LUADIR     := $(MCDIR)/package/lua
LUA_SRCDIR := $(LUADIR)/lua
LUALIB     := $(LUADIR)/liblua.a
LUADLL     := $(BINDIR)/lua54.dll
LUA_IMP    := $(BUILDDIR)/liblua54.dll.a

WMLUA_DIR  := $(MCDIR)/package/wm-lua
WMLUA_SRC  := $(WMLUA_DIR)/src/wm-lua.c
WMLUA_OBJ  := $(BUILDDIR)/wm-lua.o

LIBUNIWM_DIR    := libuniwm
LIBUNIWMLUA_DIR := libuniwm-lua
UNIWMWIN_DIR    := uniwm-windows
APP_DIR         := uniwm

INCLUDES := -I$(LIBUNIWM_DIR)/include -I$(LIBUNIWMLUA_DIR)/include -I$(UNIWMWIN_DIR)/include $(MC_INC) -I$(WMLUA_DIR)/include -I$(LUA_SRCDIR)
CFLAGS   := -std=c11 -Wall -Wextra $(INCLUDES)

WIN_LDLIBS := -lole32 -loleaut32 -lruntimeobject -luuid -luser32 -lgdi32 -ldwmapi

LIBUNIWM        := $(BINDIR)/libuniwm.dll
LIBUNIWM_IMP    := $(BUILDDIR)/libuniwm.dll.a
LIBUNIWMLUA     := $(BINDIR)/libuniwm-lua.dll
LIBUNIWMLUA_IMP := $(BUILDDIR)/libuniwm-lua.dll.a
UNIWMWIN        := $(BINDIR)/uniwm-windows.dll
UNIWMWIN_IMP    := $(BUILDDIR)/uniwm-windows.dll.a
BIN             := $(BINDIR)/uniwm.exe

LIBUNIWM_SRC    := $(wildcard $(LIBUNIWM_DIR)/src/*.c)
LIBUNIWMLUA_SRC := $(wildcard $(LIBUNIWMLUA_DIR)/src/*.c)
UNIWMWIN_SRC    := $(wildcard $(UNIWMWIN_DIR)/src/*.c)
APP_SRC         := $(wildcard $(APP_DIR)/src/*.c)

LIBUNIWM_OBJ    := $(addprefix $(BUILDDIR)/,$(notdir $(LIBUNIWM_SRC:.c=.o)))
LIBUNIWMLUA_OBJ := $(addprefix $(BUILDDIR)/,$(notdir $(LIBUNIWMLUA_SRC:.c=.o)))
UNIWMWIN_OBJ    := $(addprefix $(BUILDDIR)/,$(notdir $(UNIWMWIN_SRC:.c=.o)))
APP_OBJ         := $(addprefix $(BUILDDIR)/,$(notdir $(APP_SRC:.c=.o)))

OBJ := $(LIBUNIWM_OBJ) $(LIBUNIWMLUA_OBJ) $(WMLUA_OBJ) $(UNIWMWIN_OBJ) $(APP_OBJ)

vpath %.c $(LIBUNIWM_DIR)/src $(LIBUNIWMLUA_DIR)/src $(WMLUA_DIR)/src $(UNIWMWIN_DIR)/src $(APP_DIR)/src

include rules.mk

.PHONY: all clean
all: $(BIN)

$(UNIWMWIN_OBJ): CFLAGS += -DUNIWM_WINDOWS_BUILD

$(BIN): $(APP_OBJ) $(LIBUNIWM_IMP) $(LIBUNIWMLUA_IMP) $(UNIWMWIN_IMP) $(LUA_IMP) $(MC_LIBS) | $(BINDIR)
	$(LD) $(CFLAGS) -o $@ $(APP_OBJ) $(LIBUNIWMLUA_IMP) $(LIBUNIWM_IMP) $(UNIWMWIN_IMP) $(LUA_IMP) -Wl,--start-group $(MC_LIBS) -Wl,--end-group

$(LIBUNIWM): $(LIBUNIWM_OBJ) $(MC_LIBS) | $(BINDIR)
	$(LD) -shared $(CFLAGS) -o $@ $(LIBUNIWM_OBJ) -Wl,--whole-archive $(MC_LIBS) -Wl,--no-whole-archive -luser32 -lgdi32 -ldwmapi -Wl,--export-all-symbols -Wl,--out-implib,$(LIBUNIWM_IMP)

$(LIBUNIWM_IMP): $(LIBUNIWM) ;

$(UNIWMWIN): $(UNIWMWIN_OBJ) $(MC_LIBS) | $(BINDIR)
	$(LD) -shared $(CFLAGS) -o $@ $(UNIWMWIN_OBJ) -Wl,--out-implib,$(UNIWMWIN_IMP) -Wl,--start-group $(MC_LIBS) -Wl,--end-group $(WIN_LDLIBS)

$(UNIWMWIN_IMP): $(UNIWMWIN) ;

$(LIBUNIWMLUA): $(LIBUNIWMLUA_OBJ) $(WMLUA_OBJ) $(LIBUNIWM_IMP) $(LUA_IMP) | $(BINDIR)
	$(LD) -shared $(CFLAGS) -o $@ $(LIBUNIWMLUA_OBJ) $(WMLUA_OBJ) $(LIBUNIWM_IMP) $(LUA_IMP) -Wl,--export-all-symbols -Wl,--out-implib,$(LIBUNIWMLUA_IMP)

$(LIBUNIWMLUA_IMP): $(LIBUNIWMLUA) ;

$(LUADLL): $(LUALIB) | $(BINDIR)
	$(CC) -shared -o $@ -Wl,--whole-archive $(LUALIB) -Wl,--no-whole-archive -Wl,--export-all-symbols -Wl,--out-implib,$(LUA_IMP)

$(LUA_IMP): $(LUADLL) ;

$(OBJ): | $(MC_STAMP) $(LUALIB)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

define MC_BUILD_PKG
$(MCDIR)/package/$(1)/lib$(1).a: $(MC_STAMP)
	$$(MAKE) -C $(MCDIR)/package/$(1) CC=$$(CC) AR=$$(AR) CFLAGS="$$(MC_CFLAGS)"
endef
$(foreach p,$(MC_PKGS),$(eval $(call MC_BUILD_PKG,$(p))))

$(LUALIB): | $(MC_STAMP)
	$(MAKE) -C $(LUADIR) liblua.a CC=$(CC)

$(MC_STAMP):
	$(GIT) init -q $(MCDIR)
	$(GIT) -C $(MCDIR) config core.autocrlf false
	$(GIT) -C $(MCDIR) fetch -q $(MC_REPO) "+refs/heads/*:refs/remotes/origin/*"
	$(GIT) -C $(MCDIR) checkout -q -B $(MC_BRANCH) refs/remotes/origin/$(MC_BRANCH)
	$(TOUCH) $@

$(BUILDDIR) $(BINDIR):
	$(MKDIR) $@

clean:
	-$(RM) $(BUILDDIR)
	-$(RM) $(BINDIR)
