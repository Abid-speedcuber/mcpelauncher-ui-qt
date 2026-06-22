# High Distance Forensics

This is the repair log for the render/simulation distance patch work. It is not
a full thought dump; it is the useful path that worked, so a future maintainer
can re-walk it when Minecraft or the launcher moves underneath us.

## Final Known Good State

Known-good target:

```text
Launcher Flatpak: io.github.Abid_speedcuber.mcpelauncher
Minecraft: 1.26.22.1
Minecraft ABI: x86_64
Installed Flatpak commit tested: 70fed6b33601bc6e473081630f43bf012c53be9fcbd57ad38f6bb460d03213d9
Repo commits involved:
  5e0bfca Boost Bedrock render distance device profile
  328804d Force high simulation distance options
  2048b3c Hook Bedrock simulation distance provider
  a2c150d Patch OreUI simulation distance options
  754fa0a Document high distance launcher patch
```

The important patch files are:

```text
packaging/patches/mcpelauncher-client-high-distance-options.patch
packaging/patches/mcpelauncher-client-high-simulation-distance.patch
packaging/patches/mcpelauncher-client-flagship-device-spoof.patch
packaging/patches/mcpelauncher-client-linux-hardware-spoof.patch
packaging/patches/mcpelauncher-client-egl-renderer-spoof.patch
packaging/patches/mcpelauncher-client-distance-diagnostics.patch
```

## Symptom Timeline

Initial symptom:

```text
Render distance stayed capped at 9 chunks.
Simulation distance stayed capped at 8 chunks.
FPS was still high, so this looked like a device-tier cap rather than true load.
```

First useful fix:

```text
Render distance jumped to 22 chunks after the high-distance/device profile work.
```

That proved the launcher was able to influence Bedrock's hardware/profile path.
It also proved the installed Flatpak was actually being replaced and used.

Second symptom:

```text
Simulation distance remained capped at 8 chunks even after options.txt and the
first simulation provider patch reported success.
```

That meant the saved value and one native provider path were no longer enough.
The visible settings screen had another authority.

## Logging Trap

Running this:

```bash
flatpak run io.github.Abid_speedcuber.mcpelauncher
```

starts the Qt UI. The Qt UI captures game output into its own game log, so the
terminal may not show launcher-client hook logs.

Use the direct client when debugging hooks:

```bash
mkdir -p ~/Documents/mcpelauncher-diag
LOG="$HOME/Documents/mcpelauncher-diag/$(date +%Y%m%d-%H%M%S)-direct-client.log"
timeout 25s flatpak run --command=mcpelauncher-client \
  io.github.Abid_speedcuber.mcpelauncher \
  -dg "$HOME/.var/app/io.github.Abid_speedcuber.mcpelauncher/data/mcpelauncher/versions/1.26.22.1" \
  2>&1 | tee "$LOG"

rg -n "Applied .*patch|Applied .*hook|OreUI simulation|Simulation distance provider|options gfx|Skipped .*patch|Skipped .*hook|pointer mismatch|byte pattern mismatch" "$LOG"
```

Known-good runtime lines:

```text
Applied high distance options: render=448, sim=22, particles=256, memory tier=4
Applied simulation distance max patch at 0xb954be7
Applied OreUI simulation distance options max patch at 0xa3ecb15
Applied simulation distance option count hook at 0x14a215a8
Applied simulation distance current index hook at 0x14a215b0
Applied simulation distance setter hook at 0x14a215b8
Simulation distance provider hook exposes 10 options up to 22 chunks
DistanceDiag options gfx_viewdistance=448 gfx_renderdistance=-1 simulationdistance=22 server_viewdistance=-1 memory_tier_override=4
```

If those lines do not appear in the Qt UI terminal, that alone is not proof of
failure. Use the direct client command above.

## Render Distance Path That Worked

Render distance improved after three classes of changes:

1. Spoof stronger Android/device properties.
2. Spoof stronger Linux CPU/sysconf/scheduler properties.
3. Raise Bedrock render-distance related options and memory tier in
   `options.txt` through launcher code at startup.

Useful settings keys:

