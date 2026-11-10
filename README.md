# Src.ToolKit

Modding toolkit for Scarface: The World Is Yours.

## Build

Editor and tools (x64):

```
cmake -S . -B build -A x64
cmake --build build --config Release
```

Game plugin (Win32):

```
cmake -S gamelink -B build-gamelink -A Win32
cmake --build build-gamelink --config Release
```

| Output | From |
|---|---|
| `ToolKit.exe` | `src/` |
| `rcftool`, `p3dcheck`, `fsccheck`, `rigcheck`, `weightcheck`, `charpack` | `tools/` |
| `ToolKitLink.asi` | `gamelink/` |

## Checks

- `p3dcheck <schema dir> <game folder or .p3d>`: P3D round trip.
- `rcftool verify <game folder>`: rebuilds every `.rcf` and compares it.
- `fsccheck <art/nis/fsc folder>`: NIS round trip and validation.
- `rigcheck <model> <package.p3d> <out.p3d>`: character import without the UI.

More in [tools/README.md](tools/README.md).

## Schemas

Chunk layouts live in `schema/*.sc`. `radical.sc` is generated; hand edits go in `scarface.sc`, which overrides it.

## Credits

- Radical Entertainment's Pure3D toollib, p3dexplorer and cement library.
- [aap/scarface-p3d](https://github.com/aap/scarface-p3d)
- Dear ImGui
- [MinHook](https://github.com/TsudaKageyu/minhook)
