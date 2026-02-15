# Quake2 Makefile - Optimized for Performance (Ethos: Simplicity & Speed)
PLATFORM=$(shell uname -s|tr A-Z a-z)
DESTDIR=$(HOME)/.quake2

# --- Kompilator i Standard ---
CC=gcc
C_STANDARD = -std=gnu11
WARNINGS = -Wall -Wextra -Wpedantic

# --- Ścieżki ---
DEPS_DIR = deps
CONFIG_DIR = config
SRC_DIR = src

# --- Flagi Bazowe ---
BASE_CFLAGS = -funsigned-char -pipe $(shell sdl2-config --cflags) \
              -DGL_QUAKE -DUSE_SDL -DUSE_CURL \
              -I$(SRC_DIR)/include/nanosvg \
              -I$(SRC_DIR)/include/uthash \
              -I$(SRC_DIR)/include/steamaudio \
              -msse4.1 -march=native

# --- Optymalizacje (Carmack Style) ---
RELEASE_CFLAGS = $(BASE_CFLAGS) $(C_STANDARD) $(WARNINGS) -O3 -ffast-math -flto -DNDEBUG
DEBUG_CFLAGS   = $(BASE_CFLAGS) $(C_STANDARD) $(WARNINGS) -g

# --- Linker ---
ifeq ($(PLATFORM),freebsd)
    LDFLAGS_COMMON = -lm
else
    LDFLAGS_COMMON = -lm -ldl -Wl,-rpath,'$$ORIGIN'
endif

# UWAGA: Brak -lphonon tutaj zapewnia opcjonalność biblioteki
LDFLAGS_CLIENT = $(shell sdl2-config --libs) $(shell curl-config --libs) \
                 -ljpeg -lpng -lz -lGLU
LDFLAGS_RELEASE = -flto

# --- Obiekty ---

COMMON_OBJS = \
    $(SRC_DIR)/qcommon/cmd.o $(SRC_DIR)/qcommon/cmodel.o $(SRC_DIR)/qcommon/common.o \
    $(SRC_DIR)/qcommon/crc.o $(SRC_DIR)/qcommon/cvar.o $(SRC_DIR)/qcommon/files.o \
    $(SRC_DIR)/qcommon/md4.o $(SRC_DIR)/qcommon/net_chan.o $(SRC_DIR)/qcommon/net.o \
    $(SRC_DIR)/qcommon/pmove.o $(SRC_DIR)/qcommon/q_msg.o \
    $(SRC_DIR)/game/q_shared.o $(SRC_DIR)/game/m_flash.o \
    $(SRC_DIR)/linux/q_shlinux.o $(SRC_DIR)/linux/sys_linux.o $(SRC_DIR)/linux/glob.o \
    $(SRC_DIR)/include/minizip/ioapi.o $(SRC_DIR)/include/minizip/unzip.o

SERVER_OBJS = \
    $(SRC_DIR)/server/sv_ccmds.o $(SRC_DIR)/server/sv_ents.o $(SRC_DIR)/server/sv_game.o \
    $(SRC_DIR)/server/sv_init.o $(SRC_DIR)/server/sv_main.o $(SRC_DIR)/server/sv_send.o \
    $(SRC_DIR)/server/sv_user.o $(SRC_DIR)/server/sv_world.o

UI_OBJS = \
    $(SRC_DIR)/ui/qmenu.o $(SRC_DIR)/ui/ui_addressbook.o $(SRC_DIR)/ui/ui_atoms.o \
    $(SRC_DIR)/ui/ui_controls.o $(SRC_DIR)/ui/ui_credits.o $(SRC_DIR)/ui/ui_demos.o \
    $(SRC_DIR)/ui/ui_dmoptions.o $(SRC_DIR)/ui/ui_download.o $(SRC_DIR)/ui/ui_game.o \
    $(SRC_DIR)/ui/ui_joinserver.o $(SRC_DIR)/ui/ui_keys.o $(SRC_DIR)/ui/ui_loadsavegame.o \
    $(SRC_DIR)/ui/ui_main.o $(SRC_DIR)/ui/ui_mp3.o $(SRC_DIR)/ui/ui_multiplayer.o \
    $(SRC_DIR)/ui/ui_newoptions.o $(SRC_DIR)/ui/ui_playerconfig.o $(SRC_DIR)/ui/ui_quit.o \
    $(SRC_DIR)/ui/ui_startserver.o $(SRC_DIR)/ui/ui_video.o

CLIENT_OBJS = \
    $(SRC_DIR)/client/cl_cin.o $(SRC_DIR)/client/cl_decals.o $(SRC_DIR)/client/cl_demo.o \
    $(SRC_DIR)/client/cl_draw.o $(SRC_DIR)/client/cl_ents.o $(SRC_DIR)/client/cl_fx.o \
    $(SRC_DIR)/client/cl_http.o $(SRC_DIR)/client/cl_input.o $(SRC_DIR)/client/cl_inv.o \
    $(SRC_DIR)/client/cl_loc.o $(SRC_DIR)/client/cl_main.o $(SRC_DIR)/client/cl_newfx.o \
    $(SRC_DIR)/client/cl_parse.o $(SRC_DIR)/client/cl_pred.o $(SRC_DIR)/client/cl_scrn.o \
    $(SRC_DIR)/client/cl_tent.o $(SRC_DIR)/client/cl_view.o $(SRC_DIR)/client/console.o \
    $(SRC_DIR)/client/keys.o $(SRC_DIR)/client/snd_dma.o $(SRC_DIR)/client/snd_mix.o \
    $(SRC_DIR)/client/snd_hrtf.o $(SRC_DIR)/client/vid_sdl.o \
    $(SRC_DIR)/linux/cd_linux.o $(SRC_DIR)/linux/qgl_linux.o $(SRC_DIR)/linux/rw_linux.o \
    $(SRC_DIR)/linux/rw_sdl.o $(SRC_DIR)/linux/snd_sdl.o \
    $(SRC_DIR)/ref_gl/gl_decal.o $(SRC_DIR)/ref_gl/gl_decal_svg.o $(SRC_DIR)/ref_gl/gl_draw.o \
    $(SRC_DIR)/ref_gl/gl_image.o $(SRC_DIR)/ref_gl/gl_light.o $(SRC_DIR)/ref_gl/gl_mesh.o \
    $(SRC_DIR)/ref_gl/gl_model.o $(SRC_DIR)/ref_gl/gl_rmain.o $(SRC_DIR)/ref_gl/gl_rmisc.o \
    $(SRC_DIR)/ref_gl/gl_rsurf.o $(SRC_DIR)/ref_gl/gl_warp.o \
    $(UI_OBJS)