```text
enable_high_distance_options=true
high_distance_render_chunks=448
high_distance_particle_chunks=256
high_distance_raytracing_chunks=64
high_distance_deferred_chunks=64
high_distance_memory_tier=4
```

Runtime options touched:

```text
gfx_viewdistance
gfx_particleviewdistance
raytracing_viewdistance
deferred_viewdistance
device_info_memory_tier_override
device_info_use_memory_tier_override
show_render_distance_warning_modal
```

Render distance going from 9 to 22 was the first proof that the device/profile
spoofing plus options injection were on target.

## Simulation Distance Path That Worked

The simulation-distance fix needed two separate native paths.

### Provider Path

The first target was `Settings::SimulationDistanceDataProvider`.

Found strings and type names in `libminecraftpe.so`:

```text
simulationDistance
simulationDistanceOptions
vanilla.simulationDistanceOptions
N8Settings30SimulationDistanceDataProviderE
```

Useful provider offsets for Minecraft `1.26.22.1` x86_64:

```text
0xb954be7   code call patched to return high_distance_simulation_chunks
0x14a215a8  provider vtable slot: option count
0x14a215b0  provider vtable slot: current index
0x14a215b8  provider vtable slot: setter
```

The patch replaces this byte pattern:

```text
ff 90 70 07 00 00
```

with:

```text
b8 <chunks> 00 00 00 90
```

For the default 22 chunks, `<chunks>` is `0x16`.

This made the saved value and provider say 22, but the UI still showed only 8.
That was the clue that the settings screen was reading another list.

### OreUI Path

The real UI cap came from `OreUI::SimulationDistanceFacet`.

Useful type/string landmarks:

```text
N5OreUI23SimulationDistanceFacetE
vanilla.simulationDistanceOptions
simulationDistanceOptions
```

The relevant native object exposes `simulationDistanceOptions` as a vector-like
member at object offset `0x18`.

The decisive disassembly region was around:

```text
0xa4045a0
0xa404700
0xa404930
0xa4049e0
```

Why those mattered:

```text
0xa4045a0 and 0xa404700 register the read-only property named
"simulationDistanceOptions".

They pass member offset 0x18 while registering that property, proving the
settings screen reads the options list from the facet object.

0xa404930 and 0xa4049e0 destroy/free the member at +0x18, confirming it is real
owned state and should not be naively replaced with a static array.
```

The constructor/fill path was found by searching for references to the vtable
region. A useful target was:

```text
0x149531b0
```

One useful scan was a RIP-relative `lea` scanner over `.text`:

```bash
LIB="$HOME/.var/app/io.github.Abid_speedcuber.mcpelauncher/data/mcpelauncher/versions/1.26.22.1/lib/x86_64/libminecraftpe.so"
python3 - <<'PY' "$LIB"
import sys, struct
path=sys.argv[1]
text_addr=0x6070a00
text_off=0x6070a00
text_size=0xe5ddacd
targets={0x149531a8,0x149531b0,0x149531f8,0x149531c0,0x14953790,0x14953208}
hits={t:[] for t in targets}
with open(path,'rb') as f:
    f.seek(text_off)
    data=f.read(text_size)
i=0
pat=b'\x48\x8d\x05'
while True:
    i=data.find(pat, i)
    if i < 0:
        break
    disp=struct.unpack_from('<i', data, i+3)[0]
    target=text_addr+i+7+disp
    if target in hits:
        hits[target].append(text_addr+i)
    i += 1
for t,hs in hits.items():
    print(hex(t), len(hs), ' '.join(hex(h) for h in hs[:80]))
PY
```

Known-good useful output:

```text
0x149531b0 1 0xa3eca24
```

Disassemble around that hit:

```bash
objdump -d -Mintel --start-address=0xa3ec980 --stop-address=0xa3ecec0 "$LIB"
```

Important landmarks in that constructor:

```text
0xa3eca24  loads/writes SimulationDistanceFacet vtable
0xa3ecb15  calls vtable function at +0x770 to ask for max simulation distance
0xa3ecbf0  writes option values starting at 4
0xa3ecbfc  increments option value by 2
```

