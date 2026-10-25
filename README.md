<img src="assets/logo.png" alt="F4RT" width="160">

# F4RT

**F4RT** (PsyFart Designer) is a MIDI instrument for synthetic psytrance farts, squelches, croaks, zaps, and formant hits. Four oscillators, FM, formants, macros, randomizer, factory bank, FX rack.

VST3 and standalone. Audio Unit on macOS. Stupid Systems LLC. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/MANUAL.md](docs/MANUAL.md).

## Release

Windows, Linux, and macOS files come from GitHub Actions (`.github/workflows/dist.yml`). Run it from the Actions tab, or push a numeric tag such as `0.0.2`. The workflow does not publish a release. `<version>` is the `version` in `F4RT.jucer`.

The `dist` artifact holds:

- `F4RT-<version>-win-x64-vst3.zip`
- `F4RT-<version>-win-x64-standalone.zip`
- `F4RT-<version>-win-x64.msi`
- `F4RT-<version>-linux-x64-vst3.zip`
- `F4RT-<version>-linux-x64-standalone.zip`
- `F4RT-<version>-mac-universal-vst3.zip`
- `F4RT-<version>-mac-universal-au.zip`
- `F4RT-<version>-mac-universal-standalone.zip`

Download that artifact into `dist/` and upload the files on a GitHub release:

```
gh run download <run-id> -n dist -D dist
```

The MSI installs the VST3, the standalone app, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\F4RT`; each folder can be changed. The standalone install adds a Start menu shortcut.

The Mac zips are ad-hoc signed, not notarized. Gatekeeper blocks them until you right-click Open, or run `xattr -dr com.apple.quarantine` on the `.vst3`, the `.component`, and the `.app`. The Linux build uses the system ALSA, FreeType, fontconfig, and X11 libraries.

`just build-dist` on Windows still writes the three Windows files only. Building the MSI needs the .NET 8 SDK.
