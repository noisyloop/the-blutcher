# The Blucher

Noisyloop's eight-slot sample slicing and glitch instrument, built with JUCE 8.0.10. The repository URL retains `the-blutcher`; the instrument is named **The Blucher**.

## Get started in Ableton

Download **The-Blucher-macOS** from a successful [Actions build](../../actions), unzip it, and open the DMG. Quit Live and run `Install-The-Blucher.command`. The installer backs up existing bundles and installs the AU, VST3, and standalone app into your user account. These are ad-hoc signed development builds, not Apple-notarized distribution builds. Only run the installer for builds you trust.

1. Add The Blucher to a MIDI track.
2. Click a sample slot or drop audio onto it. Dropping multiple files fills successive slots. Each slot accepts up to 30 seconds at 8–192 kHz; WAV, AIFF, and FLAC are recommended.
3. Adjust **Slice sensitivity**, then press **Re-slice** to detect new transients. Sustained audio falls back to evenly spaced slices.
4. Press **Generate**, then **Preview**, or start Live's transport. While the host plays, the phrase follows host tempo and song position. Preview uses its own BPM when stopped.
5. Click a step to mute it; Shift-click to lock it. Step labels show source slot:slice, reverse (R), and retrigger count (x).
6. Use **Lock rhythm / sources / pitch / glitches** to constrain Generate and Mutate. **Mutate** changes only some unlocked steps; **Chaos** controls density of transformations. **Undo** restores the last phrase edit (64 edits); **Swap A/B** exchanges two phrase snapshots. Both snapshots use the same sample bank and global controls.
7. Choose straight, triplet, or dotted step lengths. This host-automatable control changes the length of all 16 steps. **Grain mode** repeats short windowed fragments within each slice.
8. **Export WAV** saves a stereo 24-bit / 44.1 kHz phrase. **Drag WAV** renders the current phrase and starts an external file drag to an Ableton audio track. Rendering uses the last known host tempo (preview tempo after a stopped-host processing block).

MIDI notes also open playback while held, with C3/MIDI 60 as the original pitch and other notes transposing the phrase. Playback follows the global phrase clock; note-on does not restart it. During host transport playback the phrase runs continuously regardless of held notes. Stop transport and Preview for note-gated auditioning.

## Sound and state

This is a rhythmic sample collage engine: source selection, transient slices, pitch shifts, reversals, up to eight retriggers per step, gated envelopes, and optional grain playback. It does not yet include a vocoder, independent formant shifting, spectral cross-synthesis, or layered simultaneous sample voices. Pitch shifting changes playback speed. Sample levels are clamped on import, with short playback fades to reduce clicks.

Ableton sessions and host presets embed the sample audio, exact slice boundaries, both patterns, seed, locks, and automatable controls. Original audio files are not needed to reopen a session. Large sample banks increase session size. Undo history and Preview running state are intentionally temporary. Generate is deterministic for a given seed, bank, settings, and starting pattern; normal generation chooses a fresh seed and saves the resulting pattern.

Sample decode, analysis, generation, state serialization, and export run outside the audio callback. The callback uses a non-blocking try-lock and outputs silence if the bank is being replaced or saved; loading/reslicing/saving can therefore briefly interrupt audio. This first release uses a single sample voice at each instant, with no network or AI dependency.

## Build

Requires CMake 3.22+, a C++17 compiler, and network access for JUCE on initial configure.

```sh
cmake -S . -B build -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

The macOS Actions workflow builds universal Intel/Apple Silicon AU, VST3, and standalone bundles, runs engine tests, ad-hoc signs the bundles, and packages a DMG. The workflow runs on PRs, main pushes, and manual dispatch.

Dependency-free engine tests can also be run on Linux:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic -fsanitize=address,undefined -g tests/EngineTests.cpp -ISource -o engine-tests
./engine-tests
```

## Release checks in Live

Before relying on a release, load contrasting samples, generate/lock/mutate, automate the division, test tempo changes and transport jumps, compare WAV export with playback, then save/reopen a Live set with original files moved. Check mono/stereo operation, AU/VST3 scanning, and MIDI note-off behavior. CI covers build and core DSP invariants; it does not run Ableton.
