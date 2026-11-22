# Changelog

Historia istotnych zmian w projekcie Q2classic.

---

## [2026-11-22] - Eliminacja mikro przycięć w multiplayer (`cl_defermodels`, cache VFS, precache dźwięków) oraz odporność na przepełnienie encji

### Dodane / Zmienione

- **Odroczone i przyrostowe ładowanie zasobów (`src/client/cl_view.c`, `src/client/cl_parse.c`, `src/client/cl_main.c`, `src/client/client.h`, `src/ref_gl/gl_local.h`, `src/ref_gl/gl_image.c`, `src/ref_gl/gl_rmain.c`):**
  - Dodano cvar `cl_defermodels` (domyślnie `1`, `CVAR_ARCHIVE`): przy wejściu na mapę natychmiast rejestrowany jest świat (`CS_MODELS+1`), wszystkie inline bmodele (`*N` — drzwi, windy, platformy, zero I/O), `CS_IMAGES`, `cl.baseclientinfo` oraz niebo, natomiast zewnętrzne modele `.md2`/`.sp2` oraz skiny/vwep graczy doładowują się przyrostowo w `CL_LoadDeferredAssets()` (maksymalnie 1 zasób/krok na klatkę renderującą, po czym wywoływane jest `R_EndRegistration()`).
  - W trakcie aktywnej gry (`cls.state == ca_active`) zmiany `CS_PLAYERSKINS`, `CS_MODELS` i `CS_SOUNDS` trafiają do kolejki przyrostowej zamiast blokować klatkę; sama zmiana nicku bez zmiany modelu/skina aktualizuje wyłącznie `ci->name`/`ci->cinfo` bez przeładowywania zasobów.
  - Ograniczono częstotliwość `SCR_UpdateScreen()` w pętli `CL_PrepRefresh()` (dla `cl_defermodels 0`) do interwału 100 ms, eliminując narzut VSync na każdym pojedynczym modelu.
  - Dodano `GL_FlushScrap()` wywoływane w `R_SetGL2D()`, zapewniając wgrywanie atlasu `scrap` również podczas trwania odroczonej rejestracji.
- **Precache dźwięków i negative cache audio (`src/client/cl_tent.c`, `src/client/cl_parse.c`, `src/client/snd_dma.c`, `src/client/snd_loc.h`, `src/client/sound.h`):**
  - Rozszerzono `CL_RegisterTEntSounds()` o dźwięki broni z `CL_ParseMuzzleFlash()`, zdarzeń z `CL_EntityEvent()` (`items/respawn1.wav`, `misc/tele1.wav`) oraz czatu (`misc/talk.wav`, `misc/talk1.wav`).
  - W `CL_RegisterSounds()` dodano precache dźwięków zależnych od modelu (`*...` z `CS_SOUNDS` oraz `*fall1.wav`/`*fall2.wav`) dla modeli `male`, `female`, `cyborg` oraz modeli obecnych w `CS_PLAYERSKINS`.
  - Podniesiono `MAX_SFX` z `512` do `1024` i `SND_HASH_SIZE` z `32` do `128`; dodano flagę `sfx_t.missing` (eliminuje ponowne `FS_LoadFile()` dla brakujących plików) oraz usunięto podwójny odczyt VFS w `S_RegisterSexedSound()`.
- **Cache VFS i szybkie sprawdzanie istnienia plików (`src/qcommon/files.c`, `src/qcommon/qcommon.h`, `src/client/cl_parse.c`, `src/client/cl_http.c`):**
  - Dodano cvar `fs_cache` (domyślnie `1`, `CVAR_ARCHIVE`) i komendę `fs_flushcache` (`FS_FlushCache()`) ze statyczną tablicą hashującą negative cache (2048 bucketów / 4096 wpisów), unieważnianą przy `FS_Restart()`, zapisie/zmianie nazwy pliku oraz zakończeniu pobrań UDP/HTTP.
  - Dodano `FS_FileExistsLength()` dla `FS_LoadFile(path, NULL)` (bezpośredni odczyt rozmiaru z indeksu hash dla `.pak`/`.pkz` oraz `stat()` na Linuxie) i pominięto zbędne drugie `fopen()` po `Q_strlwr()`, gdy ścieżka nie zawiera wielkich liter.

### Naprawione

