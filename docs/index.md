# XPPlanes

[![Build all Platforms](https://github.com/TwinFan/XPPlanes/actions/workflows/build.yml/badge.svg)](https://github.com/TwinFan/XPPlanes/actions/workflows/build.yml)

Display additional planes controlled by network messages

## Credits
The project is based on
- [X-Plane SDK](https://developer.x-plane.com/sdk/plugin-sdk-documents/) to integrate with X-Plane
- [GitHub Actions](https://docs.github.com/en/actions)

## Build

### IDE

- MacOS: Just open `XPPlanes.xcodeproj` in Xcode.
- MS Visual Studio: Do "File > Open > Folder..." on the project's main folder.
  VS will initialize based on `CMakeList.txt` and `CMakeSettings.json`.

### Docker Cross Compile

For Cross Compile in a Docker environment for Linux, Mac, and Windows see `docker/README.md`.
Essentially this boils down to installing [Docker Desktop](https://www.docker.com/products/docker-desktop), then:
```
cd docker
make
```

### GitHub Actions

The GitHub workflow `.github/workflows/build.yml` builds the plugin in GitHubs CD/CI environment.
`build.yml` calls a number of custom composite actions available in `.github/actions`,
each coming with its own `README.md`.

The workflow builds Linux, MacOS, and Windows plugin binaries and provides them as artifacts,
so you can download the result from the _Actions_ tab on GitHub.

For **MacOS**, the plugin can be **signed and notarized**, provided that the required secrets are defined in the GitHub repositry:
- `MACOS_CERTIFICATE`: Base64-encoded .p12 certificate as
  [explained here](https://localazy.com/blog/how-to-automatically-sign-macos-apps-using-github-actions#lets-get-started)
- `MACOS_CERT_PWD`: The password for the above .p12 certificate file export
- `NOTARIZATION_USERNAME`: Username for login at notarization service, typically your Apple Developer ID EMail.
- `NOTARIZATION_PASSWORD`: [App-specific password](https://support.apple.com/en-gb/HT204397) for notarization service

### Documentation

[Doxygen-generated code documentation](https://twinfan.github.io/XPPlanes/html/index.html)

## Installation

The plugin itself is to be placed under `<X-Plane>/Resources/plugins/` as usual with the following folder structure:
```
.../XPPlanes/
             mac_x64/XPPlanes.xpl
             lin_x64/XPPlanes.xpl
             win_x64/XPPlanes.xpl
             Resources/...
```
The `Resources` folder needs to hold the CSL model installation, similar to [LiveTraffic](https://twinfan.gitbook.io/livetraffic/setup/installation/step-by-step#csl-model-installation) or [XPMP2 Remote Client](https://twinfan.gitbook.io/livetraffic/setup/installation/xpmp2-remote-client#standard-setup-with-external-visuals).

Alternatively, if you have a CSL installation somewhere, you can create a symbolic link to an existing `Resources` folder. Even [Windows supports symbolic links](https://www.maketecheasier.com/create-symbolic-links-windows10/) for folders by the `mklink /D` command, Linux and Mac users will know the `ln -s` command already (or will find [help on the net](https://osxdaily.com/2015/08/06/make-symbolic-links-command-line-mac-os-x/)).

## Configuration

Currently, there is no user interface available for configuring the plugin. But the plugin writes a configuration file during shutdown, that you can modify while X-Plane/the plugin is not running. Find it in
```
<X-Plane>/Output/preferences/XPPlanes.prf
```
It includes the following config entries:
Item                    | Description
------------------------|-----------------------------------
LogLevel 0              | Logging level: 0 - Debug (most output) ... 4 - Fatal (least output)
LogModelMatch 0         | Log model matching?
ObjReplDataRefs 1       | Replace dataRefs in CSL models? ([more details](https://twinfan.github.io/XPMP2/CopyingObjFiles.html))
ObjReplTextures 1       | Replace textures in CSL models? ([more details](https://twinfan.github.io/XPMP2/CopyingObjFiles.html))
TCAS_Control 1          | Acquire control over TCAS/AI planes upon startup?
PlanesBufferPeriod 5    | Buffering period in seconds
PlanesGracePeriod 30    | Seconds after which a plane without fresh data is removed
PlanesClampAll 0        | Enforce clamping of all planes above ground?
LabelsDraw 1            | Draw plane labels
LabelsMaxDist 5556      | Max distance in meter to draw labels
LabelsCutMaxVisible 1   | Don't draw labels for planes father away than visibility
MapEnable 1             | Support display of planes in X-Plane's map?
MapLabels 1             | Add labels to planes in X-Plane's map?
NetMCGroup 239.255.1.1  | Multicast group the plugin listens to for flight data
NetMCPort 49900         | UDP Multicast port the plugin listens to for flight data, `0` switches off
NetBcstPort 49800       | UDP Broadcast port the plugin listens to for flight data, `0` switches off, e.g. `49005` would listen to RealTraffic's RTTFC data
NetTTL 8                | Time-to-live of network multicast messages
NetBufSize 8192         | (Max) network buffer size in bytes

