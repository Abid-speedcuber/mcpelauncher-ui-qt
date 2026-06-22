#!/bin/sh
set -eu

flatpak install --user --reinstall -y ./unofficial-bedrock-apk-launcher.flatpak
echo "Run with: flatpak run io.github.Abid_speedcuber.mcpelauncher"
