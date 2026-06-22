#!/bin/sh
set -eu

APP_ID="io.github.Abid_speedcuber.mcpelauncher"
REPO_URL="${1:-https://Abid-speedcuber.github.io/mcpelauncher-ui-qt/repo/}"
OUT="${2:-mcpelauncher.flatpakrepo}"

cat > "$OUT" <<EOF
[Flatpak Repo]
Title=Minecraft Pocket Edition Launcher
Name=mcpelauncher
Url=${REPO_URL}
Homepage=https://github.com/Abid-speedcuber/mcpelauncher-ui-qt
Comment=Unofficial local APK import and launch UI for mcpelauncher
DefaultBranch=master
SuggestRemoteName=mcpelauncher
EOF

echo "Wrote ${OUT}"
echo "Install remote: flatpak remote-add --user --if-not-exists mcpelauncher ${OUT}"
echo "Install app:    flatpak install --user mcpelauncher ${APP_ID}"
