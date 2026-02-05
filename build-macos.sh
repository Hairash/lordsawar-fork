#!/bin/bash
# LordsAWar! macOS Build Script
# This script automates building LordsAWar! on macOS with Homebrew

set -e

echo "=== LordsAWar! macOS Build Script ==="
echo ""

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew is required. Install from https://brew.sh"
    exit 1
fi

# Install dependencies
echo "Installing dependencies via Homebrew..."
brew install automake autoconf libtool intltool gettext glib \
    gtkmm3 glibmm libsigc++ libxml++ libxslt libarchive pkg-config gnu-tar 2>/dev/null || true

# Set up environment
export PATH="/opt/homebrew/opt/libtool/libexec/gnubin:/opt/homebrew/opt/gettext/bin:/opt/homebrew/opt/gnu-tar/libexec/gnubin:$PATH"
export ACLOCAL_PATH="/opt/homebrew/share/aclocal:/opt/homebrew/Cellar/glib/$(ls /opt/homebrew/Cellar/glib/ | head -1)/share/aclocal:$ACLOCAL_PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/libxslt/lib/pkgconfig:/opt/homebrew/opt/libarchive/lib/pkgconfig:$PKG_CONFIG_PATH"

# Generate configure if needed
if [ ! -f configure ]; then
    echo "Running autogen.sh..."
    ./autogen.sh
fi

# Configure
echo "Configuring build..."
CPPFLAGS="-I/opt/homebrew/opt/libarchive/include" \
LDFLAGS="-L/opt/homebrew/opt/libarchive/lib" \
./configure --disable-sound "$@"

# Build
echo "Building (this may take a while)..."
make -j$(sysctl -n hw.ncpu)

# Install locally
echo "Installing to local_install directory..."
rm -rf local_install
mkdir -p local_install
make DESTDIR=$(pwd)/local_install install

# Update config file
CONFIG_FILE="$HOME/.config/lordsawar/lordsawarrc"
DATA_PATH="$(pwd)/local_install/usr/local/share/lordsawar"

if [ -f "$CONFIG_FILE" ]; then
    # Update existing config
    sed -i '' "s|<d_datapath>.*</d_datapath>|<d_datapath>$DATA_PATH</d_datapath>|" "$CONFIG_FILE"
    echo "Updated data path in $CONFIG_FILE"
fi

echo ""
echo "=== Build Complete ==="
echo ""
echo "To run the game:"
echo "  ./src/lordsawar"
echo ""
echo "To run the editor:"
echo "  ./src/lordsawar --editor"
echo ""
echo "To run stress test (AI vs AI):"
echo "  ./src/lordsawar --stress-test"
echo ""
