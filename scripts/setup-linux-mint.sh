#!/bin/sh
set -eu

if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    SUDO="sudo"
fi

$SUDO apt-get update
$SUDO apt-get install -y \
    git cmake build-essential pkg-config flatpak \
    qt6-base-dev qt6-declarative-dev qt6-svg-dev \
    qml6-module-qtquick qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts qml6-module-qtquick-window \
    qml6-module-qtquick-dialogs qml6-module-qtquick-templates \
    qml6-module-qt-labs-platform \
    zlib1g-dev libzip-dev

$SUDO flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install -y flathub io.mrarm.mcpelauncher

tmpfile="$(mktemp)"
cat > "$tmpfile" <<'EOF'
#!/bin/sh
exec flatpak run --command=mcpelauncher-client io.mrarm.mcpelauncher "$@"
EOF
$SUDO install -m 0755 "$tmpfile" /usr/local/bin/mcpelauncher-client
rm -f "$tmpfile"

git submodule update --init --recursive

echo "Setup complete."
echo "Build with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j\"\$(nproc)\""
