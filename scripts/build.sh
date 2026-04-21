#!/bin/bash
# scripts/build.sh — AtlasMP Linux build script (server only)
# Usage: ./build.sh [Debug|Release] [clean]

set -e

CONFIG=${1:-Release}
BUILD_DIR="build"

echo ""
echo " ============================================"
echo "  AtlasMP Build Script (Linux/Server)"
echo "  Config: $CONFIG"
echo " ============================================"
echo ""

# Check cmake
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] CMake not found. Install: sudo apt install cmake"
    exit 1
fi

# Check gcc/clang
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "[ERROR] No C++ compiler found."
    echo "        Install: sudo apt install build-essential"
    exit 1
fi

# Clean?
if [ "$2" == "clean" ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Init submodules
echo "[INFO] Initializing submodules..."
git submodule update --init --recursive || echo "[WARNING] Submodule init failed. Continuing..."

# Configure
echo "[INFO] Configuring CMake..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DATLAS_BUILD_CLIENT=OFF \
    -DATLAS_BUILD_SERVER=ON \
    -DATLAS_BUILD_RUNTIME_LUA=ON \
    -DATLAS_BUILD_RUNTIME_JS=ON \
    -DATLAS_BUILD_RPFLIB=ON \
    -DATLAS_BUILD_TESTS=ON \
    -DATLAS_BUILD_LAUNCHER=OFF

# Build
echo "[INFO] Building ($(nproc) cores)..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ""
echo " ============================================"
echo "  Build complete!"
echo "  Output: $BUILD_DIR/bin/"
echo " ============================================"
echo ""
