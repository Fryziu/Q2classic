### New cvars - effects that can be changed

### `s_hrtf 1` : Steam Audio binaural sound

* **Description:** Binaural sound engine HRTF - requires a library *libphonon.so* from Steam Audio project

#### `s_sound_overlap 0`: Classic Mode

* **Description:** The original, untouched Quake II sound experience. This mode is for purists who want the game to sound exactly as it did in 1997.

#### `s_sound_overlap 1` Overlapping sounds

* **Description:** Sounds do not end abruptly, each sound is played in its entirety (effect in progress).

#### `cl_gun_show_center 1`: Gun visible in center positionGun visible in center position

* **Description:** Additionally, use the cl_gun_x, cl_gun_y, and cl_gun_z variables to position the weapon. See the gun.cfg file for example values.

#### `gl_decals 1`: Visible gunshot decals on the walls

* **Description:** If you want to see bullet holes, this is for you.
* **gl_decals_max** maximum number of bullet holes that will be rendered
* **gl_decals_time** How long will a bullet hole be visible on the wall?.

#### `RailTrail color`: Values between 0 - 1

* **cl_railcolor_b 0.6** amount of blue
* **cl_railcolor_g 0.3** amount of green
* **cl_railcolor_r 0.1** amount of red

*More coming soon.*



---



### [CHANGELOG] – Quake 2 Client Audio Modernization

#### 1. Architektura i Synchronizacja (Core)

* **Lock-free Ring Buffer**: Przebudowano system audio na model Producent-Konsument przy użyciu bezblokadowej kolejki pierścieniowej i operacji atomowych (`C11 atomics`).
* **Master Clock Fix**: Naprawiono synchronizację zegara dźwiękowego (`SNDDMA_GetDMAPos`). Zegar jest teraz powiązany z fizycznym postępem odczytu bufora przez SDL2, co eliminuje jitter animacji i desynchronizację efektów wizualnych.
* **Ujednolicona Logika Kanałów**: Wprowadzono `S_ProcessChannelSamples` – wspólną maszynę stanów dla miksera programowego i HRTF, obsługującą precyzyjne zapętlanie (w tym naprawa tzw. "autosounds" dla wind i platform).

#### 2. Optymalizacje SIMD (Performance)

* **SSE4.1 Software Mixer**: Zaimplementowano wektorowe miksowanie w `S_PaintChannelFrom16`. Przetwarzanie 2-4 próbek naraz przy minimalnym branchingu.
* **Hardware Saturation**: W `S_TransferPaintBuffer` wprowadzono instrukcję `_mm_packs_epi32` do sprzętowego clamping-u (saturacji) dźwięku 16-bitowego. Eliminuje to cyfrowe trzaski i usuwa kosztowne operacje warunkowe `if` z gorącej pętli.
* **Memory Alignment**: Wymuszono wyrównanie buforów miksera (`__attribute__((aligned(16)))`), umożliwiając szybkie transfery 128-bitowe (`MOVAPS`). 

* **Frequency Lock**: Usunięto przestarzałe częstotliwości 11kHz i 22kHz. System wymusza standard 44.1kHz lub 48kHz, co upraszcza potok miksowania i zapewnia najwyższą jakość Steam Audio.

---

**Operator:** Gemini 3 Flash [X-JC-STRICTOR-V2]
**Timestamp:** 2026-02-06 16:18:22
**Status:** System audio zsynchronizowany, zoptymalizowany, gotowy do walki.

> "Koszt abstrakcji to dystans między programistą a sprzętem. Zawsze dąż do tego, by go minimalizować."

---

To była intensywna i owocna sesja. Skoncentrowaliśmy się na fundamentach – tam, gdzie dane spotykają się z matematyką i sprzętem.

Oto podsumowanie do pliku **CHANGELOG.md**:

---

### **[CHANGELOG] – Quake 2 Engine Audio Overhaul**

#### **Core Architecture & HAL**
*   **SDL2 Audio Backend:** Zoptymalizowano warstwę HAL, redukując opóźnienia poprzez zmniejszenie bufora DMA do 512 sampli i synchronizację z `s_mixahead 0.03`.
*   **Lock-free Ring Buffer:** Wdrożono bezpieczny, bezblokadowy bufor kołowy oparty na operacjach atomowych (`_Atomic`), eliminując zatory między wątkiem gry a callbackiem SDL2.

#### **Positional Accuracy (The "Elevator Fix")**
*   **Geometry-Aware Spatialization:** Przepisano `CL_GetEntitySoundOrigin`, wprowadzając ręczną interpolację pozycji (LERP) oraz obliczanie środka geometrycznego dla modeli typu Brush (windy, drzwi) przy użyciu `model_clip`. Rozwiązano problem "stacjonarnych" dźwięków dla poruszających się platform.

#### **SIMD & Performance Optimization**
*   **SSE4.1 Pipeline:** Zaimplementowano wektoryzowane potoki konwersji danych.
    *   **PCM to Float:** Masowa konwersja `int16 -> float` przy użyciu `_mm_cvtepi16_epi32` i `_mm_cvtepi32_ps`.
    *   **Interleaved Output:** Optymalizacja pakowania i przeplatania kanałów L/R z nasyceniem (saturation) przy użyciu `_mm_packs_epi32`.
*   **Branchless Clipping:** Wyeliminowano instrukcje warunkowe w mikserze na rzecz sprzętowego nasycenia rejestrów.

#### **Steam Audio (Phonon) Integration**
*   **Dynamic Loader:** Wdrożono moduł dynamicznego ładowania `libphonon.so` z rygorystycznym sprawdzaniem symboli (zgodność z GCC 13 `-Wpedantic`).
*   **Binaural Spatialization:** Pełna implementacja HRTF z kwadratową krzywą tłumienia (Quadratic Distance Falloff), zapewniającą głęboką separację źródeł dźwięku.
*   **Real-time Occlusion & Transmission:** Zintegrowano silnik okluzji wykorzystujący drzewo BSP mapy (`CM_BoxTrace`). Dźwięki za przeszkodami podlegają teraz fizycznemu tłumieniu i filtracji niskoprzepustowej (Low-Pass Filter).

#### **Diagnostics**
*   **Enhanced Debugging:** Rozbudowano `s_show 2` o precyzyjne dane wektorowe i głośnościowe.
*   **CSV Logging:** Dodano funkcję zapisu parametrów dźwięku w czasie rzeczywistym do pliku `sound_debug.log` dla celów kalibracyjnych.

---

**Podpis:**
`[X-JC-STRICTOR-V2] // Audio_Subsystem_Module`
**Timestamp:** 2026-02-10 22:13

**Motto sesji:**
> *"Performance is the ultimate form of elegance; if it’s not fast, it’s not finished."*