The patch at `0xa3ecb15` changes the OreUI constructor's max-distance call to
return the configured high simulation chunk value. That makes the UI build a
larger list:

```text
4, 6, 8, 10, 12, 14, 16, 18, 20, 22
```

This is what finally raised the visible simulation slider beyond 8.

## Why Not Replace The Vector Directly

The `OreUI::SimulationDistanceFacet + 0x18` member looks like the data source,
but the destructor path frees owned state. Replacing its pointer with a static
array or memory from the wrong allocator could boot once and crash later when
the screen closes.

The safer fix was to patch the constructor's max-distance call before the game
builds the vector itself. That lets Minecraft allocate, populate, and destroy
its own container normally.

## How To Re-find Offsets After A Minecraft Update

1. Build/install the current launcher and run the direct client probe.
2. Check which patches say `Applied` and which say `Skipped`.
3. If `0xb954be7` fails, find the new
   `Settings::SimulationDistanceDataProvider` constructor/fill path.
4. If provider hooks apply but the UI still caps at 8, focus on
   `OreUI::SimulationDistanceFacet`.
5. Search strings:

```bash
strings -tx "$LIB" | rg "simulationDistanceOptions|vanilla\\.simulationDistanceOptions|SimulationDistanceFacet|SimulationDistanceDataProvider"
```

6. Disassemble around functions referencing the type/vtable/property string.
7. Find the path that writes values starting at 4 and increments by 2.
8. Patch the call that obtains max simulation distance, not the vector storage.
9. Rebuild, reinstall, and verify with the direct client probe.

## Build And Install Loop

Use this after patch edits:

```bash
./scripts/build-flatpak-bundle.sh
flatpak install --user --reinstall -y ./unofficial-bedrock-apk-launcher.flatpak
```

Then verify:

```bash
flatpak info --user io.github.Abid_speedcuber.mcpelauncher | rg "Commit|Installed|Subject|Date"
grep -aF "OreUI simulation distance options max" -n \
  ~/.local/share/flatpak/app/io.github.Abid_speedcuber.mcpelauncher/current/active/files/bin/mcpelauncher-client
```

Run the direct client probe from the logging section.

## Common Failure Meanings

```text
No hook logs in terminal:
  Probably launched the Qt UI, which captures client logs. Use direct client.

Render distance high, sim still capped at 8:
  Device/profile spoofing works, but OreUI simulationDistanceOptions path is not
  patched or moved.

simulationdistance=22 in DistanceDiag, UI still 8:
  Saved option is high, but UI options may be using the HBUI JavaScript fallback.
  In 1.26.31 the fallback is `[4,6,8]` whenever the native facet has at most
  one entry. `highdistanceassets.h` expands it through 22 during import and
  when existing versions are loaded.

Skipped simulation distance max patch: signature matched 0 locations:
  Minecraft changed the provider call sequence. Re-find and extend the full
  signature, then verify it is unique in every available test binary.

Skipped OreUI simulation distance options max patch: signature matched 0 locations:
  Minecraft changed the OreUI call sequence. Re-find the facet path.

signature matched more than 1 location:
  Do not patch any candidate. Strengthen the surrounding signature until it is
  unique; patching every virtual call at vtable offset 0x770 is unsafe.

Skipped simulation distance option count/current/setter hook: pointer mismatch:
  Provider vtable moved or changed. Re-find provider vtable slots.

Build patch says malformed patch:
  The patch file hunk line counts are stale. Update the @@ counts because these
  are git patch files, not raw source edits.
```

## Important Caveat

The original offsets were not Minecraft-version neutral. The current x86_64
implementation instead scans for two complete, unique instruction signatures.
It was verified against `1.26.22.1` and `1.26.31.1`: the old offsets
`0xb954be7`/`0xa3ecb15` moved to `0xbe685a7`/`0xa8ffe85`, while the signatures
remained stable. Future compiler changes can still require rediscovery.

Launcher updates are a different risk: the patch files may fail to apply if
upstream source changes nearby. If they still apply and compile, Minecraft code
generation around the two scanned call sites remains the main fragile part.
