# AGENT_NOTES — Q2classic (Linux)

Skondensowana baza wiedzy technicznej projektu dla przyszłych sesji. Nie zastępuje `AGENTS.md` (dyrektywy pracy i stylu), lecz dokumentuje architekturę, decyzje projektowe, znane pułapki i konfigurację środowiska.

---

## 1. Środowisko, build i pliki referencyjne

- **Platforma:** Linux Mint 22.2 (x86_64), `gcc 13.3.0`, SDL2, libcurl, libpng, libjpeg, zlib, GLU, Steam Audio (`libphonon.so` ładowane dynamicznie przez `dlopen`).
- **Build (`Makefile`):**
  - `make debug` (target domyślny `all`) — `-g` (bez `-O`, nie aktywuje `-Wmaybe-uninitialized` ani `-Wrestrict`).
  - `make release` — `-O3 -ffast-math -flto -DNDEBUG`.
  - **Zalecana weryfikacja zmian:** kompilacja z `-O2 -Wall -Wextra -Wpedantic` (bez LTO — ujawnia `-Wmaybe-uninitialized`, `-Wrestrict`, `-Wclobbered`) oraz `make release` (ujawnia `-Wlto-type-mismatch`).
  - **Artefakty:** `bin/q2classic` i `bin/q2game.so`.
- **Środowisko uruchomieniowe operatora:** `/home/user/.quake2/`
  - Dane gry: `baseq2/pak0..3.pak`, `baseq2/customs_fryziu.pkz`, `openffa/`.
  - `Sys_GetGameAPI` ładuje `q2game.so` z katalogu gry (`FS_NextPath`, np. `/home/user/.quake2/baseq2/q2game.so`), a nie z `bin/`. Do pełnego testu operator kopiuje oba pliki z `bin/`.
  - Nie modyfikować plików w `/home/user/.quake2/` bez wyraźnego polecenia.
- **Katalog `reference/` (WYŁĄCZNIE DO ODCZYTU):**
  - `reference/quake2idsoftware/` — oryginalny Quake 2 (3.21).
  - `reference/r1q2-ai/` — referencyjny klient/serwer R1Q2.
- **Serwer testowy MP:** `217.182.73.153:27910` (`q2proded`, mod `OpenFFA`, protokół 34).

---

## 2. Podsystem dźwięku (`src/client/snd_*`, `src/linux/snd_sdl.c`)

### Założenia projektowe
- **Stopniowanie unowocześnień:**
  1. Tryb bazowy (`s_hrtf 0`, `s_sound_overlap 0`) ma zachowywać brzmienie i zachowanie oryginalnego Quake 2 (klasyczna liniowa atenuacja + liniowy panning w `S_SpatializeOrigin`, oryginalne nadpisywanie kanałów encji).
  2. Rozszerzenia włączane stopniowo przez użytkownika: `s_sound_overlap 1`, `s_hrtf 1` (Steam Audio 4.8), `s_occlusion 1` (raycast `CM_BoxTrace`), a w przyszłości opcjonalne krzywe/filtry (np. s-curve) w ścieżce HRTF.
- **Każda zmiana w `s_sound_overlap` i miksowaniu wymaga testu odsłuchowego w grze** — nie zmieniać reguł nakładania bez potrzeby.
- **Niestandardowe długości dźwięków w pakach operatora (`pak3.pak` / `customs_fryziu.pkz`):**
  - `items/protect.wav`: ~28.5 s (celowo długi — ma towarzyszyć graczowi przez czas działania Invulnerability i zdradzać jego pozycję).
  - `misc/power1.wav`: ~6.6 s.
  - `items/damage.wav`: ~2.1 s (Quad gra dodatkowo `damage3.wav` przy każdym strzale — nie wydłużać ani nie dopuszczać nadmiernej kakofonii).

### Architektura pozycji źródeł i dziedziczenia (`snd_loc.h`, `snd_dma.c`, `snd_mix.c`, `snd_hrtf.c`)
- `channel_t.origin_mode` (`snd_origin_mode_t`):
  - `SND_ORIGIN_ENTITY` — ciągłe śledzenie `CL_GetEntitySoundOrigin(ch->entnum)` z cache ostatniej znanej pozycji w `ch->origin`.
  - `SND_ORIGIN_TRACK` — dla pakietów `SND_ENT | SND_POS` (wysyłanych m.in. przez `q2proded`/R1Q2 gdy gracz jest poza PVS odbiorcy) dotyczących graczy (`1..cl.maxclients`) i ruchomych bmodeli (`solid == 31`). Gdy encja pojawia się w PVS (`serverframe == cl.frame.serverframe`), dźwięk płynnie ją śledzi i aktualizuje `ch->origin` jako fallback.
  - `SND_ORIGIN_FIXED` — stały punkt (wybuchy, `entnum == 0`).
