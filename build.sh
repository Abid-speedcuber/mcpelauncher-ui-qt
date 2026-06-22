#!/bin/sh
set -eu

APP_ID="io.github.Abid_speedcuber.mcpelauncher"
MANIFEST="packaging/${APP_ID}.yml"
BUILD_DIR="${BUILD_DIR:-build-local}"
BUNDLE="${BUNDLE:-unofficial-bedrock-apk-launcher.flatpak}"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Missing required command: $1" >&2
        exit 1
    fi
}

need cmake
need flatpak
need flatpak-builder

if [ ! -f "$MANIFEST" ]; then
    echo "Missing Flatpak manifest: $MANIFEST" >&2
    exit 1
fi

echo "==> Building native launcher in ${BUILD_DIR}"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 2)"

echo "==> Building Flatpak repo and bundle"
flatpak-builder --force-clean --repo=repo flatpak-build "$MANIFEST"
flatpak build-update-repo repo
flatpak build-bundle repo "$BUNDLE" "$APP_ID"

echo
echo "Build complete:"
echo "  Native binary: ${BUILD_DIR}/mcpelauncher-ui-qt"
echo "  Flatpak repo:  repo/"
echo "  Flatpak file:  ${BUNDLE}"
