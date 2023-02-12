# build-win

This is a custom GitHub action to build an X-Plane plugin on and for Windows based on a prepared CMake setup.

## Inputs

Parameter|Requied|Default|Description
---------|-------|-------|------------
`pluginName`|yes||Plugin's name, used both as top-level folder name and as file name as required by X-Plane
`archFolder`|yes|`win_x64`|Subfolder in which the executable is placed, is based on architecture like 'mac_x64'
`flags`|No||Flags to be passed to `cmake`

## What it does

- Runs a separate command file, `build-win.cmd`, which in turn:
- Creates build folder `build-mac`
- There, runs `CMAKE`, then `NMAKE` to build

## Outputs

Output|Description
------|-----------
`xpl-file-name`|path to the produced xpl file