- **Dziedziczenie dźwięku przedmiotów (`S_TryInheritItemSound`):** dla kanału `CHAN_ITEM` w trybie `SND_ORIGIN_FIXED`, jeśli źródłowa encja przedmiotu zniknęła z PVS (`entnum == 0` lub `serverframe != cl.frame.serverframe`) w oknie `250 ms` od startu, własność przejmuje najbliższy aktywny gracz w promieniu `72` jednostek (`ch->entnum = p`, `SND_ORIGIN_TRACK`). Chroni mody emitujące dźwięk z encji podniesionego przedmiotu.
- **Lokalny gracz i dźwięki 2D:** `ch->entnum == cl.playernum + 1` oraz `ch->dist_mult <= 0` (`ATTN_NONE`) w obu backendach (Software i HRTF) omijają okluzję i spatializację kierunkową.
- **Format próbek w cache (`S_LoadSound`):** konwersja do `AUDIO_S16LSB @ dma.speed` z zachowaniem oryginalnej liczby kanałów (`1 ch` dla mono, `2 ch` dla stereo) oraz parsowaniem chunka `cue ` (`sc->loopstart` skalowane do `dma.speed`).
- **Oba backendy korzystają z jednego ring buffera bez blokad (`s_ringbuffer`, 64 KB)** zasilającego callback SDL (`src/linux/snd_sdl.c`).
- **`s_hrtf 1` wymaga wyjścia stereo (`dma.channels == 2`)**; przy `s_force_mono 1` następuje fallback do backendu `software`.

---

## 3. Pętla główna, zegar monotoniczny, `cl_async`, mysz SDL2 i VSync

- **Zegar (`src/linux/q_shlinux.c`, `src/linux/sys_linux.c`, `src/qcommon/common.c`):**
  - `Sys_Microseconds()` i `Sys_Milliseconds()` korzystają z `clock_gettime(CLOCK_MONOTONIC, ...)`.
  - `main()` w `sys_linux.c` liczy przyrosty w mikrosekundach z akumulatorem reszty (`msec_acc_us`) i wywołuje `Qcommon_Frame(time)` także z `time == 0` przy >1000 FPS; `Qcommon_Frame` nie wymusza już `frame_msec = 1` dla `msec == 0` i wywołuje `SV_Frame` wyłącznie gdy `frame_msec > 0 || dedicated->integer`.
  - Oszczędzanie CPU sterowane jest przez `sys_sleep` (`0` = spin-pause, `1` = `nanosleep` z marginesem 1000 µs, `2` = margines 250 µs, maks. kwant snu 2 ms) na podstawie `CL_NextFrameWaitUsec()`. Gdy `r_maxfps == 0` w `cl_async 1`, funkcja zwraca `0` (rytm wyznacza VSync lub pełna prędkość).
- **Tryby `CL_Frame` (`src/client/cl_main.c`, `src/client/cl_input.c`, `src/client/cl_pred.c`):**
  - `cl_async` (domyślnie `1`, `CVAR_ARCHIVE`): `1` rozdziela wysyłanie pakietów (`cl_maxfps`, zakres `5..1000`) od renderowania (`r_maxfps`, `0` = bez limitu lub `>= 5`); `0` sprzęga pakiet i render pod `cl_maxfps`.
  - W `cl_async 1` zegar `cl.time` zwiększany jest wyłącznie o surowe `msec` (raz na iterację), nigdy o skumulowane `packet_delta`.
  - `Sys_SendKeyEvents()`, `IN_Commands()`, `Cbuf_Execute()`, `CL_ReadPackets()` oraz `CL_UpdateCmd(cl_input_msec, input_dt)` wykonują się przed każdą klatką renderowania i pakietową (a polling co 1 ms w czasie oczekiwania, aby działało `cl_instantpacket`).
  - `cl.cmd_msec` (`int` w `client_state_t`, zerowane przez `CL_ClearState()` i `CL_FinalizeCmd()`) chroni `byte cl.cmd.msec` przed zawinięciem powyżej 255 ms oraz przed pakietami 0 ms (serwery R1Q2 odrzucają `msec > 250`).
  - `CL_PredictMovement()` zeruje lokalne `pm` i aktualizuje `pm.viewangles` z `cl.viewangles + delta_angles` również gdy `cl.cmd.msec == 0`.
- **Mysz SDL2 (`src/linux/rw_sdl.c`, `src/linux/rw_linux.c`):**
  - `HandleEvents()` akumuluje delty (`SDL_GetRelativeMouseState(&dx, &dy); mx += dx; my += dy;`) zamiast nadpisywać `mx, my`.
  - `IN_Move()` skaluje mysz w lokalnych `float` (`fmx`, `fmy`) i zachowuje surowe `old_mouse_x`/`old_mouse_y`.
  - `GLimp_AppActivate()` pomija wywołania bez zmiany stanu i zeruje bufor względny SDL przy przełączeniu.
- **VSync SDL2 (`src/linux/rw_sdl.c`, `src/ref_gl/gl_rmisc.c`, `src/ref_gl/gl_rmain.c`):**
  - `GL_UpdateSwapInterval()` wywołuje `GLimp_SetSwapInterval()` -> `SDL_GL_SetSwapInterval()` dynamicznie bez `vid_restart`. Domyślna wartość `gl_swapinterval` to `1` (wyłączana ręcznie np. pod `timedemo 1`).