- **Odporność na przepełnienie encji / hordy potworów (`src/client/client.h`, `src/client/ref.h`, `src/client/cl_ents.c`, `src/client/cl_parse.c`):**
  - Podniesiono `MAX_PARSE_ENTITIES` z `1024` do `16384` (`UPDATE_BACKUP * MAX_EDICTS`), `MAX_VISIBLE_ENTITIES` z `512` do `MAX_EDICTS` (`1024`) oraz `MAX_ENTITIES` renderera ze `128` do `2048`.
  - Zaktualizowano próg ważności delty w `CL_ParseFrame()` na `MAX_PARSE_ENTITIES - MAX_EDICTS` i naprawiono brakujące `else` przed `if (oldframe->serverframe != cl.frame.deltaframe)` przy `!oldframe->valid`.
  - W `CL_ParsePacketEntities()` dodano czystą konsumpcję strumienia bez modyfikowania `cl_parse_entities` dla nieważnych ramek (`!newframe->valid`) oraz poprawiono `U_REMOVE` (pominięcie `oldindex++` przy `oldnum != newnum`).
  - W `CL_ParseZPacket()` dodano walidację `compressed_len > MAX_MSGLEN`, przekroczenia końca wiadomości oraz wyniku dekompresji (`sb.cursize <= 0`).

---

## [2026-11-22] - Naprawa asynchronicznego renderowania (`cl_async`), myszy SDL2, VSync i precyzyjny zegar monotoniczny

### Naprawione

- **Asynchroniczna pętla klienta (`src/client/cl_main.c`, `src/client/cl_input.c`, `src/client/cl_pred.c`, `src/client/client.h`):**
  - Usunięto wielokrotne dodawanie skumulowanego `packet_delta` do `cl.time` w klatkach renderowania bez pakietu (poprzednio `cl.time` przyspieszało względem czasu rzeczywistego i blokowało `cl.lerpfrac` na `1.0f`).
  - Przeniesiono próbkowanie wejścia (`Sys_SendKeyEvents`, `IN_Commands`, `Cbuf_Execute`, `CL_ReadPackets`) oraz `CL_UpdateCmd()` poza warunek `if (packet_frame)`, eliminując zduplikowane klatki ruchu gracza i kamery gdy `cl_maxfps != Hz`.
  - Podpięto przełącznik `cl_async` w `CL_Frame()` (domyślnie `1`, `CVAR_ARCHIVE`): `1` rozdziela sieć (`cl_maxfps`) od renderowania (`r_maxfps` / VSync), `0` przywraca klasyczny tryb synchroniczny.
  - Rozdzielono czas klatki renderowania (`cls.frametime`) od czasu pakietu, przekazano jawny `frametime` do `CL_ImmBaseMove()` (z użyciem `CL_ImmKeyState()` zamiast `CL_KeyState()`) oraz dodano pole `cl.cmd_msec` (`int`) eliminujące przepełnienie `byte cl.cmd.msec` (>255 ms) i pakiety 0 ms.
  - W `CL_PredictMovement()` dodano zerowanie struktury `pm` oraz aktualizację `pm.viewangles` z bieżących `cl.viewangles`, gdy `cl.cmd.msec == 0`.
- **Wejście myszy SDL2 (`src/linux/rw_sdl.c`, `src/linux/rw_linux.c`):**
  - W `HandleEvents()` zastąpiono nadpisujące `SDL_GetRelativeMouseState(&mx, &my)` akumulacją (`mx += dx; my += dy;`), eliminując utratę próbek myszy i zaburzenia czułości (`sensitivity`) między klatkami pakietowymi.
  - W `GLimp_AppActivate()` pominięto zbędne wywołania `SDL_SetRelativeMouseMode()` bez zmiany stanu oraz dodano zerowanie bufora względnego przy przełączeniu aktywności.
  - W `IN_Move()` przeniesiono obliczenia `m_filter`, `m_accel`, `sensitivity` i `m_autosens` na typ `float` z zachowaniem surowych `old_mouse_x`/`old_mouse_y`.
- **VSync pod Linux/SDL2 (`src/linux/rw_sdl.c`, `src/ref_gl/gl_local.h`, `src/ref_gl/gl_rmisc.c`, `src/ref_gl/gl_rmain.c`):**
  - Zaimplementowano `GLimp_SetSwapInterval()` na `SDL_GL_SetSwapInterval()` i podpięto je pod `GL_UpdateSwapInterval()` (domyślnie `gl_swapinterval 1`).
  - Usunięto wymuszanie `vid_restart` przy zmianie `gl_swapinterval` — przełączanie VSync (np. `gl_swapinterval 0` pod `timedemo 1`) działa od kolejnej klatki.

### Dodane / Zmienione

