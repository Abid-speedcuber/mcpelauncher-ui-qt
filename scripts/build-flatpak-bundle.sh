#!/bin/sh
set -eu

APP_ID="io.github.Abid_speedcuber.mcpelauncher_minimal"
MANIFEST="packaging/${APP_ID}.yml"
CLIENT32_MANIFEST="packaging/client32/mcpelauncher-client32.yml"
CLIENT32_BUILD_DIR="build-client32"
GENERATED_CLIENT32_DIR="packaging/generated/client32"
BUNDLE="unofficial-bedrock-apk-launcher.flatpak"

flatpak-builder --force-clean "$CLIENT32_BUILD_DIR" "$CLIENT32_MANIFEST"
rm -rf "$GENERATED_CLIENT32_DIR"
mkdir -p "$GENERATED_CLIENT32_DIR"
install -m 0755 "$CLIENT32_BUILD_DIR/files/bin/mcpelauncher-client32" "$GENERATED_CLIENT32_DIR/mcpelauncher-client32"
install -m 0644 "$CLIENT32_BUILD_DIR/files/etc/ld.so.conf" "$GENERATED_CLIENT32_DIR/ld.so.conf"

flatpak-builder --force-clean --repo=repo flatpak-build "$MANIFEST"
flatpak build-update-repo repo
flatpak build-bundle repo "$BUNDLE" "$APP_ID"

echo "Built ${BUNDLE}"
