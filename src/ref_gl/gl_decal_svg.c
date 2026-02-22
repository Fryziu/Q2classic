// Plik: src/ref_gl/gl_decal_svg.c
//
// CEL: Centralne repozytorium dla wszystkich danych SVG.
// Ten plik zawiera FIZYCZNE definicje zmiennych, które są
// DEKLAROWANE jako 'extern' w pliku gl_decal_svg.h.
// To jest standardowa i poprawna praktyka w języku C.

#include "gl_local.h" // Potrzebne dla ew. przyszłych zależności

/// machineGun, schotGun

const char* g_decal_svg_bhole =
"<svg width='64' height='64' viewBox='0 0 128 128' xmlns='http://www.w3.org/2000/svg'>"
  "<defs>"
    "<!-- Gradient symulujacy wglebienie 3D (oswietlenie z lewej-gory) -->"
    "<radialGradient id='grad_crater' cx='50%' cy='50%' r='50%' fx='38%' fy='38%'>"
      "<stop offset='20%' stop-color='#000000' stop-opacity='1' />"
      "<stop offset='45%' stop-color='#1a1c1e' stop-opacity='1' />"
      "<stop offset='80%' stop-color='#8b8e93' stop-opacity='1' />"
      "<stop offset='100%' stop-color='#b5b8bc' stop-opacity='0' />"
    "</radialGradient>"
  "</defs>"

  "<!-- Warstwa 1: Cien pod odpryskami (iluzja grubosci materialu) -->"
  "<path d='M64,10 L70,28 L85,15 L82,35 L105,30 L92,48 L120,60 L96,68 L115,95 L88,85 L95,118 L75,95 L60,122 L54,95 L30,115 L42,85 L10,90 L32,70 L8,50 L32,45 L15,20 L45,32 L48,12 Z' "
        "fill='#000000' opacity='0.3' transform='translate(2, 3)' />"

  "<!-- Warstwa 2: Zewnetrzny obrys odłupanego materiału -->"
  "<path d='M64,10 L70,28 L85,15 L82,35 L105,30 L92,48 L120,60 L96,68 L115,95 L88,85 L95,118 L75,95 L60,122 L54,95 L30,115 L42,85 L10,90 L32,70 L8,50 L32,45 L15,20 L45,32 L48,12 Z' "
        "fill='#c2c5c9' stroke='#7a7c80' stroke-width='1' />"

  "<!-- Warstwa 3: Gradient lejka tworzacy wglebienie 3D -->"
  "<circle cx='64' cy='64' r='38' fill='url(#grad_crater)' />"

  "<!-- Warstwa 4: Wewnetrzna, postrzepiona dziura po pocisku (rdzen) -->"
  "<path d='M64,38 L69,44 L75,40 L78,48 L86,46 L82,54 L88,60 L82,66 L86,74 L78,78 L80,85 L72,82 L66,88 L60,84 L54,90 L50,82 L42,84 L46,76 L38,70 L44,64 L38,56 L46,50 L42,42 L52,46 L56,38 Z' "
        "fill='#000000' />"
"</svg>";
/*
const char* g_decal_svg_bhole =
    "<svg width='64' height='64' xmlns='http://www.w3.org/2000/svg'>"
    "<defs><radialGradient id='grad' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
    "<stop offset='0%' style='stop-color:rgb(0,0,0);stop-opacity:1' />"
    "<stop offset='60%' style='stop-color:rgb(40,40,40);stop-opacity:1' />"
    "<stop offset='100%' style='stop-color:rgb(120,120,120);stop-opacity:0' />"
    "</radialGradient></defs>"
    "<circle cx='32' cy='32' r='28' fill='url(#grad)' />"
    "</svg>";
*/

