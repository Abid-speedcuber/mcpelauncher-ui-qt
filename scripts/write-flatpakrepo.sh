#!/bin/sh
set -eu

APP_ID="io.github.Abid_speedcuber.mcpelauncher_minimal"
REPO_URL="${1:-https://Abid-speedcuber.github.io/mcpelauncher-ui-qt/repo/}"
OUT="${2:-mcpelauncher-minimal.flatpakrepo}"

cat > "$OUT" <<EOF
[Flatpak Repo]
Title=Unofficial Bedrock APK Launcher
Name=mcpelauncher-minimal
Url=${REPO_URL}
Homepage=https://github.com/Abid-speedcuber/mcpelauncher-ui-qt
Comment=Unofficial local APK import and launch UI for mcpelauncher
DefaultBranch=master
SuggestRemoteName=mcpelauncher-minimal
EOF

echo "Wrote ${OUT}"
echo "Install remote: flatpak remote-add --user --if-not-exists mcpelauncher-minimal ${OUT}"
echo "Install app:    flatpak install --user mcpelauncher-minimal ${APP_ID}"
