#!/bin/bash

# ==============================================================================
# Skrypt do automatycznego patchowania plików potworów (m_*.c)
#
# Cel: Zautomatyzowanie dodawania linii `currentmove_name` po każdym
#      przypisaniu wskaźnika `currentmove`.
#
# OSTRZEŻENIE: Ten skrypt modyfikuje pliki źródłowe. Uruchom go tylko na
#              czystej kopii repozytorium gita lub zrób kopię zapasową.
#              Dla każdego zmodyfikowanego pliku tworzona jest kopia
#              zapasowa z rozszerzeniem .bak.
# ==============================================================================

# Katalog zawierający pliki potworów
SOURCE_DIR="/home/user/git_project/Q2classic_2025_10/src/game/"

# Wyrażenie regularne SED:
# 1. (\s*): Zapamiętaj (jako \1) wszystkie białe znaki na początku linii (wcięcie).
# 2. (self->monsterinfo\.currentmove\s*=\s*&): Dopasuj dosłowny tekst przypisania.
# 3. ([a-zA-Z0-9_]+);: Zapamiętaj (jako \2) nazwę animacji (litery, cyfry, podkreślnik).
#
# Ciąg zastępujący:
# 1. &: Wstaw całą oryginalnie dopasowaną linię.
# 2. \n: Wstaw znak nowej linii.
# 3. \1: Wstaw zapamiętane wcięcie z oryginalnej linii.
# 4. Pozostały tekst: Wstaw nową linię kodu, używając zapamiętanej nazwy animacji (\2).
SED_EXPRESSION='s/(\s*)(self->monsterinfo\.currentmove\s*=\s*&([a-zA-Z0-9_]+);)/&\n\1    self->monsterinfo.currentmove_name = "\3";/g'

# Sprawdź, czy katalog źródłowy istnieje
if [ ! -d "$SOURCE_DIR" ]; then
  echo "Błąd: Katalog '$SOURCE_DIR' nie został znaleziony."
  exit 1
fi

echo "Rozpoczynam patchowanie plików potworów w '$SOURCE_DIR'..."

# Znajdź wszystkie pliki m_*.c i wykonaj na nich operację `sed`
find "$SOURCE_DIR" -type f -name "m_*.c" | while read -r filepath; do
  echo "Przetwarzam: $filepath"
  # Użyj sed do modyfikacji pliku w miejscu (-i) i utwórz kopię zapasową (.bak)
  # -E pozwala na użycie rozszerzonych wyrażeń regularnych
  sed -i.bak -E "$SED_EXPRESSION" "$filepath"
done

echo ""
echo "Patchowanie zakończone."
echo "Utworzono pliki zapasowe z rozszerzeniem .bak."
echo "Przejrzyj zmiany (np. za pomocą 'git diff') i ręcznie popraw ewentualne błędy."
echo ""
echo "Gdy będziesz pewien, że wszystko jest w porządku, możesz usunąć pliki zapasowe komendą:"
echo "find $SOURCE_DIR -name '*.bak' -delete"

exit 0
