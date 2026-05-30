# Unofficial Bedrock APK Launcher

This is a minimal, unofficial fork of
[`minecraft-linux/mcpelauncher-ui-qt`](https://github.com/minecraft-linux/mcpelauncher-ui-qt).

It removes the Google Play login, Google Play API, in-app APK downloading,
profile management, update checker, mod manager, troubleshooter, webview, and
archival version list. The remaining app does two things:

- import a local Android APK supplied by the user
- launch the imported game with `mcpelauncher-client`

This project is not affiliated with, endorsed by, or approved by Mojang,
Microsoft, Google, or the upstream mcpelauncher maintainers.

## No Game Files Included

This repository and its builds do not include Minecraft, Minecraft APKs,
extracted Minecraft libraries, assets, or any other Mojang/Microsoft game
files. Users must provide their own legally obtained APK.

The goal is to make the local-APK flow less painful for people who already own
the game and do not want to depend on Google login/download behavior inside the
launcher UI.

## Relationship To Upstream

This fork keeps `upstream` pointing at the original UI repository and keeps the
small build-time support libraries as Git submodules under `third_party/`.

For maintenance:

```sh
git fetch upstream
git submodule update --remote --recursive
```

Backend launching is handled by `mcpelauncher-client`. The Flatpak package in
this repo builds that backend into the same sandbox, so end users do not need a
host `mcpelauncher-client` install.

## Easiest Install

One-time Flatpak setup on Linux Mint/Ubuntu:

```sh
sudo apt install flatpak
sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

Then install a release bundle:

```sh
flatpak install --user ./unofficial-bedrock-apk-launcher.flatpak
flatpak run io.github.Abid_speedcuber.mcpelauncher_minimal
```

If the bundle is attached to a GitHub release, the friend-facing flow is:

```sh
wget https://github.com/Abid-speedcuber/mcpelauncher-ui-qt/releases/latest/download/unofficial-bedrock-apk-launcher.flatpak
flatpak install --user ./unofficial-bedrock-apk-launcher.flatpak
flatpak run io.github.Abid_speedcuber.mcpelauncher_minimal
```

Flatpak will fetch the KDE runtime from Flathub if it is not already installed.

In the app:

1. Click **Import APK**.
2. Select your legally obtained local APK.
3. Select the imported version.
4. Click **Play**.

This bundle does not include Minecraft, APK files, or Mojang/Microsoft assets.

## Building The Flatpak Bundle

Install Flatpak Builder and the build runtimes:

```sh
sudo apt install flatpak flatpak-builder
sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install -y flathub org.kde.Sdk//6.10 org.freedesktop.Sdk.Extension.llvm21//25.08
```

Clone and build:

```sh
git clone --recursive https://github.com/Abid-speedcuber/mcpelauncher-ui-qt.git
cd mcpelauncher-ui-qt
./scripts/build-flatpak-bundle.sh
./scripts/install-flatpak-bundle.sh
```

The bundle is written to `unofficial-bedrock-apk-launcher.flatpak`.

## Updating Installed Releases

A standalone `.flatpak` bundle from GitHub Releases can be installed again over
an existing install as long as the app ID stays the same:

```sh
flatpak install --user ./unofficial-bedrock-apk-launcher.flatpak
```

That replaces the installed app, but it is still a manual update flow. For a
proper update channel, publish a Flatpak repository instead of only release
assets: build each release, run `flatpak build-update-repo repo`, host the
`repo/` directory over HTTPS, and give users a `.flatpakrepo` file. After that,
users install once from your remote and receive future builds with:

```sh
flatpak update io.github.Abid_speedcuber.mcpelauncher_minimal
```

Keep the app ID stable between releases; changing it makes Flatpak treat the
next build as a different application.

This repo includes a GitHub Pages helper for that flow:

```sh
./scripts/build-flatpak-bundle.sh
./scripts/publish-flatpak-repo-gh-pages.sh
```

That publishes `repo/` and `mcpelauncher-minimal.flatpakrepo` to the `gh-pages`
branch. Enable GitHub Pages for that branch in the repository settings, then
users can add the remote with:

```sh
flatpak remote-add --user --if-not-exists mcpelauncher-minimal https://Abid-speedcuber.github.io/mcpelauncher-ui-qt/mcpelauncher-minimal.flatpakrepo
flatpak install --user mcpelauncher-minimal io.github.Abid_speedcuber.mcpelauncher_minimal
```

For a tagged public release with both update repo publishing and GitHub Release
assets, commit your changes and run:

```sh
./scripts/release.sh v1.2.3
```

That script builds `unofficial-bedrock-apk-launcher.flatpak`, publishes the
Flatpak update repo to `gh-pages`, tags the source, and uploads the bundle plus
`.flatpakrepo` file to GitHub Releases.

## Linux Mint / Ubuntu Host Build

This is useful for development, but the Flatpak bundle above is the recommended
redistribution format.

Clone with submodules:

```sh
git clone --recursive https://github.com/Abid-speedcuber/mcpelauncher-ui-qt.git
cd mcpelauncher-ui-qt
```

Install dependencies:

```sh
./scripts/setup-linux-mint.sh
```

Build and run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/mcpelauncher-ui-qt
```

## Host Build Backend

The non-Flatpak host build expects a command named `mcpelauncher-client` on
`PATH`.

The setup script creates `/usr/local/bin/mcpelauncher-client` as a wrapper around
the Flathub package:

```sh
flatpak run --command=mcpelauncher-client io.mrarm.mcpelauncher
```

This host-build backend can be updated through Flatpak:

```sh
flatpak update io.mrarm.mcpelauncher
```

## Building Without The Script

Install the usual Qt6/CMake/libzip dependencies, install the Flatpak backend,
then build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

If you keep dependency sources somewhere else, pass:

```sh
cmake -S . -B build -DMCPELAUNCHER_ROOT=/path/to/dependencies
```

The expected dependency layout is:

```text
third_party/
  axml-parser/
  mcpelauncher-apkinfo/
  mcpelauncher-common/
  mcpelauncher-extract/
  file-util/
```

## License

This fork remains licensed under GNU GPLv3. See [`LICENSE`](LICENSE).

Because this is a modified GPLv3 fork, redistributors must provide the
corresponding source code for the exact binary they distribute, preserve license
and copyright notices, and make it clear that this is an unofficial modified
version.

See [`NOTICE.md`](NOTICE.md) for attribution and redistribution notes.