---

## 4. Ładowanie zasobów (anti-stutter), VFS cache i limity encji

- **Odroczone i przyrostowe ładowanie (`src/client/cl_view.c`, `src/client/cl_parse.c`, `src/client/cl_main.c`):**
  - `cl_defermodels` (domyślnie `1`, `CVAR_ARCHIVE`): przy `CL_PrepRefresh()` natychmiast rejestrowany jest świat (`CS_MODELS+1`), wszystkie inline bmodele `*N`, `CS_IMAGES`, `cl.baseclientinfo` oraz niebo; zewnętrzne `.md2`/`.sp2` oraz `CS_PLAYERSKINS` ładują się po 1 elemencie/kroku na klatkę w `CL_LoadDeferredAssets()` (po `SCR_UpdateScreen()`), a po zakończeniu wywoływane jest `R_EndRegistration()`. `GL_FlushScrap()` w `R_SetGL2D()` zapewnia aktualność atlasu `scrap` w trakcie odroczonej rejestracji.
  - W trakcie aktywnej gry (`cls.state == ca_active`) `CL_ParseConfigString()` kolejkuje dynamiczne `CS_MODELS`, `CS_SOUNDS` i zmiany `CS_PLAYERSKINS` (`cl_ci_jobs`: model -> skin/ikona -> kolejne `w_*.md2` -> sexed sounds dla customowych modeli). Zmiana samego nicku (bez zmiany części po `\`) nie przeładowuje modeli.
  - `CL_RegisterTEntSounds()` i `CL_RegisterSounds()` preładują dźwięki muzzleflash, `items/respawn1.wav`, `misc/tele1.wav`, `misc/talk*.wav` oraz sexed sounds dla `male`/`female`/`cyborg` i modeli z `CS_PLAYERSKINS`; `MAX_SFX = 1024`, `sfx_t.missing` zapobiega powtarzanym `FS_LoadFile()`.
- **VFS (`src/qcommon/files.c`):**
  - `fs_cache` (domyślnie `1`, `CVAR_ARCHIVE`) utrzymuje statyczny negative cache (2048 bucketów / 4096 wpisów), czyszczony przez `FS_FlushCache()` (komenda `fs_flushcache`, oraz automatycznie przy `FS_Restart()`, zapisie, zmianie nazwy i zakończeniu pobrań). `FS_LoadFile(path, NULL)` używa szybkiej ścieżki `FS_FileExistsLength()` (`stat()` / indeks hash pak/pkz).
- **Limity encji i parsowanie ramek (`src/client/client.h`, `src/client/ref.h`, `src/client/cl_ents.c`, `src/client/cl_parse.c`):**
  - `MAX_PARSE_ENTITIES = 16384` (`UPDATE_BACKUP * MAX_EDICTS`), `MAX_VISIBLE_ENTITIES = MAX_EDICTS` (`1024`), `MAX_ENTITIES = 2048`.
  - W `CL_ParseFrame()` próg delty wynosi `MAX_PARSE_ENTITIES - MAX_EDICTS`; nieważne ramki nie zaśmiecają pierścienia `cl_parse_entities`; `U_REMOVE` nie przesuwa `oldindex` przy `oldnum != newnum`; `CL_ParseZPacket()` waliduje `compressed_len`, granice bufora i wynik dekompresji.

---

## 5. Znane pułapki i otwarte drobiazgi

1. **Czyszczenie ostrzeżeń w kodzie Q2 bez klamr:** w historii projektu przy usuwaniu nieużywanych zmiennych zdarzyło się zostawienie osieroconego `if` (`sv_world.c:SV_PointContents`) i usunięcie `emptyleaf` (`cmodel.c:CM_InitBoxHull`). Zawsze weryfikować logikę względem `reference/` przy edycji plików z silnika podstawowego.
2. **Menu opcji audio (`src/ui/ui_controls.c:126,230,284`):** pozycja `"sound quality"` odczytuje i ustawia historyczny cvar `sndspeed` zamiast używanego przez `snd_sdl.c` cvara `s_khz`.
3. **Konflikt `CHAN_ITEM` w `src/game/g_items.c:790` (`Touch_Item`):** w DM z `DF_INSTANT_ITEMS` i `s_sound_overlap 0`, `items/pkup.wav` jest wywoływane na `CHAN_ITEM` zaraz po `Use_Invulnerability` / `Use_Quad` (również `CHAN_ITEM`).
4. **LTO w `Makefile`:** flaga `-flto` w `RELEASE_CFLAGS` / `LDFLAGS_RELEASE` powoduje ostrzeżenie `lto-wrapper` o serial LTRANS jobs (można w przyszłości zmienić na `-flto=auto`).
5. **Dokumentacja:** `docs/Changelog_2026-06-14.md` został włączony do głównego `docs/CHANGELOG.md`.
