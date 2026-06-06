# High Distance Loophole

This fork ships a small runtime patch set that convinces Minecraft Bedrock's
Android/x86_64 build to expose higher render and simulation distance limits than
it normally grants to this PC.

## Why It Works

Minecraft does not only read `options.txt` for these sliders. The visible caps
come from native game code that asks the detected device profile how much render
and simulation distance should be allowed.

This fork attacks that in the launcher, before Minecraft finishes starting:

- it spoofs stronger device and renderer properties, so Bedrock's hardware
  classification lands in a higher tier;
- it raises the saved Bedrock options for render distance, particle distance,
  ray tracing/deferred view distances, memory tier, and simulation distance;
- it patches the native simulation-distance provider that maps slider indexes to
  chunk values;
- it also patches the OreUI settings-screen constructor that builds
  `vanilla.simulationDistanceOptions`, which was the separate UI-side 8 chunk
  cap.

That last part is the important loophole. The game had one path where the saved
value could be 22 chunks, but the settings screen still only offered
`[4, 6, 8]`. The patch at `0xa3ecb15` changes the OreUI options-list max so the
settings screen is handed a bigger list, such as:

```text
4, 6, 8, 10, 12, 14, 16, 18, 20, 22
```

So the slider finally agrees with the patched saved value instead of silently
clamping the user interface.

## Who Gets It

Anyone who installs a Flatpak built from this fork after these patches are
included gets the behavior automatically. They do not need to edit
`options.txt`, clear caches, or relaunch after every test.

This does not affect the upstream `io.mrarm.mcpelauncher` Flatpak unless that
Flatpak is rebuilt with the same patch set. It also does not affect an already
published/downloaded bundle unless the bundle was built after these commits.

## How To Reproduce From A Clone

Build and install the Flatpak bundle:

```bash
git clone https://github.com/Abid-speedcuber/mcpelauncher-ui-qt.git
cd mcpelauncher-ui-qt
./scripts/build-flatpak-bundle.sh
flatpak install --user --reinstall -y ./unofficial-bedrock-apk-launcher.flatpak
flatpak run io.github.Abid_speedcuber.mcpelauncher_minimal
```

If a release bundle was built from this patched repo, users can install that
bundle directly:

```bash
flatpak install --user --reinstall -y ./unofficial-bedrock-apk-launcher.flatpak
flatpak run io.github.Abid_speedcuber.mcpelauncher_minimal
```

## Runtime Settings

The default high-distance values are controlled by the launcher-client settings
file inside the Flatpak app data:

```text
~/.var/app/io.github.Abid_speedcuber.mcpelauncher_minimal/data/mcpelauncher/mcpelauncher-client-settings.txt
```

Relevant keys:

```text
enable_high_distance_options=true
high_distance_render_chunks=448
high_distance_simulation_chunks=22
high_distance_memory_tier=4
```

The simulation value is intentionally conservative at 22 chunks. Higher values
can be tried by changing `high_distance_simulation_chunks`, but Minecraft still
has real CPU and world-tick costs once chunks are actually simulated.

## How To Verify

The Qt launcher captures game output in its own log view, so terminal output from
`flatpak run io.github.Abid_speedcuber.mcpelauncher_minimal` may be quiet. A
direct client probe is easier:

```bash
mkdir -p ~/Documents/mcpelauncher-diag
LOG="$HOME/Documents/mcpelauncher-diag/$(date +%Y%m%d-%H%M%S)-direct-client.log"
timeout 25s flatpak run --command=mcpelauncher-client \
  io.github.Abid_speedcuber.mcpelauncher_minimal \
  -dg "$HOME/.var/app/io.github.Abid_speedcuber.mcpelauncher_minimal/data/mcpelauncher/versions/1.26.22.1" \
  2>&1 | tee "$LOG"

rg -n "Applied .*patch|Applied .*hook|OreUI simulation|Simulation distance provider|options gfx|Skipped .*patch|Skipped .*hook" "$LOG"
```

Expected lines include:

```text
Applied simulation distance max patch at 0xb954be7
Applied OreUI simulation distance options max patch at 0xa3ecb15
Simulation distance provider hook exposes 10 options up to 22 chunks
options gfx_viewdistance=448 ... simulationdistance=22
```

## Maintenance Notes

The native offsets are version-specific. They were found for Minecraft Bedrock
`1.26.22.1` x86_64. If Mojang updates `libminecraftpe.so`, the byte patterns or
vtable addresses may move. In that case the launcher should log a skipped patch
or pointer mismatch instead of blindly writing into the wrong place.

The relevant patch files are:

- `packaging/patches/mcpelauncher-client-high-distance-options.patch`
- `packaging/patches/mcpelauncher-client-high-simulation-distance.patch`
- `packaging/patches/mcpelauncher-client-flagship-device-spoof.patch`
- `packaging/patches/mcpelauncher-client-linux-hardware-spoof.patch`
- `packaging/patches/mcpelauncher-client-egl-renderer-spoof.patch`

