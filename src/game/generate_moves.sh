#!/bin/bash

# Plik wyjściowy dla wygenerowanego kodu C
OUTPUT_FILE="g_core_moves.c"
# Katalog do przeszukania
SOURCE_DIR="/home/user/git_project/Q2classic_2025_10/src/game/"

# Wyrażenie regularne w standardzie Perl do znalezienia linii definiujących mmove_t
# Szuka linii, które zaczynają się od "mmove_t", a następnie chwyta nazwę zmiennej.
REGEX='^mmove_t\s+([a-zA-Z0-9_]+)\s*='

echo "// Ten plik został wygenerowany automatycznie przez skrypt generate_moves.sh" > $OUTPUT_FILE
echo "// Zawiera funkcję rejestrującą wszystkie animacje z podstawowej gry." >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE
echo "#include \"g_local.h\"" >> $OUTPUT_FILE
echo "#include \"g_moves.h\"" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE
echo "void RegisterCoreMoves(void)" >> $OUTPUT_FILE
echo "{" >> $OUTPUT_FILE

# Znajdź wszystkie pliki, znajdź pasujące linie, wyodrębnij nazwy i stwórz kod C
grep -rPh -o $REGEX $SOURCE_DIR/*.c | sed -E "s/$REGEX/\1/" | while read -r move_name; do
  # Dodaj deklarację extern do pliku nagłówkowego
  echo "extern mmove_t $move_name;"
  # Dodaj linię rejestracji do pliku .c
  echo "    RegisterMMove(\"$move_name\", &$move_name);" >> $OUTPUT_FILE
done > g_moves.h # Przekieruj deklaracje extern do g_moves.h

echo "}" >> $OUTPUT_FILE

echo ""
echo "Generowanie zakończone."
echo "Plik nagłówkowy 'g_moves.h' z deklaracjami extern został utworzony."
echo "Plik źródłowy '$OUTPUT_FILE' z funkcją RegisterCoreMoves() został utworzony."
