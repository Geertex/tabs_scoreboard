#!/bin/bash

set -o errexit
cd "$(dirname "$0")"

# --- Configuration ---
OUT_DIR=build
OUT_NAME=$OUT_DIR/tabs_scoreboard.exe
IMGUI_DIR=./imgui
BACKEND_DIR=./imgui/backends

# Create the build directory if it doesn't exist
mkdir -p $OUT_DIR

# --- Sync Assets ---
# This copies everything from an 'assets' folder to your 'build/assets' folder
if test -d "assets"; then
    echo Syncing assets...
    cp -r assets/* $OUT_DIR/assets/
fi

echo Building project...

# --- Compilation ---
clang++ \
    -std=c++17 \
    main.cpp \
    $IMGUI_DIR/*.cpp \
    "$BACKEND_DIR/imgui_impl_glfw.cpp" \
    "$BACKEND_DIR/imgui_impl_opengl3.cpp" \
    "-I$IMGUI_DIR" \
    "-I$BACKEND_DIR" \
    -lglfw -lGL -ldl -lpthread -lX11 \
    -o "$OUT_NAME"
