# Internal tools

Built with the editor when this folder exists (see the `if(EXISTS tools/...)` at the end of CMakeLists.txt). Outputs go to `build/bin/Release`.

## rcftool

```
rcftool list <archive.rcf> [filter]
rcftool extract <archive.rcf> <out folder> [filter]
rcftool pack <folder> <archive.rcf>
rcftool put <archive.rcf> <file> [name in archive]
rcftool remove <archive.rcf> <name in archive>
rcftool verify <archive.rcf | folder of .rcf>
rcftool hash <name>
```

`extract` without a filter writes `cement.manifest` and sets every file's date to the one stored in the archive; `pack` reads both back, so extract + pack of an unedited folder gives the identical archive. Edited or new files get their own write time, new files go after the listed ones. `verify` rebuilds each archive in memory and compares it byte for byte with the original.

## p3dcheck

```
p3dcheck <schema dir> <file or directory> [-v]
```

Parses every `.p3d`, checks the byte-exact round trip, decodes and re-encodes every chunk with its schema and lists the chunk types that don't match or have no schema. Use it after editing a schema.

## fsccheck

```
fsccheck [-v] <file.fsc | folder>
```

Parses every `.fsc`, checks the byte-exact round trip and runs the NIS validation against each scene's streams. Errors only by default; `-v` adds warnings and notes.

## rigcheck

```
rigcheck <model.glb|fbx> <package.p3d> <out.p3d> [--markers] [--height 1.8] [--rotate x y z] [--texture file]
```

Runs the character import without the editor (guessed markers or the model's skeleton) and writes the result to `out.p3d`.

## charpack

```
charpack <character package.p3d> <NewName> [game folder]
```

Copies a character package under a new name (every name inside it is renamed) into `packages/characters/pc/<NewName>.p3d` and adds it to `art/characters/pc/pc_loadgroup.p3d` (backup `.orig`), so the game lists it as one more main-character outfit. The Character Import wizard does the same with *Save as new character*. Names of the game's own outfits are refused.