- **Zegar monotoniczny i frame pacing (`src/linux/q_shlinux.c`, `src/linux/sys_linux.c`, `src/qcommon/common.c`, `src/game/q_shared.h`, `src/qcommon/qcommon.h`):**
  - Zastąpiono niemonotoniczne `gettimeofday()` funkcją `clock_gettime(CLOCK_MONOTONIC, ...)` i dodano `Sys_Microseconds()` do mikrosekundowego limitowania klatek (wsparcie dla monitorów 240–260+ Hz bez błędów dzielenia całkowitego `1000 / fps`).
  - Dodano `CL_NextFrameWaitUsec()` oraz cvar `sys_sleep` (`CVAR_ARCHIVE`, domyślnie `1`: `0` = bez `nanosleep`, `1` = hybrydowe `nanosleep` z marginesem 1000 µs, `2` = oszczędniejsze `nanosleep` z marginesem 250 µs dla starszych CPU).
  - Zmieniono domyślne wartości i flagi: `cl_async 1` (`CVAR_ARCHIVE`), `r_maxfps 0` (`CVAR_ARCHIVE`, `0` = brak limitu programowego lub `>= 5`), `cl_maxfps 125` (`CVAR_ARCHIVE`, zakres `5..1000`).
- **Dokumentacja (`docs/PLAYER_COMMANDS.md`, `docs/CVARS.md`, `docs/AGENT_NOTES.md`):** dodano poradnik komend gracza dla ustawień obrazu i płynności oraz zaktualizowano dokumentację cvarów i notatki architektoniczne.

---

## [2026-11-22] - Naprawa `SV_PointContents`, kolizji BSP, podsystemu dźwięku i kompilacji bez ostrzeżeń

### Naprawione

- **Serwer / kolizja zawartości (`src/server/sv_world.c`):** usunięto osieroconą instrukcję `if (hit->solid != SOLID_BSP)` w `SV_PointContents`. Wcześniej dla wszystkich encji `SOLID_BSP` (m.in. `func_water`, drzwi, platform) wywołanie `CM_TransformedPointContents` było pomijane, a wynik `contents` był łączony operacją `|=` z niezainicjalizowaną zmienną `c2`. Eliminuje to brak pływania i obrażeń w podnoszonym kwasie na mapie `base3` oraz losowe obrażenia od lawy/kwasu w grze jednoosobowej.
- **Model kolizji BSP (`src/qcommon/cmodel.c`):** przywrócono zmienną `emptyleaf` i jej wyszukiwanie w `CMod_LoadLeafs` oraz poprawiono `CM_InitBoxHull` (`c->children[side] = -1 - emptyleaf` zamiast `-1 - numleafs`), dzięki czemu zewnętrzna strona bounding boxa encji `SOLID_BBOX` nie zwraca już `CONTENTS_MONSTER`. Rozszerzono `map_leafs` do `MAX_MAP_LEAFS + 1` i dodano kontrolę `numleafs + 1 > MAX_MAP_LEAFS`.
- **Dźwięk / śledzenie pozycji i dziedziczenie (`src/client/snd_loc.h`, `src/client/snd_dma.c`, `src/client/snd_mix.c`, `src/client/snd_hrtf.c`):**
  - Wprowadzono tryby źródła pozycji (`SND_ORIGIN_ENTITY`, `SND_ORIGIN_TRACK`, `SND_ORIGIN_FIXED`) i wspólną funkcję `S_GetChannelOrigin`. Dźwięki wysłane przez serwery (Q2Pro/R1Q2/mody) jako `SND_ENT | SND_POS` dla graczy poza PVS (m.in. długi `items/protect.wav` i `misc/power1.wav`) przełączają się na dynamiczne śledzenie gracza po wejściu w PVS zamiast pozostawać w miejscu podniesienia runy.
  - Dodano dziedziczenie dźwięku zdobywanych przedmiotów (`S_TryInheritItemSound`): dźwięki na kanale `CHAN_ITEM` ze znikających encji przedmiotów lub `SND_POS` bez gracza przejmują w oknie 250 ms najbliższego gracza (promień 72 j.).
  - Przywrócono w `S_Spatialize` i dodano w `S_HRTF_MixFrame` bezpośrednie miksowanie 2D dla lokalnego gracza (`entnum == cl.playernum + 1`) i `ATTN_NONE`, eliminując błędną okluzję od ścian obok gracza oraz pozostawanie własnych powerupów w miejscu podniesienia.
  - Przywrócono w `S_SpatializeOrigin` oryginalny liniowy panning i atenuację Quake 2 oraz dodano obsługę `s_swapstereo` w obu backendach (Software i HRTF).
  - Poprawiono konwersję próbek mono/stereo w `S_LoadSound` (próbki mono pozostają 1-kanałowe w cache zamiast dublowania do stereo; dodano sprawdzanie błędu `SDL_BuildAudioCVT` i przywrócono parsowanie chunka `cue ` dla `loopstart`).
  - Poprawiono `S_TransferPaintBuffer` dla trybu mono (`dma.channels == 1` / `s_force_mono 1`).
  - Naprawiono `S_PaintChannels` (usunięto pomijanie `S_Spatialize` i przesuwania `ch->pos` dla kanałów chwilowo poza zasięgiem), `S_PickChannel` (preferowanie wolnych slotów przed wywłaszczaniem aktywnych), `S_StartSound` (`entnum` zamiast `cl_entities[entnum].current.number` w `S_RegisterSexedSound`) i `S_ProcessChannelSamples` (płynne zawijanie `autosound` i zerowanie zakończonych kanałów).
  - W `snd_hrtf.c`: naprawiono próbkowanie końcówek i płynne zapętlanie w `S_HRTF_FillInputBuffer`, przeniesiono obsługę `s_pendingplays` na każdą ramkę 512 próbek w `S_HRTF_Update`, dodano pomijanie DSP/okluzji dla źródeł poza zasięgiem, walidację `dma.channels == 2` w `S_HRTF_Init` oraz zwalnianie buforów `posix_memalign` i zerowanie stanu w `S_HRTF_Shutdown`.
