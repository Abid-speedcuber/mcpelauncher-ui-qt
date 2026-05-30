#!/bin/sh
set -eu

APP_ID="io.github.Abid_speedcuber.mcpelauncher_minimal"
MANIFEST="packaging/${APP_ID}.yml"
BUNDLE="unofficial-bedrock-apk-launcher.flatpak"

flatpak-builder --force-clean --repo=repo flatpak-build "$MANIFEST"
flatpak build-update-repo repo
flatpak build-bundle repo "$BUNDLE" "$APP_ID"

echo "Built ${BUNDLE}"