/// blaster & hyperBlaster
const char* g_decal_svg_blaster =
"<svg width='64' height='64' viewBox='0 0 128 128' xmlns='http://www.w3.org/2000/svg'>"
  "<defs>"
    "<!-- Gradient 1: Szeroka aura przypalenia od fali goraca -->"
    "<radialGradient id='grad_heat' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#1a0a00' stop-opacity='0.8' />"
      "<stop offset='50%' stop-color='#0a0400' stop-opacity='0.4' />"
      "<stop offset='100%' stop-color='#000000' stop-opacity='0' />"
    "</radialGradient>"
    
    "<!-- Gradient 2: Wkleśnięty, stopiony material z efektem 3D -->"
    "<radialGradient id='grad_slag' cx='50%' cy='50%' r='50%' fx='35%' fy='35%'>"
      "<stop offset='0%' stop-color='#4a1500' stop-opacity='1' />"
      "<stop offset='50%' stop-color='#110400' stop-opacity='0.9' />"
      "<stop offset='85%' stop-color='#2b1005' stop-opacity='0.7' />"
      "<stop offset='100%' stop-color='#0a0400' stop-opacity='0.4' />"
    "</radialGradient>"

    "<!-- Gradient 3: Intensywnie swiecacy rdzen (pozostalosc plazmy/lasera) -->"
    "<radialGradient id='grad_glow' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#ffffff' stop-opacity='1' />"
      "<stop offset='15%' stop-color='#ffdd88' stop-opacity='1' />"
      "<stop offset='40%' stop-color='#ff5500' stop-opacity='0.9' />"
      "<stop offset='75%' stop-color='#aa1100' stop-opacity='0.5' />"
      "<stop offset='100%' stop-color='#000000' stop-opacity='0' />"
    "</radialGradient>"
  "</defs>"

  "<!-- Warstwa 1: Osmalenie sciany -->"
  "<circle cx='64' cy='64' r='60' fill='url(#grad_heat)' />"

  "<!-- Warstwa 2: Zewnetrzny, ostygniety stopiony slad (nieregularny kleks) -->"
  "<path d='M64,24 C88,20 104,36 108,56 C112,76 96,104 72,108 C48,112 24,96 20,72 C16,48 36,28 64,24 Z' fill='#0a0502' opacity='0.6' />"

  "<!-- Warstwa 3: Wewnetrzny krater plynnego materialu (symulacja 3D) -->"
  "<path d='M64,32 C82,28 94,40 98,58 C102,74 86,94 68,96 C50,98 32,86 28,66 C24,46 42,36 64,32 Z' fill='url(#grad_slag)' opacity='0.3' />"

  "<!-- Warstwa 4: Swiecacy punkt trafienia (lekko przesuniety dla asymetrii) -->"
  "<circle cx='60' cy='62' r='26' fill='url(#grad_glow)' opacity='0.7'/>"

  "<!-- Warstwa 5: Rozpryski stopionego materialu (detale) -->"
  "<circle cx='35' cy='25' r='6' fill='url(#grad_glow)' opacity='0.7' />"
  "<circle cx='95' cy='85' r='4' fill='url(#grad_glow)' opacity='0.8' />"
  "<circle cx='90' cy='30' r='5' fill='#2b1005' />"
  "<circle cx='25' cy='80' r='3' fill='#110400' />"
"</svg>";

/*
const char* g_decal_svg_blaster =
"<svg width='64' height='64' viewBox='0 0 128 128' xmlns='http://www.w3.org/2000/svg'>"
  "<defs>"
    "<!-- Gradient 1: Szeroka aura przypalenia od fali goraca -->"
    "<radialGradient id='grad_heat' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#1a0a00' stop-opacity='0.8' />"
      "<stop offset='50%' stop-color='#0a0400' stop-opacity='0.4' />"
      "<stop offset='100%' stop-color='#000000' stop-opacity='0' />"
    "</radialGradient>"
    
    "<!-- Gradient 2: Wkleśnięty, stopiony material z efektem 3D -->"
    "<radialGradient id='grad_slag' cx='50%' cy='50%' r='50%' fx='35%' fy='35%'>"
      "<stop offset='0%' stop-color='#4a1500' stop-opacity='1' />"
      "<stop offset='50%' stop-color='#110400' stop-opacity='1' />"
      "<stop offset='85%' stop-color='#2b1005' stop-opacity='1' />"
      "<stop offset='100%' stop-color='#0a0400' stop-opacity='1' />"
    "</radialGradient>"

    "<!-- Gradient 3: Intensywnie swiecacy rdzen (pozostalosc plazmy/lasera) -->"
    "<radialGradient id='grad_glow' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#ffffff' stop-opacity='1' />"
      "<stop offset='15%' stop-color='#ffdd88' stop-opacity='1' />"
      "<stop offset='40%' stop-color='#ff5500' stop-opacity='0.9' />"
      "<stop offset='75%' stop-color='#aa1100' stop-opacity='0.5' />"
      "<stop offset='100%' stop-color='#000000' stop-opacity='0' />"
    "</radialGradient>"
  "</defs>"

  "<!-- Warstwa 1: Osmalenie sciany -->"
  "<circle cx='64' cy='64' r='60' fill='url(#grad_heat)' />"

  "<!-- Warstwa 2: Zewnetrzny, ostygniety stopiony slad (nieregularny kleks) -->"
  "<path d='M64,24 C88,20 104,36 108,56 C112,76 96,104 72,108 C48,112 24,96 20,72 C16,48 36,28 64,24 Z' fill='#0a0502' />"

  "<!-- Warstwa 3: Wewnetrzny krater plynnego materialu (symulacja 3D) -->"
  "<path d='M64,32 C82,28 94,40 98,58 C102,74 86,94 68,96 C50,98 32,86 28,66 C24,46 42,36 64,32 Z' fill='url(#grad_slag)' />"

  "<!-- Warstwa 4: Swiecacy punkt trafienia (lekko przesuniety dla asymetrii) -->"
  "<circle cx='60' cy='62' r='26' fill='url(#grad_glow)' />"

  "<!-- Warstwa 5: Rozpryski stopionego materialu (detale) -->"
  "<circle cx='35' cy='25' r='6' fill='url(#grad_glow)' opacity='0.7' />"
  "<circle cx='95' cy='85' r='4' fill='url(#grad_glow)' opacity='0.8' />"
  "<circle cx='90' cy='30' r='5' fill='#2b1005' />"
  "<circle cx='25' cy='80' r='3' fill='#110400' />"
"</svg>";
*/

