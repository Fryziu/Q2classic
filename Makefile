# Quake2 Makefile - Optimized for Performance
PLATFORM=$(shell uname -s|tr A-Z a-z)
DESTDIR=$(HOME)/.quake2

# --- Kompilator i Standard ---
CC=gcc
C_STANDARD = -std=gnu11
WARNINGS = -Wall -Wextra -Wpedantic

# --- Ścieżki i Flagi Bazowe ---
# Dodano Steam Audio do ścieżek wyszukiwania nagłówków
BASE_CFLAGS = -funsigned-char -pipe $(shell sdl2-config --cflags) \
              -DGL_QUAKE -DUSE_SDL -DUSE_CURL \
              -Isrc/include/nanosvg -Isrc/include/uthash -Isrc/include/steamaudio

# --- Optymalizacje ---
# O3 + ffast-math dla maksymalnej wydajności obliczeń (etos Carmacka)
RELEASE_CFLAGS = $(BASE_CFLAGS) $(C_STANDARD) $(WARNINGS) -O3 -ffast-math -flto -DNDEBUG
DEBUG_CFLAGS   = $(BASE_CFLAGS) $(C_STANDARD) $(WARNINGS) -g

# --- Linker ---
ifeq ($(PLATFORM),freebsd)
    LDFLAGS_COMMON = -lm
else
    LDFLAGS_COMMON = -lm -ldl
endif

LDFLAGS_CLIENT = $(shell sdl2-config --libs) $(shell curl-config --libs) \
                 -ljpeg -lpng -lz -lGLU -lphonon
LDFLAGS_RELEASE = -flto

# --- Obiekty ---
COMMON_OBJS = \
    src/qcommon/cmd.o src/qcommon/cmodel.o src/qcommon/common.o \
    src/qcommon/crc.o src/qcommon/cvar.o src/qcommon/files.o \
    src/qcommon/md4.o src/qcommon/net_chan.o src/qcommon/net.o \
    src/qcommon/pmove.o src/qcommon/q_msg.o \
	src/game/q_shared.o src/game/m_flash.o \
    src/game/q_shared.o \
    src/linux/q_shlinux.o src/linux/sys_linux.o src/linux/glob.o \
    src/include/minizip/ioapi.o src/include/minizip/unzip.o

SERVER_OBJS = \
    src/server/sv_ccmds.o src/server/sv_ents.o src/server/sv_game.o \
    src/server/sv_init.o src/server/sv_main.o src/server/sv_send.o \
    src/server/sv_user.o src/server/sv_world.o

CLIENT_OBJS = \
    src/client/cl_cin.o src/client/cl_decals.o src/client/cl_demo.o \
    src/client/cl_draw.o src/client/cl_ents.o src/client/cl_fx.o \
    src/client/cl_http.o src/client/cl_input.o src/client/cl_inv.o \
    src/client/cl_loc.o src/client/cl_main.o src/client/cl_newfx.o \
    src/client/cl_parse.o src/client/cl_pred.o src/client/cl_scrn.o \
    src/client/cl_tent.o src/client/cl_view.o src/client/console.o \
    src/client/keys.o src/client/snd_dma.o src/client/snd_mix.o \
    src/client/snd_hrtf.o \
    src/client/vid_sdl.o \
    src/linux/cd_linux.o src/linux/qgl_linux.o src/linux/rw_linux.o \
    src/linux/rw_sdl.o src/linux/snd_sdl.o \
    src/ref_gl/gl_decal.o src/ref_gl/gl_decal_svg.o src/ref_gl/gl_draw.o \
    src/ref_gl/gl_image.o src/ref_gl/gl_light.o src/ref_gl/gl_mesh.o \
    src/ref_gl/gl_model.o src/ref_gl/gl_rmain.o src/ref_gl/gl_rmisc.o \
    src/ref_gl/gl_rsurf.o src/ref_gl/gl_warp.o \
    src/ui/qmenu.o src/ui/ui_addressbook.o src/ui/ui_atoms.o \
    src/ui/ui_controls.o src/ui/ui_credits.o src/ui/ui_demos.o \
    src/ui/ui_dmoptions.o src/ui/ui_download.o src/ui/ui_game.o \
    src/ui/ui_joinserver.o src/ui/ui_keys.o src/ui/ui_loadsavegame.o \
    src/ui/ui_main.o src/ui/ui_multiplayer.o src/ui/ui_newoptions.o \
    src/ui/ui_playerconfig.o src/ui/ui_quit.o src/ui/ui_startserver.o \
    src/ui/ui_video.o

GAME_OBJS = \
    src/game/g_ai.o src/game/g_chase.o src/game/g_cmds.o src/game/g_combat.o \
    src/game/g_core_moves.o src/game/g_func.o src/game/g_items.o \
    src/game/g_main.o src/game/g_misc.o src/game/g_monster.o \
    src/game/g_move_reg.o src/game/g_phys.o src/game/g_save.o \
    src/game/g_spawn.o src/game/g_svcmds.o src/game/g_target.o \
    src/game/g_trigger.o src/game/g_turret.o src/game/g_utils.o \
    src/game/g_weapon.o src/game/m_actor.o src/game/m_berserk.o \
    src/game/m_boss2.o src/game/m_boss31.o src/game/m_boss32.o \
    src/game/m_boss3.o src/game/m_brain.o src/game/m_chick.o \
    src/game/m_flash.o src/game/m_flipper.o src/game/m_float.o \
    src/game/m_flyer.o src/game/m_gladiator.o src/game/m_gunner.o \
    src/game/m_hover.o src/game/m_infantry.o src/game/m_insane.o \
    src/game/m_medic.o src/game/m_move.o src/game/m_mutant.o \
    src/game/m_parasite.o src/game/m_soldier.o src/game/m_supertank.o \
    src/game/m_tank.o src/game/p_client.o src/game/p_hud.o \
    src/game/p_trail.o src/game/p_view.o src/game/p_weapon.o \
    src/game/q_shared.o

# --- Targety ---
all: debug

bin:
	mkdir -p bin

debug: bin
	$(MAKE) targets CFLAGS="$(DEBUG_CFLAGS)" LDFLAGS_REL=""

release: bin
	$(MAKE) targets CFLAGS="$(RELEASE_CFLAGS)" LDFLAGS_REL="$(LDFLAGS_RELEASE)"

targets: bin/q2classic bin/q2game.so

# --- Reguły ---
src/game/%.o : src/game/%.c
	@mkdir -p $(shell dirname $@)
	@echo " [CC] $(shell basename $@)"
	@$(CC) $(CFLAGS) -fPIC -o $@ -c $<

src/%.o : src/%.c
	@mkdir -p $(shell dirname $@)
	@echo " [CC] $(shell basename $@)"
	@$(CC) $(CFLAGS) -o $@ -c $<

bin/q2classic : $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS)
	@echo "[LD] $@"
	@$(CC) -o $@ $^ $(LDFLAGS_COMMON) $(LDFLAGS_CLIENT) $(LDFLAGS_REL)

bin/q2game.so : $(GAME_OBJS)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) -shared -o $@ $^ -Wl,--no-as-needed $(LDFLAGS_REL)

clean:
	@echo "[CLEAN]"
	@rm -rf src/*/*.o src/*/*/*.o bin/*