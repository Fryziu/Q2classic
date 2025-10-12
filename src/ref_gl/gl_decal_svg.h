#ifndef GL_DECAL_SVG_H
#define GL_DECAL_SVG_H

// =======================================================================
//          Repozytorium Wbudowanych Zasobów SVG dla Decali
// =======================================================================
// M-AI-812: Przechowujemy tutaj kody SVG jako stałe, aby uniezależnić
// się od plików zewnętrznych podczas developmentu.
// =======================================================================

// --- Ślad po kuli (Bullethole) - Ciemnoszary, nieregularny ---
static const char* g_decal_svg_bhole =
    "<svg width=\"64\" height=\"64\" viewBox=\"0 0 64 64\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<defs><radialGradient id=\"g\" cx=\"50%\" cy=\"50%\" r=\"50%\"><stop stop-color=\"#1a1a1a\" offset=\"0%\"/><stop stop-color=\"#333\" offset=\"60%\"/><stop stop-color=\"#fff\" stop-opacity=\"0\" offset=\"100%\"/></radialGradient></defs>"
    "<path d=\"M32,16 C18,20 22,30 18,34 C14,38 18,48 32,50 C44,46 44,38 48,34 C52,30 48,20 32,16 Z\" fill=\"url(#g)\"/>"
    "</svg>";

// --- Ślad po Blasterze (Energetyczny) - Żółty, świecący ---
static const char* g_decal_svg_blaster =
    "<svg width=\"64\" height=\"64\" viewBox=\"0 0 64 64\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<defs><radialGradient id=\"g\" cx=\"50%\" cy=\"50%\" r=\"50%\"><stop stop-color=\"#FFFF00\" offset=\"0%\"/><stop stop-color=\"#FFAA00\" offset=\"70%\"/><stop stop-color=\"#FF0000\" stop-opacity=\"0\" offset=\"100%\"/></radialGradient></defs>"
    "<circle cx=\"32\" cy=\"32\" r=\"28\" fill=\"url(#g)\"/>"
    "</svg>";

// --- Ślad po Wybuchu (Scorch) - Duży, pomarańczowy, opalony ---
static const char* g_decal_svg_scorch =
    "<svg width=\"128\" height=\"128\" viewBox=\"0 0 128 128\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<defs><radialGradient id=\"g\" cx=\"50%\" cy=\"50%\" r=\"50%\"><stop stop-color=\"#332211\" offset=\"0%\"/><stop stop-color=\"#FF8800\" offset=\"60%\"/><stop stop-color=\"#FFFFFF\" stop-opacity=\"0\" offset=\"100%\"/></radialGradient></defs>"
    "<circle cx=\"64\" cy=\"64\" r=\"60\" fill=\"url(#g)\"/>"
    "</svg>";

// --- Placeholder dla krwi ---
// (możemy dodać w przyszłości)

#endif // GL_DECAL_SVG_H