/*
const char* g_decal_svg_blaster =
    "<svg width='64' height='64' xmlns='http://www.w3.org/2000/svg'>"
    "<defs><radialGradient id='grad' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
    "<stop offset='0%' style='stop-color:rgb(255,220,180);stop-opacity:1' />"
    "<stop offset='30%' style='stop-color:rgb(255,180,50);stop-opacity:0.9' />"
    "<stop offset='100%' style='stop-color:rgb(200,80,0);stop-opacity:0' />"
    "</radialGradient></defs>"
    "<circle cx='32' cy='32' r='30' fill='url(#grad)' />"
    "</svg>";
*/

/// rocketLauncher
const char* g_decal_svg_scorch =
"<svg width='160' height='160' viewBox='0 0 200 200' xmlns='http://www.w3.org/2000/svg'>"
  "<defs>"
    "<!-- Gradient tla (sadza i pył) -->"
    "<radialGradient id='grad_soot' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#4b4541' stop-opacity='0.9'/>"
      "<stop offset='50%' stop-color='#3f2f24' stop-opacity='0.5'/>"
      "<stop offset='100%' stop-color='#403a3a' stop-opacity='0'/>"
    "</radialGradient>"
    
    "<!-- Gradient epicentrum wybuchu -->"
    "<radialGradient id='grad_core' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#322d2d' stop-opacity='1.0'/>"
      "<stop offset='60%' stop-color='#463c3c' stop-opacity='0.8'/>"
      "<stop offset='100%' stop-color='#363131' stop-opacity='0'/>"
    "</radialGradient>"

    "<!-- Filtry rozmycia dla nadania realistycznej miękkości -->"
    "<filter id='blur_high'><feGaussianBlur stdDeviation='6'/></filter>"
    "<filter id='blur_med'><feGaussianBlur stdDeviation='2.5'/></filter>"
    "<filter id='blur_low'><feGaussianBlur stdDeviation='0.8'/></filter>"
  "</defs>"

  "<!-- Warstwa 1: Rozmyta sadza i ogolne sciemnienie (ambient soot) -->"
  "<circle cx='80' cy='80' r='75' fill='url(#grad_soot)' filter='url(#blur_high)' opacity='0.8'/>"

  "<!-- Warstwa 2: Zewnetrzne slady rozbryzgu (outer scorch spikes) -->"
  "<path d='M135,80 L115,92 L145,115 L108,108 L112,142 L90,118 L80,150 L68,118 L42,142 L55,108 L15,115 L45,92 L10,80 L45,68 L25,40 L55,52 L45,15 L68,45 L80,10 L90,45 L125,25 L105,55 L145,45 L115,68 Z' fill='#110c08' filter='url(#blur_med)' opacity='0.45'/>"

  "<!-- Warstwa 3: Glowny czarny srodek wybuchu (inner blast spikes) -->"
  "<path d='M115,80 L102,88 L120,105 L96,98 L98,118 L85,102 L80,125 L73,102 L58,118 L62,98 L40,105 L55,88 L35,80 L55,72 L45,55 L62,65 L58,40 L73,62 L80,35 L85,62 L108,45 L96,65 L125,55 L102,72 Z' fill='#050505' filter='url(#blur_low)' opacity='0.35'/>"

  "<!-- Warstwa 4: Centralne calkowite wypalenie (core burn) -->"
  "<circle cx='80' cy='80' r='28' fill='url(#grad_core)' opacity='0.45'/>"
"</svg>";

