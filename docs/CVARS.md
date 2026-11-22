# Q2classic — Client Console Variables

Reference for console variables (`cvars`) registered by the Linux client
executable produced by this tree (`bin/q2classic`). The data below was taken
from the registration and usage sites in `src/client`, `src/ui`, `src/ref_gl`,
`src/qcommon` and `src/linux`.

## Scope

* Covered: every cvar registered by a translation unit linked into
  `bin/q2classic` (see `Makefile`, `COMMON_OBJS`, `SERVER_OBJS`,
  `CLIENT_OBJS`).
* The client links the server objects, so listen-server cvars exist in the
  same cvar table. They are listed separately in
  [Appendix A](#appendix-a--listen-server-cvars) and do not affect a client
  connected to a remote server.
* Game-module cvars (`gi.cvar`) are listed in
  [Appendix B](#appendix-b--game-module-cvars-listen-server-only). They are
  registered only after a local game library is loaded.
* Names that are written by the UI but never registered, and registrations
  that are commented out or unused in this build, are listed in
  [Unregistered, no-op and legacy names](#unregistered-no-op-and-legacy-names).

## Flags and notation

| Flag | Value | Meaning in this tree |
|---|---|---|
| `A` | `CVAR_ARCHIVE` | Written to `config.cfg` by `Cvar_WriteVariables`. |
| `U` | `CVAR_USERINFO` | Mirrored into the player userinfo string sent to the server. |
| `S` | `CVAR_SERVERINFO` | Mirrored into server info (listen server). |
| `L` | `CVAR_LATCH` / `CVAR_LATCHED` | Change is deferred until the subsystem restarts (video/sound/game). |
| `C` | `CVAR_CHEAT` | Console writes are rejected while `cheats` is `0`. |
| `N` | `CVAR_NOSET` | Console/`Cvar_SetByVar` writes are rejected. |
| `R` | `CVAR_ROM` | Read-only for normal writes. |
| `—` | `0` | No flags. |

Range notation:

* **engine** — enforced in code (`OnChange`, clamp, or a validation at use
  time). Values outside the range are corrected or rejected.
* **UI** — limit of a menu slider/spin control only. The console can set
  values outside it unless an engine clamp is also shown.
* **none** — no explicit range check in this tree.

`Cvar_Set` / `Cvar_SetValue` call `Cvar_SetByVar(..., true)` and therefore
bypass `CVAR_CHEAT`, `CVAR_ROM` and `CVAR_NOSET`. Menu writes use that path.
Console writes use `Cvar_SetByVar(..., false)` and honour those flags.

## Player identity and userinfo

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `name` | `unnamed` | A, U | engine: max 15 chars; empty becomes `unnamed` | Player name in userinfo. `OnChange_Name` truncates at 15 characters. |
| `skin` | `male/grunt` | A, U | none | Player model/skin string. Written by the player setup menu. |
| `rate` | `10000` | A, U | server: 100..15000 (default 5000 if absent) | Requested network rate in bytes/s. Enforced by `SV_UserinfoChanged`. |
| `msg` | `1` | A, U | none | Message filter level sent to the server. |
| `hand` | `0` | A, U | none (UI 0..2) | View handedness: `0` right, `1` left, `2` center. `2` hides the view weapon unless `cl_gun_show_center` is `1`. |
| `fov` | `90` | A, U | engine: 1..179 in `CalcFov`; game module clamps to 1..160, `<1` becomes `90` | Horizontal field of view. |
| `gender` | `male` | A, U | none | Player gender token for obituary messages. |
| `gender_auto` | `1` | A | none | Enables automatic gender selection in the player setup menu. |
| `password` | *(empty)* | U | none | Server password sent in userinfo. |
| `spectator` | `0` | U | none | Requests spectator mode on connect. |
| `cl_clan` | *(empty)* | — | none | Clan tag inserted into demo file names when longer than 2 characters. |
| `cl_vwep` | `1` | A | none | Draws all weapon models held by other players instead of only the active one. |
| `cl_hudalpha` | `1` | A | UI 0..1 | Alpha of HUD icons and status-bar graphics. |
| `cl_gunalpha` | `1` | — | none | Alpha of the first-person weapon model; applied when `< 1.0`. |
| `cl_protocol` | `35` | A | engine: `34` (vq2) or `35` (r1q2); any other non-zero value is reset to `35` | Protocol requested for the next connection. `0` leaves the choice to the server. |
| `cl_instantpacket` | `1` | — | none | Sends input packets immediately instead of batching them in `CL_SendCmd`. |
| `cl_http_proxy` | *(empty)* | — | none | Proxy string passed to libcurl (`CURLOPT_PROXY`). |
| `cl_http_filelists` | `1` | — | none | Enables HTTP download of server file lists and `.bsp` files. |
| `cl_http_downloads` | `1` | — | none | Enables the HTTP download path when a download server is known. |
| `cl_http_max_connections` | `2` | — | engine: 1..4 | Maximum concurrent HTTP downloads. Values above 2 print a specification warning. |
| `adr0` | `tastyspleen.net` | A | none | Address-book slot 0 used by the server browser. |
| `adr1` | `217.182.73.153` | A | none | Address-book slot 1. |
| `adr2`, `adr3`, `adr4`, `adr5`, `adr6`, `adr7`, `adr8` | *(empty)* | A | none | Remaining address-book slots. |
| `menu_serversort` | `1` | A | UI spin control | Server-browser sort mode. |

## Input and movement

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `sensitivity` | `3` | A | UI 1..11 (slider 2..22 divided by 2) | Mouse sensitivity multiplier. |
| `m_pitch` | `0.022` | A | none (UI forces sign) | Vertical mouse-to-pitch scale. The controls menu stores the absolute value and applies the sign from the invert-mouse box. |
| `m_yaw` | `0.022` | — | none | Horizontal mouse-to-yaw scale. |
| `m_forward` | `1` | — | none | Scale applied to forward/back movement when mouse-looking is active. |
| `m_side` | `1` | — | none | Scale applied to strafe movement. |
| `cl_run` | `1` | A | UI 0..1 | Always-run toggle. |
| `freelook` | `1` | A | UI 0..1 | Free mouse look without holding `+mlook`. |
| `lookspring` | `0` | A | UI 0..1 | Re-centres the view when mouse look is released. |
| `lookstrafe` | `0` | A | UI 0..1 | Mouse X strafes instead of turning. |
| `cl_upspeed` | `200` | — | none | Swim/up movement speed. |
| `cl_forwardspeed` | `200` | — | none | Forward movement speed. |
| `cl_sidespeed` | `200` | — | none | Strafe movement speed. |
| `cl_yawspeed` | `140` | — | none | Keyboard turn speed (degrees/s). |
| `cl_pitchspeed` | `150` | — | none | Keyboard pitch speed (degrees/s). |
| `cl_anglespeedkey` | `1.5` | — | none | Multiplier for keyboard turn speed while the run key is held. |
| `m_filter` | `0` | — | none | Averages current and previous mouse delta (2-sample smoothing). |
| `m_autosens` | `0` | — | none | Scales mouse movement by `fov_x/90` and `fov_y/90`. |
| `m_accel` | `0` | — | none | Mouse acceleration factor added to `sensitivity`. |
| `mevdev` | `/dev/input/event2` | A | none | evdev device path. Compiled only with `WITH_EVDEV`; that macro is not set by the `Makefile`, so this is a no-op in the default build. |
| `in_mouse` | `1` | A | none | Registered by `rw_linux.c`; not read by the Linux input path in this tree. |
| `in_dgamouse` | `1` | A | none | Registered by `rw_linux.c`; not read by the Linux input path in this tree. |

## Client core, prediction and network

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `cl_nodelta` | `0` | — | none | Disables delta compression; requests full snapshots. |
| `cl_maxpackets` | `0` | — | engine: when non-zero, `cl_maxfps/3 .. cl_maxfps` | Caps outgoing command packets per second. `0` disables the cap. Adjusted in `CL_SendCmd`. |
| `r_maxfps` | `0` | A | engine: `0` (unlimited) or `>= 5` | Renderer frame cap in `cl_async 1` mode. `0` disables the software cap (paced by VSync when `gl_swapinterval` is `1`, or uncapped when `0`). |
| `cl_maxfps` | `125` | A | engine: `5..1000` | Network command packet rate in `cl_async 1`, or coupled frame/packet cap in `cl_async 0`. |
| `cl_async` | `1` | A | none (boolean) | `1` decouples rendering (`r_maxfps`/VSync) from network packets (`cl_maxfps`); `0` runs synchronous frames coupled to `cl_maxfps` and enables `cl_maxpackets` throttling when set. |
| `cl_timeout` | `120` | — | none | Seconds without server traffic before the connection is dropped. |
| `cl_shownet` | `0` | — | none | Network debug output level (`1` summary, `2`/`3` more detail). |
| `cl_showmiss` | `0` | — | none | Prints prediction miss distances. |
| `showclamp` | `0` | — | none | Prints server-time high/low clamp events. |
| `paused` | `0` | C | none | Pauses the local client simulation. |
| `timedemo` | `0` | C | none | Timedemo mode; prints average FPS on completion. |
| `rcon_password` | *(empty)* | — | none | Password for `rcon` commands. |
| `rcon_address` | *(empty)* | — | none | Optional `host:port` for `rcon` when not connected. |
| `r_lightlevel` | `0` | — | none | Output cvar: current light level written by the renderer. |
| `cl_predict` | `1` | — | none | Enables client-side player movement prediction. |
| `cl_gun` | `2` | — | none | View weapon: `0` off, `1` hidden above 90° FOV, `2` always drawn with FOV offset. |
| `cl_gun_x` | `0` | — | none | View weapon X offset. |
| `cl_gun_y` | `0` | — | none | View weapon Y offset. |
| `cl_gun_z` | `0` | — | none | View weapon Z offset. |
| `cl_gun_show_center` | `0` | A | none | When `hand` is `2`, `0` hides the view weapon and `1` shows it. |
| `cl_footsteps` | `1` | — | none | Enables predicted footstep sounds. |
| `cl_noskins` | `0` | — | none | Ignores custom player skins. |
| `cl_blend` | `1` | — | none | Enables damage/pickup screen blends. |
| `cl_lights` | `1` | — | none | Enables dynamic lights. |
| `cl_particles` | `1` | — | none | Enables particles. |
| `cl_entities` | `1` | — | none | Enables entity rendering. |
| `cl_stereo_separation` | `0.4` | A | engine: 0..1 | Stereo separation; values outside the range are clamped. |
| `cl_stereo` | `0` | — | none | Enables stereo rendering. |
| `cl_railcolor_r` | `0.3` | A | none | Red component of the rail trail colour. |
| `cl_railcolor_g` | `0.5` | A | none | Green component of the rail trail colour. |
| `cl_railcolor_b` | `1.0` | A | none | Blue component of the rail trail colour. |
| `cl_wsfov` | `1` | — | none | Corrects vertical FOV for wide aspect ratios. |
| `cl_stats` | `0` | — | none | Prints per-frame client statistics. |
| `cl_defermodels` | `1` | A | none (boolean) | Defers loading of external map models (`.md2`/`.sp2`) and player skins/vwep until after entering the level (1 step per frame in `CL_LoadDeferredAssets`); world and inline brush models (`*N`) load immediately. Ignored during `timedemo 1`. |
| `cl_testblend` | `0` | — | none | Test mode: fills the screen blend. |
| `cl_testparticles` | `0` | — | none | Test mode: spawns 4096 particles. |
| `cl_testentities` | `0` | — | none | Test mode: spawns test entities. |
| `cl_testlights` | `0` | C | none | Test mode: spawns dynamic lights. |
| `cl_drawlocs` | `0` | — | none | Draws location names loaded from `loc_dir`. |
| `loc_dir` | `locs` | — | none | Directory containing `.loc` files; reloaded on change. |

## HUD and screen

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `viewsize` | `100` | A | engine: 40..100 | Screen coverage percentage; also sets `scr_vrect`. UI slider is 40..100. |
| `scr_conheight` | `0.5` | A | engine: 0.1..1.0 | Console drop height as a fraction of the screen. UI slider 0.1..1.0. |
| `scr_conspeed` | `3` | — | none | Console open/close speed. |
| `scr_showturtle` | `0` | — | none | Shows the turtle icon when the client is running slowly. |
| `scr_showpause` | `1` | — | none | Shows the pause icon. |
| `scr_centertime` | `2.5` | — | none | Seconds a centre print remains visible. |
| `scr_printspeed` | `8` | — | none | Characters per second for centre-print typewriter effect. |
| `netgraph` | `0` | — | none | Draws the network graph. |
| `timegraph` | `0` | — | none | Draws the time graph. |
| `debuggraph` | `0` | — | none | Draws the debug graph. |
| `graphheight` | `32` | — | none | Graph height in pixels. |
| `graphscale` | `1` | — | none | Graph value scale. |
| `graphshift` | `0` | — | none | Graph vertical shift. |
| `scr_drawall` | `0` | — | none | Forces all screen elements to redraw; set automatically by the renderer during `vid_restart`. |
| `scr_draw2d` | `1` | — | none | Enables 2D HUD drawing. |
| `crosshair` | `1` | A | UI 0..8 (new options), 0..3 (controls menu); no engine clamp | Crosshair number. `0` disables it; `1`..`8` load `ch1`..`ch8`. |
| `ch_alpha` | `1` | A | UI 0..1 | Crosshair alpha. |
| `ch_pulse` | `0` | A | UI 0..4 | Crosshair alpha pulse rate. |
| `ch_scale` | `2` | A | UI 1..8 | Crosshair scale. |
| `ch_red` | `1` | A | UI 0..1 | Crosshair red component. |
| `ch_green` | `1` | A | UI 0..1 | Crosshair green component. |
| `ch_blue` | `1` | A | UI 0..1 | Crosshair blue component. |
| `ch_health` | `0` | — | none | Colours the crosshair by player health instead of `ch_red`/`ch_green`/`ch_blue`. |
| `ch_x` | `0` | — | none | Crosshair X offset in pixels. |
| `ch_y` | `0` | — | none | Crosshair Y offset in pixels. |
| `cl_clock` | `0` | A | UI 0..2 | Clock display: `0` off, `1`/`2` select colour. |
| `cl_clockx` | `0` | — | none | Clock X position; `0` with `cl_clocky` `0` uses the default corner. |
| `cl_clocky` | `0` | — | none | Clock Y position. |
| `cl_clockformat` | `%H:%M:%S` | — | none | `strftime` format for the clock. |
| `cl_maptime` | `0` | A | UI 0..2; values > 10 select round time | Map timer display. Values `>10` show time since round start and use `value-10` as colour. |
| `cl_maptimex` | `0` | — | none | Map timer X position. |
| `cl_maptimey` | `0` | — | none | Map timer Y position. |
| `cl_fps` | `0` | A | UI 0..2 | FPS counter: `0` off, `1`/`2` select colour. |
| `cl_fpsx` | `0` | — | none | FPS counter X position. |
| `cl_fpsy` | `0` | — | none | FPS counter Y position. |
| `cl_chathud` | `0` | A | UI 0..2 | Chat HUD: `0` off, `1`/`2` select colour. |
| `cl_chathudx` | `0` | — | none | Chat HUD X position. |
| `cl_chathudy` | `0` | — | none | Chat HUD Y position. |
| `cl_chathudlines` | `4` | A | engine: 1..8 | Number of chat lines retained in the HUD. |
| `cl_chathudtime` | `15` | A | none | Seconds a chat HUD line remains visible. |
| `scr_drawlagometer` | `0` | A | none | Draws the lagometer. |
| `scr_drawlagometer_x` | `0` | — | none | Lagometer X position. |
| `scr_drawlagometer_y` | `0` | — | none | Lagometer Y position. |

## Console

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `con_notifytime` | `3` | — | none | Seconds a notify line stays on screen. |
| `con_notifylines` | `4` | A | engine: 0..8 (`CON_TIMES`) | Maximum notify lines. UI slider 1..8. |
| `con_notifyfade` | `0` | A | UI 0..1 | Enables fade-out of notify lines. |
| `con_scrlines` | `2` | A | engine: minimum 1, no upper clamp | Console scroll step in lines. UI slider 1..10. |
| `con_mwheel` | `1` | — | none | Enables mouse-wheel scrolling in the console. |
| `con_cmdcomplete` | `2` | — | none | Command completion mode. |
| `con_alpha` | `0.6` | A | UI 0..1 | Console background alpha. |

## Chat, ignore and highlighting

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `cl_timestamps` | `0` | A | UI 0..2 | Message timestamps: `0` off, `1` chat only, `2` all messages. |
| `cl_timestampsformat` | `[%H:%M:%S]` | — | none | `strftime` format for message timestamps. |
| `cl_ignoremode` | `1` | — | none | Ignore-list mode: `0` disabled, `1` normal, `2` inverse (ignore everything not listed). |
| `cl_highlight` | `0` | A | none (bit mask) | Highlight enable mask; bit `2` applies text colours. |
| `cl_highlightmode` | `0` | — | none | `0` matches the visible text, `1` matches the full line. |
| `cl_highlightcolor` | `0` | A | engine: 0..7 | Default highlight colour index. |
| `cl_highlightnames` | `0` | — | none | Applies highlighting to player names. |
| `cl_textcolors` | `0` | A | none | Enables colour codes in chat text. |
| `cl_mychatcolor` | `0` | A | engine: 0..7 | Colour index for the local player's chat. |
| `cl_chat_notify` | `1` | — | none | Enables chat notifications while the console is closed. |
| `cl_autoscreenshot` | `0` | — | none | On a detected limit message: `1` runs the `screenshot` command, `2` runs `screenshotjpg`. |
| `cl_customlimitmsg` | `timelimit hit` | — | none | Additional substring that triggers the limit-message screenshot path. |

## Demos

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `cl_demotimescale` | `1` | — | engine: 0..1000 | Demo playback speed multiplier; `0` pauses playback. |
| `cl_autorecord` | `0` | — | none | Automatically records a demo when a map starts. |
| `cl_clan` | *(empty)* | — | none | Clan tag used in automatic demo file names. |

## Video and window (SDL)

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `vid_fullscreen` | `1` | A | none (treated as boolean) | `1` borderless/fullscreen window, `0` windowed. |
| `vid_fullscreen_exclusive` | `0` | A | none (treated as boolean) | With `vid_fullscreen` `1`, requests exclusive SDL fullscreen. |
| `vid_gamma` | `1.0` | A | UI brightness 0.5..1.3 (mapped) | Gamma applied by the software image path. |
| `vid_displayindex` | `0` | A | engine: must be a valid SDL display index, otherwise `0` | Monitor selected for the window. |
| `vid_xpos` | `3` | A | none | Registered for compatibility; the Linux SDL path does not read it. |
| `vid_ypos` | `22` | A | none | Registered for compatibility; the Linux SDL path does not read it. |

## Renderer — mode, scaling and buffers

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `gl_mode` | `-1` | A | engine: `-1` or `0..13`; invalid modes fall back to the previous mode | Video mode. `-1` uses `r_customwidth` × `r_customheight`. |
| `r_customwidth` | `0` | A | engine: `0` or `>= 320` (non-zero values below 320 become 320) | Custom mode width. |
| `r_customheight` | `0` | A | engine: `0` or `>= 240` (non-zero values below 240 become 240) | Custom mode height. |
| `gl_scale` | `2.0` | A | engine: minimum 1.0, no upper clamp | Internal render scale. Values above 1 render at a lower resolution and upscale; also selects high-resolution console characters. |
| `gl_swapinterval` | `1` | A | none (`0` off, `1` VSync, `-1` adaptive VSync with fallback to `1`) | Vertical-sync interval applied dynamically on Linux/SDL2 via `SDL_GL_SetSwapInterval` without triggering `vid_restart`. |
| `gl_ext_swapinterval` | `1` | A, L | none | Registered but not read in this tree (legacy). |
| `gl_multisample` | `0` | A | none | Changing it triggers `vid_restart`. No renderer code reads the value in this tree. |
| `gl_finish` | `0` | A | none | Calls `qglFinish` after each frame. |
| `gl_drawbuffer` | `GL_BACK` | — | none (string compared with `GL_FRONT`) | Selects the draw buffer. |
| `gl_driver` | `libGL.so` | A, L | none | OpenGL library name. On failure the renderer retries the built-in name and resets the cvar. |
| `gl_allow_software` | `0` | L | none | Allows the software renderer fallback. |
| `gl_3dlabs_broken` | `1` | A, L | none | Applies the 3Dlabs driver workaround. |
| `gl_vertex_arrays` | `1` | A | none | Registered but not read in this tree (legacy). |
| `gl_bitdepth` | `0` | A, L | none | Registered but not read in this tree (legacy). |

### Video modes (`gl_mode`)

| Value | Resolution |
|---|---|
| `-1` | `r_customwidth` × `r_customheight` |
| `0` | 320 × 240 |
| `1` | 400 × 300 |
| `2` | 512 × 384 |
| `3` | 640 × 480 |
| `4` | 800 × 600 |
| `5` | 960 × 720 |
| `6` | 1024 × 768 |
| `7` | 1152 × 864 |
| `8` | 1280 × 960 |
| `9` | 1600 × 1200 |
| `10` | 2048 × 1536 |
| `11` | 1024 × 480 |
| `12` | 1280 × 768 |
| `13` | 1280 × 1024 |

## Renderer — textures and images

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `gl_texturemode` | `GL_LINEAR_MIPMAP_LINEAR` | A | one of the six names below | Texture filter mode. |
| `gl_texturebits` | `0` | A, L | only `16` and `32` change behaviour; anything else uses the default path | Requested texture bit depth. |
| `gl_round_down` | `1` | A, L | none (boolean) | Rounds non-power-of-two textures down. |
| `gl_picmip` | `0` | A, L | none (used as a right shift; UI 0..3) | Texture downscaling. UI stores `3 - slider`. |
| `gl_skymip` | `0` | A, L | none | Reduces sky texture mip levels. |
| `gl_gammapics` | `1` | A, L | none (boolean) | Applies gamma to pictures as well as world textures. |
| `gl_texturelighting` | `1` | A | none (boolean) | Enables texture lighting. Change takes effect on `vid_restart`. |
| `gl_intensity` | `2.666` | A | engine: minimum 1 (set during image init) | Lightmap intensity. |
| `gl_brightness` | `1.666` | A | none | Image brightness. Change takes effect on `vid_restart`. |
| `gl_contrast` | `1.0` | A | none | Image contrast. Change takes effect on `vid_restart`. |
| `gl_saturation` | `1.0` | A | none | Image saturation. Change takes effect on `vid_restart`. |
| `gl_monochrome` | `0` | — | none (boolean) | Monochrome image mode. Change takes effect on `vid_restart`. |
| `gl_invert` | `0` | — | none (boolean) | Inverts image colours. |
| `gl_modulate` | `1.666` | A | none | Global lightmap/dynamic-light multiplier. |
| `gl_coloredlightmaps` | `1` | A, L | none (boolean) | Enables coloured lightmaps. |
| `gl_replacewal` | `1` | A, L | none (boolean) | Allows `.wal` replacement by external images. |
| `gl_replacepcx` | `1` | A, L | none (boolean) | Allows `.pcx` replacement by external images. |
| `gl_replacemd2` | `0` | A, L | none (boolean) | Allows `.md2` replacement by external models. |
| `gl_ext_texture_compression` | `0` | A, L | none (boolean) | Enables texture compression when the extension is present. |
| `gl_ext_texture_filter_anisotropic` | `1` | A, L | none (boolean) | Enables anisotropic filtering when available. |
| `gl_ext_max_anisotropy` | `999` | A | engine: 0..hardware maximum when anisotropic filtering is active | Anisotropy level. |
| `gl_sgis_mipmap` | `1` | L | none (boolean) | Enables the SGIS mipmap generation path. |
| `gl_screenshot_quality` | `85` | A | engine: 1..100 (clamped inside `WriteJPG`) | JPEG quality for screenshots. |

### Accepted `gl_texturemode` values

`GL_NEAREST`, `GL_LINEAR`, `GL_NEAREST_MIPMAP_NEAREST`,
`GL_LINEAR_MIPMAP_NEAREST`, `GL_NEAREST_MIPMAP_LINEAR`,
`GL_LINEAR_MIPMAP_LINEAR`.

## Renderer — lighting, shadows and effects

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `gl_shadows` | `2` | A | none (values used: `0`, `1`, `2`) | `0` no shadows; non-zero draws blob shadows; `2` additionally uses the stencil path when stencil is available. |
| `gl_dynamic` | `1` | — | none (boolean) | Dynamic lightmaps. |
| `gl_flashblend` | `0` | — | none (boolean) | Renders dynamic lights as blended sprites instead of lightmap updates. |
| `gl_polyblend` | `1` | — | none (boolean) | Enables polygon screen blends. |
| `gl_stainmaps` | `1` | A, L | none (boolean) | Enables stain maps. |
| `gl_fog` | `0` | — | none (boolean) | Enables GL fog. |
| `gl_fog_density` | `1` | — | none | Fog density; passed as `value * 2 / 4096`. |
| `gl_watercaustics` | `1` | — | none (boolean) | Enables water caustics. |
| `gl_waterwaves` | `1` | A | engine: 0..4 | Water wave amplitude. |
| `gl_shelleffect` | `1` | A | none (boolean) | Enables the shell effect texture. |
| `gl_celshading` | `0` | A | none (boolean) | Enables cel shading. |
| `gl_celshading_width` | `4` | A | none | Cel-shading outline width. |
| `gl_motionblur` | `0` | — | none (boolean) | Enables motion blur when rectangle textures are available. |
| `gl_fontshadow` | `0` | A | none (boolean) | Draws a shadow behind console/HUD text. |
| `gl_minlight_entities` | `1` | — | none (boolean) | Applies a minimum light level to entities. |
| `gl_decals` | `1` | A | none (boolean) | Enables decals. |
| `gl_decals_time` | `30` | A | none | Decal lifetime in seconds. |
| `gl_decals_max` | `256` | A | engine: 256..32768 (`MAX_DECALS`) | Maximum active decals. Changing it reallocates the decal pool. |
| `gl_decals_debug` | `0` | A | none | Decal debug output level. |
| `skydistance` | `4096` | A | none (clamped internally to a 65536 upper bound) | Skybox distance used to compute the far plane. |

## Renderer — particles

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `gl_particle` | `1` | A | none (boolean) | Enables particle rendering. |
| `gl_particle_scale` | `2` | — | none | Particle time scale; used as `1.5 * value`. |
| `gl_particle_size` | `40` | A | none | Point size when point parameters are unavailable. |
| `gl_particle_min_size` | `2` | A | none | `GL_POINT_SIZE_MIN_EXT`. |
| `gl_particle_max_size` | `40` | A | none | `GL_POINT_SIZE_MAX_EXT`. |
| `gl_particle_att_a` | `0.01` | A | none | Point attenuation constant term. |
| `gl_particle_att_b` | `0.0` | A | none | Point attenuation linear term. |
| `gl_particle_att_c` | `0.01` | A | none | Point attenuation quadratic term. |

## Renderer — debug, culling and world effects

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `r_norefresh` | `0` | — | none (boolean) | Skips view rendering. |
| `r_fullbright` | `0` | C | none (boolean) | Fullbright world rendering. |
| `r_drawentities` | `1` | — | none (boolean) | Draws entities. |
| `r_drawworld` | `1` | C | none (boolean) | Draws world surfaces. |
| `r_novis` | `0` | — | none (boolean) | Disables PVS culling. |
| `r_nocull` | `0` | — | none (boolean) | Disables polygon culling. |
| `r_lerpmodels` | `1` | — | none (boolean) | Interpolates model frames. |
| `r_speeds` | `0` | — | none | Prints renderer speed counters. |
| `r_lightlevel` | `0` | — | none | Output cvar written by the renderer. |
| `gl_clear` | `0` | — | none (boolean) | Clears the colour buffer each frame. |
| `gl_cull` | `1` | — | none (boolean) | Enables face culling. |
| `gl_ztrick` | `0` | — | none (boolean) | Uses the z-trick depth clear. |
| `gl_showtris` | `0` | — | none (boolean) | Draws world triangles. |
| `gl_nobind` | `0` | — | none (boolean) | Disables texture binding for performance tests. |
| `gl_lockpvs` | `0` | C | none (boolean) | Locks the PVS for debugging. |
| `gl_lightmap` | `0` | C | none (boolean) | Shows lightmaps directly. |
| `gl_eff_world_wireframe` | `0` | C | none (boolean) | World wireframe effect. |
| `gl_eff_world_bg_type` | `0` | C | none | World background effect type. |
| `gl_eff_world_bg_color_r` | `255` | — | none; `< 256` is divided by 255, `>= 256` randomises | Background effect red component. |
| `gl_eff_world_bg_color_g` | `255` | — | none; `< 256` is divided by 255, `>= 256` randomises | Background effect green component. |
| `gl_eff_world_bg_color_b` | `255` | — | none; `< 256` is divided by 255, `>= 256` randomises | Background effect blue component. |
| `gl_eff_world_lines_color_r` | `0` | — | none; `< 256` is divided by 255, `>= 256` randomises | Wireframe red component. |
| `gl_eff_world_lines_color_g` | `255` | — | none; `< 256` is divided by 255, `>= 256` randomises | Wireframe green component. |
| `gl_eff_world_lines_color_b` | `0` | — | none; `< 256` is divided by 255, `>= 256` randomises | Wireframe blue component. |
| `flushmap` | `0` | — | none (boolean) | Forces the world model to be reloaded even when the map name matches the cached one. |

## Renderer — GL extensions

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `gl_ext_multitexture` | `1` | A, L | none (boolean) | Enables ARB/SGIS multitexture. |
| `gl_ext_pointparameters` | `0` | A, L | none (boolean) | Enables `GL_EXT_point_parameters`. |
| `gl_ext_compiled_vertex_array` | `1` | A, L | none (boolean) | Enables compiled vertex arrays. |

## Sound

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `s_initsound` | `1` | L | none (boolean) | Initialises the sound subsystem. `0` skips sound. |
| `s_hrtf` | `0` | A, L | none (boolean) | Selects the HRTF sound backend. Forced to `0` if HRTF initialisation fails. |
| `s_occlusion` | `0` | A, L | none (boolean) | Enables HRTF occlusion. |
| `s_volume` | `0.7` | A | UI 0..1; no engine clamp | Master sound volume. Values above `1.0` amplify and may clip. |
| `s_khz` | `44100` | A, L | engine: only `44100` or `48000`; anything else is forced to `44100` | Output sample rate. |
| `s_mixahead` | `0.03` | A | engine: minimum 512 samples, no upper clamp | Target buffer level in seconds. Converted to samples with `value * dma.speed`. |
| `s_show` | `0` | — | none | Sound debug output; `2` uses the verbose path. |
| `s_swapstereo` | `0` | A | none (boolean) | Swaps left/right channels. |
| `s_ambient` | `1` | — | none (boolean) | Enables ambient sound processing while connected. |
| `s_force_mono` | `0` | A, L | none (boolean) | Forces a mono output device; change triggers `snd_restart`. |
| `s_sound_overlap` | `0` | A | none (boolean) | Allows more than one sound on the same entity channel. |

## Filesystem, system and common

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `basedir` | `.` | N | none | Base directory used by the filesystem and the GL loader. Registered in `files.c` and again with `CVAR_NOSET` in `qgl_linux.c`. |
| `game` | *(empty)* | L, S | none | Game directory name. |
| `fs_developer` | `0` | — | none (boolean) | Filesystem debug output. |
| `fs_allpakloading` | `1` | A | none (boolean) | Loads all `.pak` files instead of stopping at the first complete set. |
| `fs_cache` | `1` | A | none (boolean) | Enables negative-lookup hash cache in the virtual filesystem to avoid repeated disk scans for missing files. Cleared automatically on `FS_Restart`, file writes/renames and downloads, or manually via `fs_flushcache`. |
| `fs_usehomedir` | `1` | — | none (boolean) | Uses the home directory for the game data path. |
| `developer` | `0` | — | none | General developer output level. |
| `timescale` | `1` | C | engine: maximum 400 | Simulation time multiplier. |
| `fixedtime` | `0` | C | none | Fixed frame time in milliseconds; `0` uses real time. |
| `logfile` | `0` | — | none | `1` buffers the console log to `qconsole.log`, `2` flushes after each print. |
| `showtrace` | `0` | — | none | Prints a stack trace on `Com_Error`. |
| `host_speeds` | `0` | — | none | Prints server/frame timing breakdown. |
| `dedicated` | `0` | N | none | Dedicated-server flag. `CVAR_NOSET` in the normal client build. |
| `version` | build version | S, R | none | Engine version string. |
| `map_noareas` | `0` | — | none (boolean) | Disables area checks in the collision model. |
| `net_maxmsglen` | `1390` | — | engine: 0..4086 (`MAX_MSGLEN - PACKET_HEADER`) | Maximum network message length. |
| `net_ignore_icmp` | `0` | — | none (boolean) | Ignores ICMP port-unreachable errors. |
| `net_port` | `27901` (`PORT_CLIENT`) | — | none | Client UDP port. |
| `ip` | `localhost` | N | none | Bind address for server sockets. |
| `ip_hostport` | `0` | N | none | Overrides the server port. |
| `hostport` | `0` | N | none | Server port override used when `ip_hostport` is `0`. |
| `port` | `27910` (`PORT_SERVER`) | N | none | Server port. |
| `qport` | random | N | none | Per-connection network channel identifier. |
| `showpackets` | `0` | — | none | Prints sent/received packet summaries. |
| `showdrop` | `0` | — | none | Prints dropped/reliable packet events. |
| `nostdout` | `0` | — | none (boolean) | Suppresses output to stdout on Linux. |
| `sys_sleep` | `1` | A | none (`0` off, `1` hybrid 1000 µs margin, `>=2` 250 µs margin) | Controls `nanosleep` yielding in the Linux main loop while waiting for the next packet or render deadline. |

## Unregistered, no-op and legacy names

### Names written by the UI but never registered by the engine

`Cvar_SetValue` creates a user-created cvar when the name does not exist.
These names are therefore present at runtime but are not registration-backed
settings:

| Name | Written by | Effect in this tree |
|---|---|---|
| `sndspeed` | `ui_controls.c` | No engine reader; the real control is `s_khz`. |
| `sw_mode` | `ui_video.c` | Software-renderer mode; no software renderer is built. |
| `sw_stipplealpha` | `ui_video.c` | Software-renderer option; unused. |
| `gamerules` | `ui_game.c`, `ui_startserver.c` | No engine reader in this build. |
| `in_joystick` | `ui_controls.c` | Joystick support is not built on Linux. |

### Registrations commented out or not linked

| Name | Reason |
|---|---|
| `gl_playermip`, `gl_monolightmap`, `gl_nosubimage` | Registration lines are commented out in `gl_rmain.c`. |
| `gl_colorbits`, `gl_alphabits`, `gl_depthbits`, `gl_stencilbits` | Registration block is commented out in `gl_rmain.c`. |
| `cddir` | Registration is commented out in `files.c`. |
| `cl_autoskins` | Registration is commented out in `cl_main.c`. |
| `scr_drawlagometer_alpha` | Registration is commented out in `cl_draw.c`. |
| `vid_xpos`, `vid_ypos`, `in_mouse`, `in_dgamouse` | Registered, but the Linux SDL input/window path does not read them. |
| `gl_vertex_arrays`, `gl_bitdepth`, `gl_ext_swapinterval` | Registered, but not read by the renderer in this tree. |

## Appendix A — Listen-server cvars

Registered by `src/server` in the same executable. They are relevant only
when hosting a local server. Several names (`paused`, `timedemo`,
`showclamp`, `rcon_password`, `password`) are shared with the client table.

| Cvar | Default | Flags | Range / clamp | Function |
|---|---|---|---|---|
| `skill` | `1` | — | game-module dependent | Difficulty level. |
| `deathmatch` | `0` | L | none | Deathmatch mode. |
| `coop` | `0` | L | none | Cooperative mode. |
| `dmflags` | `DF_INSTANT_ITEMS` | S | none (bit mask) | Deathmatch rule flags. |
| `fraglimit` | `0` | S | none | Frag limit. |
| `timelimit` | `0` | S | none | Time limit in minutes. |
| `cheats` | `0` | S, L | none (boolean) | Enables cheat-protected commands. |
| `protocol` | `34` | S, N | none | Server protocol version. |
| `maxclients` | `1` | S, L | UI 0..maxclients (self-clamp) | Maximum clients. |
| `hostname` | `noname` | S, A | none | Server name. |
| `timeout` | `125` | — | none | Client timeout in seconds. |
| `zombietime` | `2` | — | none | Seconds before a zombie client is removed. |
| `showclamp` | `0` | — | none | Server time-clamp messages. |
| `paused` | `0` | — | none | Pauses the server. |
| `timedemo` | `0` | — | none | Server timedemo mode. |
| `sv_enforcetime` | `0` | — | none (boolean) | Enforces server time on clients. |
| `allow_download` | `1` | A | none (boolean) | Master download switch. |
| `allow_download_players` | `0` | A | none (boolean) | Allows player model downloads. |
| `allow_download_models` | `1` | A | none (boolean) | Allows model downloads. |
| `allow_download_sounds` | `1` | A | none (boolean) | Allows sound downloads. |
| `allow_download_maps` | `1` | A | none (boolean) | Allows map downloads. |
| `sv_noreload` | `0` | — | none (boolean) | Prevents reloading the game library on map change. |
| `sv_airaccelerate` | `0` | L | none (boolean) | Enables air acceleration. |
| `public` | `0` | — | none (boolean) | Public-server flag. |
| `sv_reconnect_limit` | `3` | A | none | Reconnect attempts allowed. |
| `rcon_password` | *(empty)* | — | none | Remote-console password. |

## Appendix B — Game module cvars (listen server only)

Registered through `gi.cvar` when a local game library is loaded
(`src/game/g_save.c`). Defaults are those passed at registration; the game
module may clamp or overwrite them at runtime.

| Cvar | Default | Flags | Function |
|---|---|---|---|
| `gun_x` | `0` | — | View weapon X offset. |
| `gun_y` | `0` | — | View weapon Y offset. |
| `gun_z` | `0` | — | View weapon Z offset. |
| `sv_rollspeed` | `200` | — | Roll speed. |
| `sv_rollangle` | `2` | — | Maximum roll angle. |
| `sv_maxvelocity` | `2000` | — | Maximum entity velocity. |
| `sv_gravity` | `800` | — | Gravity. |
| `dedicated` | `0` | N | Dedicated-server flag. |
| `cheats` | `0` | S, L | Cheat switch. |
| `gamename` | `GAMEVERSION` | S, L | Game library name. |
| `gamedate` | `__DATE__` | S, L | Game library build date. |
| `maxclients` | `4` | S, L | Maximum clients. |
| `maxspectators` | `4` | S | Maximum spectators. |
| `deathmatch` | `0` | L | Deathmatch mode. |
| `coop` | `0` | L | Cooperative mode. |
| `skill` | `1` | L | Difficulty level. |
| `maxentities` | `1024` | L | Entity limit. |
| `dmflags` | `0` | S | Deathmatch flags. |
| `fraglimit` | `0` | S | Frag limit. |
| `timelimit` | `0` | S | Time limit. |
| `password` | *(empty)* | U | Server password. |
| `spectator_password` | *(empty)* | U | Spectator password. |
| `needpass` | `0` | S | Whether a password is required. |
| `filterban` | `1` | — | Enables the address filter. |
| `g_select_empty` | `0` | A | Allows selecting empty weapons. |
| `run_pitch` | `0.002` | — | Running pitch bob. |
| `run_roll` | `0.005` | — | Running roll bob. |
| `bob_up` | `0.005` | — | Vertical bob. |
| `bob_pitch` | `0.002` | — | Pitch bob. |
| `bob_roll` | `0.002` | — | Roll bob. |
| `flood_msgs` | `4` | — | Flood-protection message count. |
| `flood_persecond` | `4` | — | Flood-protection rate. |
| `flood_waitdelay` | `10` | — | Flood-protection delay. |
| `sv_maplist` | *(empty)* | — | Map rotation list. |

## Appendix C — Code-enforced clamps (quick reference)

| Cvar | Clamp | Source |
|---|---|---|
| `name` | max 15 chars, empty → `unnamed` | `cl_main.c`, `OnChange_Name` |
| `rate` | 100..15000, default 5000 | `sv_main.c`, `SV_UserinfoChanged` |
| `fov` | 1..179 (`CalcFov`); 1..160 in game module | `q_shared.c`, `p_client.c` |
| `cl_protocol` | 34 or 35 | `cl_main.c`, `OnChange_Protocol` |
| `cl_http_max_connections` | 1..4 | `cl_main.c`, `OnChange_http_max_connections` |
| `cl_stereo_separation` | 0..1 | `cl_main.c`, `OnChange_StereoSeparation` |
| `r_maxfps` | `0` or `>= 5` | `cl_main.c`, `OnChange_MaxFps` |
| `cl_maxfps` | `5..1000` | `cl_main.c`, `OnChange_MaxFps` |
| `cl_maxpackets` | `cl_maxfps/3 .. cl_maxfps` when non-zero | `cl_input.c`, `CL_SendCmd` |
| `viewsize` | 40..100 | `cl_scrn.c`, `OnChange_Viewsize` |
| `scr_conheight` | 0.1..1.0 | `cl_scrn.c`, `OnChange_Conheight` |
| `con_notifylines` | 0..8 | `console.c`, `OnChange_notifyLines` |
| `con_scrlines` | minimum 1 | `console.c`, `OnChange_scrlines` |
| `cl_chathudlines` | 1..8 | `cl_draw.c`, `OnChange_Chathudlines` |
| `cl_highlightcolor`, `cl_mychatcolor` | 0..7 | `cl_parse.c`, `OnChange_Color` |
| `cl_demotimescale` | 0..1000 | `cl_demo.c` |
| `gl_mode` | -1 or 0..13 | `gl_rmain.c`, `R_GetModeInfo` |
| `r_customwidth` | 0 or >= 320 | `gl_rmain.c`, `OnChangeCustomWidth` |
| `r_customheight` | 0 or >= 240 | `gl_rmain.c`, `OnChangeCustomHeight` |
| `gl_scale` | minimum 1.0 | `gl_rmain.c`, `OnChange_Scale` |
| `gl_waterwaves` | 0..4 | `gl_rmain.c`, `OnChange_WaterWaves` |
| `gl_ext_max_anisotropy` | 0..hardware maximum | `gl_rmain.c`, `OnChangeMaxAnisotropy` |
| `gl_intensity` | minimum 1 | `gl_image.c`, `GL_InitImages` |
| `gl_screenshot_quality` | 1..100 | `gl_image.c`, `WriteJPG` |
| `gl_decals_max` | 256..32768 | `gl_decal.c`, `OnChange_DecalsMax` |
| `timescale` | maximum 400 | `common.c`, `OnChange_Timescale` |
| `net_maxmsglen` | 0..4086 | `net_chan.c`, `OnChange_MaxMsgLen` |
| `s_khz` | 44100 or 48000 | `snd_sdl.c`, `SNDDMA_Init` |
| `s_mixahead` | minimum 512 samples | `snd_mix.c`, `S_MixAudio` |
| `vid_displayindex` | valid SDL display index, otherwise 0 | `rw_sdl.c`, `SDLimp_InitGraphics` |
