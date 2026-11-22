# Q2classic — Przewodnik komend i ustawień dla gracza

![Platform](https://img.shields.io/badge/Platform-Linux%20(SDL2%20%2F%20OpenGL)-informational)
![Render](https://img.shields.io/badge/Render-Async%20%2F%20High%20Hz-success)
![Status](https://img.shields.io/badge/Dokumentacja-Rozwijana-blue)

Praktyczny zbiór najważniejszych komend i zmiennych konsolowych (`cvar`) dla graczy Q2classic. Pełna techniczna lista wszystkich zmiennych silnika znajduje się w [`docs/CVARS.md`](CVARS.md).

> [!NOTE]
> Wszystkie polecenia wpisuje się w konsoli gry (domyślnie klawisz `~`) lub w pliku konfiguracyjnym (`q2config.cfg` / `autoexec.cfg`). Zmienne oznaczone jako **zapisywane** są automatycznie utrwalane w `q2config.cfg`.

---

## Spis treści

- [Obraz, płynność i synchronizacja](#obraz-płynność-i-synchronizacja)
- [Tryb okna, rozdzielczość i monitor](#tryb-okna-rozdzielczość-i-monitor)
- [Gotowe profile ustawień](#gotowe-profile-ustawień)
- [Diagnostyka i benchmark (`timedemo`)](#diagnostyka-i-benchmark-timedemo)

---

## Obraz, płynność i synchronizacja

Q2classic rozdziela częstotliwość wysyłania pakietów sieciowych do serwera (`cl_maxfps`) od częstotliwości renderowania klatek obrazu (`r_maxfps` / `gl_swapinterval`). Dzięki temu obraz może działać płynnie w 144 / 240 / 260+ Hz niezależnie od limitu pakietów sieciowych (np. `cl_maxfps 125` lub `cl_maxfps 60`).

| Zmienna | Domyślna | Zakres / Wartości | Zapisywana | Opis |
| :--- | :---: | :--- | :---: | :--- |
| `cl_async` | `1` | `0` / `1` | Tak | **`1`** — asynchroniczne renderowanie (niezależny rytm obrazu i sieci). **`0`** — klasyczny tryb synchroniczny (obraz i pakiety sprzężone z `cl_maxfps`). |
| `r_maxfps` | `0` | `0` lub `>= 5` | Tak | Limit klatek renderowanych na sekundę (aktywny przy `cl_async 1`). **`0`** wyłącza sztuczny limit programowy (płynność wyznacza VSync przy `gl_swapinterval 1` lub pełna moc GPU przy `0`). |
| `cl_maxfps` | `125` | `5` – `1000` | Tak | Przy `cl_async 1`: liczba pakietów ruchu wysyłanych do serwera na sekundę (zalecane `60`–`125`). Przy `cl_async 0`: wspólny limit FPS obrazu i sieci. |
| `gl_swapinterval` | `1` | `0`, `1`, `-1` | Tak | Synchronizacja pionowa (VSync) przełączana w locie bez restartu wideo. **`1`** — VSync włączony, **`0`** — VSync wyłączony, **`-1`** — Adaptive VSync (z automatycznym powrotem do `1`, jeśli sterownik nie wspiera). |
| `sys_sleep` | `1` | `0`, `1`, `2` | Tak | Kontrola usypiania wątku (`nanosleep`) między klatkami dla ograniczenia obciążenia CPU: **`0`** — wyłączone (minimalny jitter, wysokie użycie jednego rdzenia), **`1`** — tryb hybrydowy (margines 1 ms), **`2`** — tryb oszczędny (margines 0.25 ms, zalecany na starsze CPU). |
| `cl_defermodels` | `1` | `0` / `1` | Tak | **`1`** — szybsze wejście na mapę: od razu ładowany jest świat i ruchome elementy mapy (drzwi, windy), a zewnętrzne modele oraz skiny graczy dociągają się przyrostowo po 1 elemencie na klatkę bez przycięć. **`0`** — pełne ładowanie przed wejściem na mapę. Ignorowane podczas `timedemo 1`. |
| `fs_cache` | `1` | `0` / `1` | Tak | **`1`** — pamięć podręczna brakujących plików w systemie plików (eliminuje powtarzane odczyty dysku dla nieistniejących tekstur/dźwięków przy `gl_replacepcx 1` i `cl_vwep 1`). Czyści się automatycznie po pobraniu lub zapisie pliku; ręczne czyszczenie komendą `fs_flushcache`. |
| `fs_flushcache` | *(komenda)* | — | — | Ręcznie czyści pamięć podręczną `fs_cache` (przydatne po ręcznym dorzuceniu plików do katalogu gry bez restartu klienta). |

> [!IMPORTANT]
> **Fizyka Quake 2 i zgodność z serwerami:** przy `cl_async 1` zalecaną wartością `cl_maxfps` jest **`125`** (klasyczny rytm skoków i ruchu Q2) lub **`60`–`100`** na serwerach z restrykcyjnymi limitami pakietów. Nie ustawiaj `cl_maxfps` wyżej niż wymaga tego serwer — od płynności obrazu na monitorach 144/240/260+ Hz służą `gl_swapinterval` oraz `r_maxfps`.

> [!TIP]
> Zmiana `gl_swapinterval` działa natychmiast w kolejnej klatce i **nie wymaga** `vid_restart`. Jeśli chcesz wykonać test wydajności `timedemo 1`, wyłącz ręcznie VSync poleceniem `gl_swapinterval 0`, a po teście wróć do `gl_swapinterval 1`.

---

## Tryb okna, rozdzielczość i monitor

| Komenda / Zmienna | Domyślna | Wartości | Zapisywana | Opis |
| :--- | :---: | :--- | :---: | :--- |
| `vid_fullscreen` | `1` | `0` / `1` | Tak | **`1`** — pełny ekran (domyślnie borderless dopasowany do monitora), **`0`** — tryb okienkowy. |
| `vid_fullscreen_exclusive` | `0` | `0` / `1` | Tak | Przy `vid_fullscreen 1`: **`0`** — pełny ekran bez ramki (*borderless desktop*), **`1`** — tryb wyłączny (*exclusive fullscreen*). |
| `vid_displayindex` | `0` | `0`, `1`, ... | Tak | Indeks monitora, na którym ma zostać otwarte okno gry w konfiguracji wielomonitorowej. |
| `gl_mode` | `-1` | `-1` lub `0`–`13` | Tak | Predefiniowana rozdzielczość wideo (`0`–`13`) lub **`-1`** dla rozdzielczości własnej z `r_customwidth` × `r_customheight`. |
| `r_customwidth` | `0` | `0` lub `>= 320` | Tak | Własna szerokość obrazu (dla `gl_mode -1`). |
| `r_customheight` | `0` | `0` lub `>= 240` | Tak | Własna wysokość obrazu (dla `gl_mode -1`). |
| `vid_restart` | *(komenda)* | — | — | Ponownie inicjalizuje okno SDL2 i kontekst OpenGL po zmianie rozdzielczości, trybu pełnoekranowego lub monitora. |

> [!NOTE]
> Po zmianie `vid_fullscreen`, `vid_fullscreen_exclusive`, `vid_displayindex`, `gl_mode`, `r_customwidth` lub `r_customheight` wykonaj `vid_restart`, aby zastosować nowy tryb.

---

## Gotowe profile ustawień

### Monitor 144 / 240 / 260+ Hz z VSync (ustawienie domyślne)
```cfg
cl_async 1
cl_maxfps 125
r_maxfps 0
gl_swapinterval 1
sys_sleep 1
```

### Monitor 240 / 260+ Hz bez VSync (sztywny limit klatek)
```cfg
cl_async 1
cl_maxfps 125
r_maxfps 260
gl_swapinterval 0
sys_sleep 1
```

### Starszy sprzęt (np. Core 2 Duo + GeForce GT 730) — niższe obciążenie CPU
```cfg
cl_async 1
cl_maxfps 100
r_maxfps 0
gl_swapinterval 1
sys_sleep 2
```
*(Przy wyłączonym VSync można ustawić np. `r_maxfps 125` lub `r_maxfps 144` wraz z `sys_sleep 2`.)*

### Klasyczny tryb synchroniczny (jak w oryginalnym Quake 2)
```cfg
cl_async 0
cl_maxfps 125
gl_swapinterval 0
sys_sleep 1
```

---

## Diagnostyka i benchmark (`timedemo`)

<details>
<summary><b>Pokaż komendy diagnostyczne i procedurę testu <code>timedemo 1</code></b></summary>

| Komenda / Zmienna | Wartości | Opis |
| :--- | :--- | :--- |
| `cl_fps` | `0`, `1`, `2` | Wyświetlanie licznika FPS na ekranie (`0` wyłączony, `1`/`2` kolor). |
| `timedemo` | `0` / `1` | Tryb pomiaru średniego FPS podczas odtwarzania dema. |
| `cl_showmiss` | `0` / `1` | Wypisuje w konsoli ewentualne rozbieżności predykcji ruchu gracza. |
| `showclamp` | `0` / `1` | Wypisuje komunikaty `high clamp` / `low clamp` synchronizacji czasu serwera. |

#### Pomiar wydajności na wbudowanym demie (`demo1.dm2`):
```cfg
gl_swapinterval 0
r_maxfps 0
timedemo 1
map demo1.dm2
```
Po zakończeniu pomiaru wróć do normalnej gry:
```cfg
timedemo 0
gl_swapinterval 1
```

> [!WARNING]
> Pozostawienie `gl_swapinterval 1` podczas `timedemo 1` ograniczy wynik pomiaru do częstotliwości odświeżania monitora (np. 60 / 144 / 240 FPS).

</details>
