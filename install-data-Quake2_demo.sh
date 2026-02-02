#!/bin/bash

# ==============================================================================
#        Quake 2 Classic Game Environment Setup Script
# ==============================================================================
# Version: 2.3-EN
# Author: //Architekt (M-AI-812)
# Description: This script precisely downloads and extracts the necessary game
#              data from official installers, then compiles and installs
#              the Q2classic project binaries.
# ==============================================================================
set -e # Exit the script on the first error

# --- Configuration ---
readonly DEMO_URL="http://tastyspleen.net/quake/downloads/q2-314-demo-x86.exe"
readonly FULL_URL="http://tastyspleen.net/quake/downloads/q2-3.20-x86-full-ctf.exe"
readonly INSTALL_DIR="$HOME/.q2classic"
# Directory for storing downloaded files
readonly DOWNLOAD_DIR="download_cache"

# Colors for better readability
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly RED='\033[0;31m'
readonly NC='\033[0m' # No Color

# --- Helper Functions ---

function check_dependencies() {
    echo -e "${YELLOW}Step 1: Checking dependencies...${NC}"
    local missing_deps=0
    for cmd in wget unzip make gcc; do
        if ! command -v "$cmd" &> /dev/null; then
            echo -e "${RED}ERROR: Required program '$cmd' is not installed.${NC}"; missing_deps=1; fi
    done
    if [[ $missing_deps -eq 1 ]]; then echo -e "${RED}Please install the missing packages.${NC}"; exit 1; fi
    echo -e "${GREEN}All dependencies are met.${NC}"
}

function check_urls() {
    echo -e "${YELLOW}Step 2: Verifying availability of source files...${NC}"
    for url in "$DEMO_URL" "$FULL_URL"; do
        if ! wget --spider -q "$url"; then echo -e "${RED}ERROR: File $url is not available.${NC}"; exit 1; fi
    done
    echo -e "${GREEN}Source files are available.${NC}"
}

# --- Main Script Logic ---

clear
echo "================================================="
echo "      Quake 2 Classic Game Data Installer"
echo "================================================="
echo

check_dependencies; echo
check_urls; echo

read -p "This script will prepare the game environment in '$INSTALL_DIR'. Continue? [Y/n] " -n 1 -r; echo
if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ $REPLY != "" ]]; then echo "Aborted."; exit 0; fi

# Create target directory and download cache directory
mkdir -p "$INSTALL_DIR"
mkdir -p "$DOWNLOAD_DIR"

# Step 3: Downloading (or verifying) archives
echo -e "${YELLOW}Step 3: Downloading archives to '$DOWNLOAD_DIR' directory...${NC}"
wget -c --progress=bar --show-progress -P "$DOWNLOAD_DIR" "$DEMO_URL"
wget -c --progress=bar --show-progress -P "$DOWNLOAD_DIR" "$FULL_URL"
echo -e "${GREEN}Archives downloaded and ready.${NC}"
echo

# Step 4: Selective extraction from the full package
echo -e "${YELLOW}Step 4: Unpacking 'baseq2' and 'ctf' directories from the full package...${NC}"
if ! unzip -o "$DOWNLOAD_DIR/q2-3.20-x86-full-ctf.exe" "baseq2/*" "ctf/*" -d "$INSTALL_DIR"; then
    echo -e "${RED}ERROR: Unpacking the full package failed.${NC}"; exit 1;
fi
echo -e "${GREEN}'baseq2' and 'ctf' directories unpacked successfully.${NC}"
echo

# Step 5: Surgical extraction of pak0.pak from the demo
echo -e "${YELLOW}Step 5: Extracting 'pak0.pak' from the demo package...${NC}"
TEMP_EXTRACT_DIR=$(mktemp -d)
trap 'echo -e "${YELLOW}Cleaning up temporary files...${NC}"; rm -rf -- "$TEMP_EXTRACT_DIR"' EXIT # Schedule cleanup

# Extract only the required file to the temporary directory
unzip -o "$DOWNLOAD_DIR/q2-314-demo-x86.exe" "Install/Data/baseq2/pak0.pak" -d "$TEMP_EXTRACT_DIR"

# Check if the file exists, then move it
if [[ -f "$TEMP_EXTRACT_DIR/Install/Data/baseq2/pak0.pak" ]]; then
    mv "$TEMP_EXTRACT_DIR/Install/Data/baseq2/pak0.pak" "$INSTALL_DIR/baseq2/"
    echo -e "${GREEN}'pak0.pak' has been moved successfully.${NC}"
else
    echo -e "${RED}ERROR: Could not find 'pak0.pak' in the demo archive.${NC}"; exit 1;
fi
echo

# Step 6: Compiling the project
echo -e "${YELLOW}Step 6: Compiling the Q2classic project...${NC}"
make clean
if ! make; then echo -e "${RED}ERROR: Compilation failed.${NC}"; exit 1; fi
echo -e "${GREEN}Project compiled successfully.${NC}"
echo

# Step 7: Installing the compiled files
echo -e "${YELLOW}Step 7: Copying compiled files to the game directory...${NC}"
if [[ ! -f "bin/q2classic" ]] || [[ ! -f "bin/q2game.so" ]]; then
    echo -e "${RED}ERROR: Compiled files not found.${NC}"; exit 1;
fi
cp bin/q2classic "$INSTALL_DIR/"
cp bin/q2game.so "$INSTALL_DIR/baseq2/"
echo -e "${GREEN}Files copied successfully.${NC}"
echo

# --- Completion ---
echo "==================================================================="
echo -e "${GREEN}Installation completed successfully!${NC}"
echo
echo "Downloaded archives have been saved in the '$DOWNLOAD_DIR' directory."
echo "To run the game, execute the command:"
echo -e "${YELLOW}$INSTALL_DIR/q2classic${NC}"
echo "==================================================================="
