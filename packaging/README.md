# Packaging Notes

This fork is safe to package only as an APK-import launcher. Do not package
Minecraft APKs, extracted Minecraft files, or proprietary Mojang/Microsoft
assets.

## Recommended Package

Use the Flatpak manifest in this directory. It builds:

- the stripped-down Qt launcher UI
- `mcpelauncher-client`
- the required open-source launcher support libraries
- desktop, icon, and AppStream metadata

It does not include Minecraft, APKs, extracted game files, or proprietary
Mojang/Microsoft assets.

Build a local bundle:

```sh
./scripts/build-flatpak-bundle.sh
```

Install the bundle:

```sh
flatpak install --user ./unofficial-bedrock-apk-launcher.flatpak
```

## Redistributing

The simplest release is a GitHub Release asset containing
`unofficial-bedrock-apk-launcher.flatpak`. Users can install it with:

```sh
flatpak install --user ./unofficial-bedrock-apk-launcher.flatpak
```

The stronger update-friendly route is a hosted Flatpak repository:

```sh
flatpak build-update-repo repo
```

Host the generated `repo/` directory over HTTPS and provide a `.flatpakrepo`
file. Users can then add the remote once, install the app, and receive future
updates with `flatpak update`.

Flathub submission is possible later, but it needs stricter review of app ID,
metadata, screenshots, permissions, licensing, and trademark wording.

## Other Packages

Native `.deb` packages are possible for development users, but they either need
to include `mcpelauncher-client` themselves or depend on a host command named
`mcpelauncher-client`. For non-power-users, the Flatpak bundle is the cleaner
path.

## App Identity

Suggested public names:

- Unofficial Bedrock APK Launcher
- Local Bedrock APK Launcher
- mcpelauncher-minimal

Avoid implying official Mojang/Microsoft approval.