GAME_OBJS = \
    $(SRC_DIR)/game/g_ai.o $(SRC_DIR)/game/g_chase.o $(SRC_DIR)/game/g_cmds.o \
    $(SRC_DIR)/game/g_combat.o $(SRC_DIR)/game/g_core_moves.o $(SRC_DIR)/game/g_func.o \
    $(SRC_DIR)/game/g_items.o $(SRC_DIR)/game/g_main.o $(SRC_DIR)/game/g_misc.o \
    $(SRC_DIR)/game/g_monster.o $(SRC_DIR)/game/g_move_reg.o $(SRC_DIR)/game/g_phys.o \
    $(SRC_DIR)/game/g_save.o $(SRC_DIR)/game/g_spawn.o $(SRC_DIR)/game/g_svcmds.o \
    $(SRC_DIR)/game/g_target.o $(SRC_DIR)/game/g_trigger.o $(SRC_DIR)/game/g_turret.o \
    $(SRC_DIR)/game/g_utils.o $(SRC_DIR)/game/g_weapon.o $(SRC_DIR)/game/m_actor.o \
    $(SRC_DIR)/game/m_berserk.o $(SRC_DIR)/game/m_boss2.o $(SRC_DIR)/game/m_boss31.o \
    $(SRC_DIR)/game/m_boss32.o $(SRC_DIR)/game/m_boss3.o $(SRC_DIR)/game/m_brain.o \
    $(SRC_DIR)/game/m_chick.o $(SRC_DIR)/game/m_flash.o $(SRC_DIR)/game/m_flipper.o \
    $(SRC_DIR)/game/m_float.o $(SRC_DIR)/game/m_flyer.o $(SRC_DIR)/game/m_gladiator.o \
    $(SRC_DIR)/game/m_gunner.o $(SRC_DIR)/game/m_hover.o $(SRC_DIR)/game/m_infantry.o \
    $(SRC_DIR)/game/m_insane.o $(SRC_DIR)/game/m_medic.o $(SRC_DIR)/game/m_move.o \
    $(SRC_DIR)/game/m_mutant.o $(SRC_DIR)/game/m_parasite.o $(SRC_DIR)/game/m_soldier.o \
    $(SRC_DIR)/game/m_supertank.o $(SRC_DIR)/game/m_tank.o $(SRC_DIR)/game/p_client.o \
    $(SRC_DIR)/game/p_hud.o $(SRC_DIR)/game/p_trail.o $(SRC_DIR)/game/p_view.o \
    $(SRC_DIR)/game/p_weapon.o $(SRC_DIR)/game/q_shared.o

GAME_OBJS = $(wildcard $(SRC_DIR)/game/*.c)
GAME_OBJS_RES = $(GAME_OBJS:.c=.o)

# --- Targets ---
all: release

bin:
	mkdir -p bin

debug: bin
	$(MAKE) targets CFLAGS="$(DEBUG_CFLAGS)" LDFLAGS_REL=""

release: bin
	$(MAKE) targets CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS_REL="$(LDFLAGS_RELEASE)"

targets: bin/q2classic bin/q2game.so copy-optional-deps

copy-optional-deps:
	@echo "[CP] Optional: libphonon.so -> bin/"
	@cp $(DEPS_DIR)/libphonon.so bin/ 2>/dev/null || true

# --- Reguły ---
$(SRC_DIR)/game/%.o : $(SRC_DIR)/game/%.c
	@mkdir -p $(shell dirname $@)
	@echo " [CC] (PIC) $(shell basename $@)"
	@$(CC) $(CFLAGS) -fPIC -o $@ -c $<

$(SRC_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(shell dirname $@)
	@echo " [CC] $(shell basename $@)"
	@$(CC) $(CFLAGS) -o $@ -c $<

bin/q2classic : $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS)
	@echo "[LD] $@"
	@$(CC) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_CLIENT) $(LDFLAGS_REL)

bin/q2game.so : $(GAME_OBJS_RES)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) -shared -o $@ $^ -Wl,--no-as-needed $(LDFLAGS_REL)

install: release
	@echo "[INSTALL] to $(DESTDIR)"
	@mkdir -p $(DESTDIR)/baseq2
	@cp bin/q2classic $(DESTDIR)/
	@cp bin/q2game.so $(DESTDIR)/baseq2/
	@cp bin/libphonon.so $(DESTDIR)/ 2>/dev/null || true
	@cp $(CONFIG_DIR)/*.cfg $(DESTDIR)/baseq2/ 2>/dev/null || true

clean:
	@echo "[CLEAN]"
	@find $(SRC_DIR) -name "*.o" -delete
	@rm -rf bin/*