- **Sieć klienta (`src/client/cl_main.c`):** w `CL_ParsePrintMessage` dodano przypisanie `r = &broadcastRequest` przy obsłudze odpowiedzi broadcast (`REQ_PING`), usuwając odczyt niezainicjalizowanego wskaźnika `r->sendtime`.
- **System plików (`src/qcommon/files.c`):** przepisano `FS_FindMaps` na statyczne bufory z kontrolą granic zamiast `strdup`/`strtok` (usunięto wyciek pamięci, ryzyko użycia niezainicjalizowanego `mapName` oraz brak limitu wpisów).
- **UI / LTO (`src/ui/ui_playerconfig.c`):** poprawiono niezgodną deklarację `extern int s_multiplayer_menu` na `extern menuframework_s s_multiplayer_menu` (usunięto `-Wlto-type-mismatch`).
- **UI zapisu gry (`src/ui/ui_loadsavegame.c`):** w `Create_Savestrings` dodano weryfikację wyniku `fread` i jawne zakończenie łańcucha znakiem `'\0'`.
- **Bezpieczeństwo buforów tekstowych:** zastąpiono niebezpieczne wywołania `strncpy`/`strncat` funkcjami `Q_strncpyz`/`Q_strncatz`/`memcpy` w `src/game/g_spawn.c`, `src/game/g_target.c`, `src/ref_gl/gl_image.c` oraz `src/ui/ui_joinserver.c`.
- **Parser poleceń (`src/qcommon/cmd.c`):** w `Cbuf_AddLateCommands` rozdzielono `text` i `build` na osobne alokacje `Z_TagMalloc` (eliminacja ostrzeżenia `-Wrestrict` na `strcat`).
- **Pętla główna (`src/qcommon/common.c`):** w `Qcommon_Frame` wydzielono lokalną zmienną `frame_msec` po `setjmp` (eliminacja `-Wclobbered`).
- **Renderer (`src/ref_gl/gl_local.h`, `src/ref_gl/gl_rmain.c`, `src/ref_gl/gl_rsurf.c`):** zmieniono typ parametru `axis` w `R_RotateForEntity` na `vec3_t *axis` (eliminacja fałszywego ostrzeżenia `-Wstringop-overflow=` w GCC 13).
- **System / Linux (`src/linux/sys_linux.c`):** dodano obsługę wartości zwracanych przez `setreuid`, `setegid` i `seteuid` oraz usunięto zduplikowane wywołanie `fcntl(0, ...)` w `main`.
- **Build (`Makefile`):** usunięto zduplikowany wpis `src/game/q_shared.o` w `COMMON_OBJS`.

### Usunięte

- **Martwy kod i nieużywane cvary audio (`src/client/snd_dma.c`, `src/client/snd_mix.c`, `src/client/snd_hrtf.c`, `src/client/snd_loc.h`, `src/linux/snd_sdl.c`):** usunięto 4 zakomentowane eksperymentalne warianty `S_SpatializeOrigin`, zakomentowaną starą wersję `S_HRTF_MixFrame`, pustą funkcję `S_PaintChannelFrom8`, nieużywane zmienne `sndbits`/`sndspeed`/`sndchannels` oraz nieużywane cvary `s_loadas8bit`, `s_testsound`, `s_primary`, `s_oldresample`, `s_quality`, `s_sdl_resample`.
- **Logi debugujące:** usunięto tymczasowe komunikaty diagnostyczne `//Mentor Log` z `src/game/g_main.c` (`EndDMLevel`, `ExitLevel`), `src/game/g_spawn.c` (`SpawnEntities`) oraz `Sys_GetGameAPI: Trying path:` z `src/linux/sys_linux.c`.