/*
const char* g_decal_svg_scorch =
"<svg width='160' height='160' viewBox='0 0 260 260' xmlns='http://www.w3.org/2000/svg'>"
  "<defs>"
    "<!-- Gradient tla (sadza i pył) -->"
    "<radialGradient id='grad_soot' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#4b4541' stop-opacity='0.9'/>"
      "<stop offset='50%' stop-color='#3f2f24' stop-opacity='0.5'/>"
      "<stop offset='100%' stop-color='#403a3a' stop-opacity='0'/>"
    "</radialGradient>"
    
    "<!-- Gradient epicentrum wybuchu -->"
    "<radialGradient id='grad_core' cx='50%' cy='50%' r='50%'>"
      "<stop offset='0%' stop-color='#322d2d' stop-opacity='1.0'/>"
      "<stop offset='60%' stop-color='#463c3c' stop-opacity='0.8'/>"
      "<stop offset='100%' stop-color='#363131' stop-opacity='0'/>"
    "</radialGradient>"

    "<!-- Filtry rozmycia dla nadania realistycznej miękkości -->"
    "<filter id='blur_high'><feGaussianBlur stdDeviation='6'/></filter>"
    "<filter id='blur_med'><feGaussianBlur stdDeviation='2.5'/></filter>"
    "<filter id='blur_low'><feGaussianBlur stdDeviation='0.8'/></filter>"
  "</defs>"

  "<!-- Warstwa 1: Rozmyta sadza i ogolne sciemnienie (ambient soot) -->"
  "<circle cx='80' cy='80' r='75' fill='url(#grad_soot)' filter='url(#blur_high)' opacity='0.8'/>"

  "<!-- Warstwa 2: Zewnetrzne slady rozbryzgu (outer scorch spikes) -->"
  "<path d='M135,80 L115,92 L145,115 L108,108 L112,142 L90,118 L80,150 L68,118 L42,142 L55,108 L15,115 L45,92 L10,80 L45,68 L25,40 L55,52 L45,15 L68,45 L80,10 L90,45 L125,25 L105,55 L145,45 L115,68 Z' fill='#110c08' filter='url(#blur_med)' opacity='0.85'/>"

  "<!-- Warstwa 3: Glowny czarny srodek wybuchu (inner blast spikes) -->"
  "<path d='M115,80 L102,88 L120,105 L96,98 L98,118 L85,102 L80,125 L73,102 L58,118 L62,98 L40,105 L55,88 L35,80 L55,72 L45,55 L62,65 L58,40 L73,62 L80,35 L85,62 L108,45 L96,65 L125,55 L102,72 Z' fill='#050505' filter='url(#blur_low)' opacity='0.95'/>"

  "<!-- Warstwa 4: Centralne calkowite wypalenie (core burn) -->"
  "<circle cx='80' cy='80' r='35' fill='url(#grad_core)'/>"
"</svg>";
*/

/*
const char* g_decal_svg_scorch =
"<svg width='160' height='160' xmlns='http://www.w3.org/2000/svg'>"
  "<defs><radialGradient id='grad_scorch' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
      "<stop offset='30%' style='stop-color:rgb(30,20,10);stop-opacity:0.6'/>"
      "<stop offset='70%' style='stop-color:rgb(60,40,20);stop-opacity:0.2'/>"
      "<stop offset='100%' style='stop-color:rgb(0,0,0);stop-opacity:0.3'/>"
    "</radialGradient><filter id='soften'><feGaussianBlur stdDeviation='1.4'/></filter></defs>"
"<path d='M64,12 C78,18 88,20 92,34 C110,40 104,54 94,60 C108,74 96,90 80,86 C76,102 60,100 56,88 C40,98 28,86 36,72 C20,66 24,48 40,44 C36,28 48,20 60,24 C62,18 63,15 64,12 Z' fill='url(#grad_scorch)' filter='url(#soften)' opacity='0.44'/>"
"</radialGradient><filter id='soften'><feGaussianBlur stdDeviation='0.8'/></filter></defs>"
"<path d='M64,8 L74,28 L90,22 L86,38 L108,42 L88,56 L98,78 L80,70 L86,96 L66,78 L52,100 L50,76 L28,88 L40,66 L16,52 L42,44 L34,24 L54,34 L64,8 Z' fill='url(#grad_scorch)' filter='url(#soften)' opacity='0.82'/>"
"</svg>";
*/

/*
    "<svg width='128' height='128' xmlns='http://www.w3.org/2000/svg'>"
    "<defs><radialGradient id='grad_scorch' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
    "<stop offset='30%' style='stop-color:rgb(30,20,10);stop-opacity:0.9'/>"
    "<stop offset='70%' style='stop-color:rgb(60,40,20);stop-opacity:0.6'/>"
    "<stop offset='100%' style='stop-color:rgb(0,0,0);stop-opacity:0'/>"
    "</radialGradient></defs>"
    "<circle cx='64' cy='64' r='60' fill='url(#grad_scorch)'/>"
    "</svg>";
*/


const char* g_conchars_svg =
    "<svg width='256' height='256' xmlns='http://www.w3.org/2000/svg'></svg>";
