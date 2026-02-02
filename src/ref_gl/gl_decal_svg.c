// Plik: src/ref_gl/gl_decal_svg.c
//
// CEL: Centralne repozytorium dla wszystkich danych SVG.
// Ten plik zawiera FIZYCZNE definicje zmiennych, które są
// DEKLAROWANE jako 'extern' w pliku gl_decal_svg.h.
// To jest standardowa i poprawna praktyka w języku C.

#include "gl_local.h" // Potrzebne dla ew. przyszłych zależności

const char* g_decal_svg_bhole =
    "<svg width='64' height='64' xmlns='http://www.w3.org/2000/svg'>"
    "<defs><radialGradient id='grad' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
    "<stop offset='0%' style='stop-color:rgb(0,0,0);stop-opacity:1' />"
    "<stop offset='60%' style='stop-color:rgb(40,40,40);stop-opacity:1' />"
    "<stop offset='100%' style='stop-color:rgb(120,120,120);stop-opacity:0' />"
    "</radialGradient></defs>"
    "<circle cx='32' cy='32' r='28' fill='url(#grad)' />"
    "</svg>";

const char* g_decal_svg_blaster =
    "<svg width='64' height='64' xmlns='http://www.w3.org/2000/svg'>"
    "<defs><radialGradient id='grad' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
    "<stop offset='0%' style='stop-color:rgb(255,220,180);stop-opacity:1' />"
    "<stop offset='30%' style='stop-color:rgb(255,180,50);stop-opacity:0.9' />"
    "<stop offset='100%' style='stop-color:rgb(200,80,0);stop-opacity:0' />"
    "</radialGradient></defs>"
    "<circle cx='32' cy='32' r='30' fill='url(#grad)' />"
    "</svg>";

const char* g_decal_svg_scorch =
    "<svg width='128' height='128' xmlns='http://www.w3.org/2000/svg'>"
    "<defs><radialGradient id='grad_scorch' cx='50%' cy='50%' r='50%' fx='50%' fy='50%'>"
    "<stop offset='30%' style='stop-color:rgb(30,20,10);stop-opacity:0.9'/>"
    "<stop offset='70%' style='stop-color:rgb(60,40,20);stop-opacity:0.6'/>"
    "<stop offset='100%' style='stop-color:rgb(0,0,0);stop-opacity:0'/>"
    "</radialGradient></defs>"
    "<circle cx='64' cy='64' r='60' fill='url(#grad_scorch)'/>"
    "</svg>";

const char* g_conchars_svg =
    "<svg width='256' height='256' xmlns='http://www.w3.org/2000/svg'></svg>";