---

## [2026-06-14] - Poprawki protokołu sieciowego i `cl_instantpacket`

### Naprawione

- **Kompatybilność protokołu sieciowego (`src/client/cl_input.c`):** przywrócono przekazywanie `cls.protocolVersion` do `MSG_WriteDeltaUsercmd` zamiast surowego `cls.serverProtocol`, co wyeliminowało błędne kodowanie `clc_move` i rozłączanie z serwerem (`bad clc_move usercmd read` / `read past end of message`).
- **Obsługa `cl_instantpacket` (`src/client/cl_main.c`, `src/client/cl_input.c`):**
  - Przeniesiono `Sys_SendKeyEvents()` na początek `CL_Frame` dla zmniejszenia opóźnień wejściowych.
  - Dodano sprawdzanie `cl.sendPacketNow` przed ogranicznikiem częstotliwości wysyłania pakietów w `CL_Frame`.
  - Poprawiono logikę dławika `cl_maxpackets` w `CL_SendCmd` przez przechwycenie `cl.sendPacketNow` do zmiennej lokalnej przed wyzerowaniem, zapobiegając błędnemu inkrementowaniu `outgoing_sequence` i deformacji kompresji delta.

---

## [2026-03-15] - Clean Compilation (GCC 15 Compatibility & Strict Type Safety)

### Fixed

- **Core/System:** resolved strict ISO C `-Wpedantic` warnings regarding function-to-object pointer casts in `sv_main.c` (`Com_BeginRedirect`), `sys_linux.c` (`Sys_GetGameAPI`), and `g_save.c` using `intptr_t` and `typeof`.
- **OpenGL Subsystem:** fixed `-Wpedantic` pointer cast warnings in `qgl_linux.c` and `gl_rmain.c` extension loading macros using `(typeof(func))(intptr_t)`.
- **Renderer Safety:** addressed `-Wstringop-overflow` in `gl_rsurf.c` for `R_RotateForEntity`.
- **Memory Bounds Checking:** fixed `-Wsign-compare` issues in parsing and rendering loops (`cl_parse.c`, `cl_cin.c`, `cl_demo.c`, `gl_model.c`, `gl_light.c`).
- **UI & Menus:** cleaned up `-Wsign-compare` conflicts in `ui_credits.c`, `ui_atoms.c`, `qmenu.c`, and `ui_playerconfig.c`.
- **Const Correctness:** eliminated `-Wdiscarded-qualifiers` warnings in `cl_main.c`, `ui_joinserver.c`, and `q_shared.c` (`COM_StripExtension`).
- **Text Encoding:** refactored `COM_MakePrintable` in `q_shared.c` to use explicit hexadecimal ASCII values and `unsigned char` casts.
- **Flow Control:** added explicit `/* fallthrough */` comments in `gl_image.c`, `cl_http.c`, and `files.c`.
- **Data Serialization:** added explicit zero-initializers (`-Wmissing-field-initializers`) for `field_t` arrays in `g_save.c`.

### Removed

- **Legacy CD Audio:** removed obsolete Linux CD-ROM playback routines (`cd_linux.c`, `cdaudio.h`) and related `#ifdef` branches.
- **Legacy AVI Export:** removed obsolete `aviDump_t` structures and frame-dumping code (`R_BeginAviDemo`, `R_WriteAviFrame`).

---

## [2026-02-28] - Netcode, Audio & Zero-Warning Cleanup

### Added / Changed

- **Netcode / Input:** implemented sub-frame input sampling bypass in `CL_SendCmd` (`cl_input.c`) so attack button changes trigger immediate packet transmission bypassing `cl_maxpackets` throttling.
- **Audio:** reverted `begin` time in `pendingplay_t` (`snd_mix.c`) to signed `int` to prevent integer underflow on negative time deltas.

### Fixed

- **Build System:** resolved `-Wincompatible-pointer-types` in `ui_atoms.c` with `M_Menu_JoinServer_Wrapper_f`.
- **Types & Safety:** eliminated `-Wsign-compare`, `-Wpedantic`, `-Wimplicit-fallthrough`, `-Wmissing-field-initializers`, and `-Wunused-parameter` warnings across client, renderer, UI, and game modules.
