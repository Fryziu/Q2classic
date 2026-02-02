#ifndef GL_DECAL_SVG_H
#define GL_DECAL_SVG_H

// Deklaracje "extern" informują kompilator, że te zmienne istnieją,
// ale są zdefiniowane w innym pliku (.c).
// Dzięki temu każdy plik, który dołącza ten nagłówek, wie o ich istnieniu,
// ale nie tworzy ich własnej kopii.

extern const char* g_decal_svg_bhole;
extern const char* g_decal_svg_blaster;
extern const char* g_decal_svg_scorch;
extern const char* g_conchars_svg;

#endif // GL_DECAL_SVG_H


/*
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

// --- Siatka Znaków Konsoli (Font Atlas) ---
// Pełna siatka ma 256 znaków (16x16) w teksturze 256x128.
// Każdy znak ma 16x8 pikseli.
static const char* g_conchars_svg =
    "<svg width=\"128\" height=\"128\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<style> .font { font: 10px monospace; fill: white; } </style>"
    // Linia 0 (znaki specjalne)
    // ...
    // Linia 2 (znaki spacji, !, "...)
    "<text x=\"1\" y=\"23\" class=\"font\"> </text>"
    "<text x=\"9\" y=\"23\" class=\"font\">!</text>"
    "<text x=\"17\" y=\"23\" class=\"font\">\"</text>"
    // ...
    // Linia 3 (znaki @, A, B, C...)
    "<text x=\"1\" y=\"39\" class=\"font\">@</text>"
    "<text x=\"9\" y=\"39\" class=\"font\">A</text>"
    "<text x=\"17\" y=\"39\" class=\"font\">B</text>"
    "<text x=\"25\" y=\"39\" class=\"font\">C</text>"
    // ...i tak dalej dla wszystkich 256 znaków...
    "</svg>";

#endif // GL_DECAL_SVG_H
*/
