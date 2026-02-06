#!/bin/bash

# ==============================================================================
#        Skrypt Konfiguracyjny Środowiska Gry Quake 2 Classic
# ==============================================================================
# Wersja: 2.3
# Autor: //Architekt (M-AI-812)
# Opis: Ten skrypt precyzyjnie pobiera i rozpakowuje tylko niezbędne dane
#       gry z oficjalnych instalatorów, a następnie kompiluje i instaluje
#       binaria projektu Q2classic.
# ==============================================================================
set -e # Przerwij działanie skryptu w przypadku pierwszego błędu

# --- Konfiguracja ---
readonly DEMO_URL="http://tastyspleen.net/quake/downloads/q2-314-demo-x86.exe"
readonly FULL_URL="http://tastyspleen.net/quake/downloads/q2-3.20-x86-full-ctf.exe"
readonly INSTALL_DIR="$HOME/.quake2"
# NOWOŚĆ: Katalog do przechowywania pobranych plików
readonly DOWNLOAD_DIR="download_cache"

# Kolory dla czytelności komunikatów
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly RED='\033[0;31m'
readonly NC='\033[0m' # Brak koloru

# --- Funkcje Pomocnicze ---
# (Funkcje check_dependencies i check_urls pozostają bez zmian)
function check_dependencies() {
    echo -e "${YELLOW}Krok 1: Sprawdzanie zależności...${NC}"
    local missing_deps=0
    for cmd in wget unzip make gcc; do
        if ! command -v "$cmd" &> /dev/null; then
            echo -e "${RED}BŁĄD: Wymagany program '$cmd' nie jest zainstalowany.${NC}"; missing_deps=1; fi
    done
    if [[ $missing_deps -eq 1 ]]; then echo -e "${RED}Zainstaluj brakujące pakiety.${NC}"; exit 1; fi
    echo -e "${GREEN}Wszystkie zależności są spełnione.${NC}"
}

function check_urls() {
    echo -e "${YELLOW}Krok 2: Weryfikacja dostępności plików źródłowych...${NC}"
    for url in "$DEMO_URL" "$FULL_URL"; do
        if ! wget --spider -q "$url"; then echo -e "${RED}BŁĄD: Plik $url jest niedostępny.${NC}"; exit 1; fi
    done
    echo -e "${GREEN}Pliki źródłowe są dostępne.${NC}"
}

# --- Główna Logika Skryptu ---

clear
echo "================================================="
echo "      Instalator Danych Gry Quake 2 Classic"
echo "================================================="
echo

check_dependencies; echo
check_urls; echo

read -p "Skrypt przygotuje środowisko gry w '$INSTALL_DIR'. Kontynuować? [T/n] " -n 1 -r; echo
if [[ ! $REPLY =~ ^[Tt]$ ]] && [[ $REPLY != "" ]]; then echo "Przerwano."; exit 0; fi

# Utwórz katalog docelowy i katalog na pobrane pliki
mkdir -p "$INSTALL_DIR"
mkdir -p "$DOWNLOAD_DIR"

# Krok 3: Pobieranie (lub weryfikacja) archiwów
echo -e "${YELLOW}Krok 3: Pobieranie archiwów do katalogu '$DOWNLOAD_DIR'...${NC}"
wget -c --progress=bar --show-progress -P "$DOWNLOAD_DIR" "$DEMO_URL"
wget -c --progress=bar --show-progress -P "$DOWNLOAD_DIR" "$FULL_URL"
echo -e "${GREEN}Archiwa pobrane i gotowe do użycia.${NC}"
echo

# Krok 4: Selektywna ekstrakcja z paczki pełnej
echo -e "${YELLOW}Krok 4: Rozpakowywanie katalogów 'baseq2' i 'ctf' z pełnej paczki...${NC}"
# Używamy cudzysłowów, aby poprawnie przekazać wzorce do unzip
if ! unzip -o "$DOWNLOAD_DIR/q2-3.20-x86-full-ctf.exe" "baseq2/*" "ctf/*" -d "$INSTALL_DIR"; then
    echo -e "${RED}BŁĄD: Rozpakowywanie pełnej paczki nie powiodło się.${NC}"; exit 1;
fi
echo -e "${GREEN}Katalogi 'baseq2' i 'ctf' rozpakowane pomyślnie.${NC}"
echo

# Krok 5: Chirurgiczna ekstrakcja pliku pak0.pak z dema
echo -e "${YELLOW}Krok 5: Wyodrębnianie 'pak0.pak' z paczki demo...${NC}"
# Stwórz bezpieczny katalog tymczasowy do ekstrakcji
TEMP_EXTRACT_DIR=$(mktemp -d)
trap 'rm -rf -- "$TEMP_EXTRACT_DIR"' EXIT # Zaplanuj usunięcie tego katalogu

# Wyodrębnij tylko ten jeden plik do katalogu tymczasowego
unzip -o "$DOWNLOAD_DIR/q2-314-demo-x86.exe" "Install/Data/baseq2/pak0.pak" -d "$TEMP_EXTRACT_DIR"

# Sprawdź, czy plik istnieje, a następnie przenieś go
if [[ -f "$TEMP_EXTRACT_DIR/Install/Data/baseq2/pak0.pak" ]]; then
    mv "$TEMP_EXTRACT_DIR/Install/Data/baseq2/pak0.pak" "$INSTALL_DIR/baseq2/"
    echo -e "${GREEN}'pak0.pak' został pomyślnie przeniesiony.${NC}"
else
    echo -e "${RED}BŁĄD: Nie udało się znaleźć 'pak0.pak' w archiwum demo.${NC}"; exit 1;
fi
echo

# Krok 6: Kompilacja projektu
echo -e "${YELLOW}Krok 6: Kompilowanie projektu Q2classic...${NC}"
make clean
if ! make; then echo -e "${RED}BŁĄD: Kompilacja nie powiodła się.${NC}"; exit 1; fi
echo -e "${GREEN}Projekt skompilowany pomyślnie.${NC}"
echo

# Krok 7: Instalacja skompilowanych plików
echo -e "${YELLOW}Krok 7: Kopiowanie skompilowanych plików do katalogu gry...${NC}"
if [[ ! -f "bin/q2classic" ]] || [[ ! -f "bin/q2game.so" ]]; then
    echo -e "${RED}BŁĄD: Skompilowane pliki nie zostały znalezione.${NC}"; exit 1;
fi
cp bin/q2classic "$INSTALL_DIR/"
cp bin/q2game.so "$INSTALL_DIR/baseq2/"
echo -e "${GREEN}Pliki skopiowane pomyślnie.${NC}"
echo

# --- Zakończenie ---
echo "==================================================================="
echo -e "${GREEN}Instalacja zakończona pomyślnie!${NC}"
echo
echo "Pobrane archiwa zostały zachowane w katalogu: '$DOWNLOAD_DIR'"
echo "Aby uruchomić grę, wykonaj polecenie:"
echo -e "${YELLOW}$INSTALL_DIR/q2classic${NC}"
echo "==================================================================="
