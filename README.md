# XPPlanes

[![Build all Platforms](https://github.com/TwinFan/XPPlanes/actions/workflows/build.yml/badge.svg)](https://github.com/TwinFan/XPPlanes/actions/workflows/build.yml)

Display additional planes controlled by network messages.
These network message are to be generated by a 3rd party tool like a traffic generator
or a script feeding pre-recorded data. Examples are provided in the `scripts` folder.

The supported network message formats are explained below in the [Network Message Formats](#network-message-formats) sections.

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

## Network Message Formats

XPPlanes processes traffic data that is received from UDP network datagrams.

### General Principles Applicable to All Formats

XPPlanes can listen to one UDP multicast port, one UDP broadcast
port, or both at the same time.

XPPlanes processes traffic data from incoming network messages on either port.
The format is determined from the message _content,_ ie. is _not_ derived from
aspects like the port number. (So you could even mix formats in different messages...)

Each traffic data record adds position information for one plane.
There is no expectation as to how often updates are received.
There is no need to update all planes in one go at the same time;
depending on your feeding application it may be more reasonable to send updates based on individual assessment
(like more frequently for turning or fast moving planes, less often for stationary planes).

Each traffic data record _must_ include a numeric identifier for the plane it represents,
and position information. Everything else is optional.

Not provided data will not change. E.g., if you want to extend the gear you can send
`gear = 1.0` at the time of gear extension, then you could leave out this attribute until you want
to retract the gear, at which point your send `gear = 0.0`.

Mass, wing span and area, as well as lift are used for [wake turbulence configuration](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/).
If not provided, then the [XPMP2 library](https://github.com/TwinFan/XPMP2) provides defaults,
which base on the wake turbulence category,
which in turn is derived from the ICAO aircraft type designator:
- L: C172
- L/M: B350
- M: A320
- H: B744
- J: A388

#### Timestamp

It is recommended to include _timestamp_ information with the tracking data,
but this is not mandatory. Without timestamp information the data is assumed to be valid _now_ (identical to a relative timestamp of `0.0`).
Timestamp information can be relative (recommended) or absolute:
- A _relative_ timestamp is a (comparibly small) float saying when the data was/is valid
  compared to _now_ in seconds. E.g., `-1.2` defines the data was valid 1.2 seconds ago,
  `0.7` says the data becomes valid in 0.7 seconds time.
- An _absolute_ timestamps directly defines the absolute time the data is valid.
  This timestamp is compared to the time of the computer XPPlanes runs on.
  You need to ensure proper computer clock synchronization in case you feed data from
  a different computer or take it from a different source.

In all cases will the `PlanesBufferPeriod` (see [Configuration](#configuration)) be added
to the received timestamp information and hence will delay display of the plane at the
given position. There is no restriction on the value of `PlanesBufferPeriod`, it can even be `0`.

Typically, `PlanesBufferPeriod` is positive,
which is useful if your traffic data is always a few seconds old,
like if you would relay real-world traffic data: If you compensate the age of the data
with the buffering period, then XPPlanes can still properly interpolate between two given
positions and planes will fly nicely. Would you run with `PlanesBufferPeriod = 0` then
XPPlanes would always only see outdated data and would need to extrapolate positions
beyond the last received position, which tends to be inaccurate.

`PlanesBufferPeriod = 0` is only recommended if you can feed high-speed data
(like updates every one or two seconds) with current positions.

Planes, for which the youngest timestamp is older than `PlanesGracePeriod` seconds,
will be removed.

### XPPTraffic

`XPPTraffic` is a custom purpose-built JSON format that supports all
features of XPPlanes. One traffic data records looks like this
(see file `docs/XPPTraffic.json`):

```
{
  "id" : 4711,
  "ident" : {
    "airline" : "DLH",
    "reg" : "D-EVEL",
    "call" : "DLH1234",
    "label" : "Test Flight"
  },
  "type" : {
    "icao" : "C172",
    "wingSpan" : 11.1,
    "wingArea" : 16.2
  },
  "position" : {
    "lat" : 51.406292,
    "lon" : 6.939847,
    "alt_geo" : 407,
    "gnd" : true,
    "timestamp" : -0.7
  },
  "attitude" : {
    "roll" : -0.2,
    "heading" : 42,
    "pitch" : 0.1
  },
  "config" : {
    "mass" : 1037.6,
    "lift" : 10178.86,
    "gear" : 1,
    "noseWheel" : -2.5,
    "flaps" : 0.5,
    "spoiler" : 0
  },
  "light" : {
    "taxi" : true,
    "landing" : false,
    "beacon" : true,
    "strobe" : false,
    "nav" : true
  }
}
```

Field           | Description
----------------| ------------------------------
id              | **Mandatory** numeric identification of the plane. Can be a numeric integer value like `4711` or a string value. A string value is interpreted as a hex number, like `00c01abc`.
` `             | ` `
**ident/**      | Optional object with plane identifiers, recommended to be sent at least with the first record, but can be updated any time
/airline        | String used as _operator code_ in [CSL model matching](https://twinfan.gitbook.io/livetraffic/reference/faq#matching)
/reg            | String used as _special livery_ in [CSL model matching](https://twinfan.gitbook.io/livetraffic/reference/faq#matching)
/call           | String used for computing a default label
/label          | String directly determining the label
` `             | ` `
**type/**       | Optional object with plane type information, recommended to be sent at least with the first record, but can be updated any time
/icao           | [ICAO aircraft type designator](https://www.icao.int/publications/DOC8643/Pages/Search.aspx) used in [CSL model matching](https://twinfan.gitbook.io/livetraffic/reference/faq#matching), defaults to `A320`
/wingSpan       | Wing span in meters, used for [wake turbulence configuration](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/)
/wingArea       | Wing area in square meters, used for [wake turbulence configuration](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/)
` `             | ` `
**positions/**  | **Mandatory** object with position information
/lat            | latitude, float with decimal coordinates
/lon            | longitude, float with decimal coordinates
/alt_geo        | geometric altitude in feet, integer, optional/ignored if `gnd = true`.
/gnd            | boolean value stating if plane is on the ground, optional, defaults to `false`
` `             | Means: Either `gnd = true` or `alt_geo` is required.
/timestamp      | timestamp, either a float with a relative timestamp in seconds, a float with a [Unix epoch timestamp](https://www.epochconverter.com) including decimals, or an integer with a Java epoch timestamp (ie. a Unix epoch timestamp in milliseconds). See section [Timestamp](#timestamp) for more details.
` `             | ` `
**attitude/**   | Optional object with plane attitude information
/roll           | roll in degrees, float, negative is left
/heading        | heading in degress, integer `0 .. 359`
/pitch          | pitch in degrees, float, negative is down
` `             | ` `
**config/**     | Optional object with plane configuration data (unlike `type/` this is data which is likely to change throughout a flight)
/mass           | mass of the plane in `kg`, used for [wake turbulence configuration](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/)
/lift           | current lift in Newton, optional, defaults to `mass * earth gravity`, used for [wake turbulence configuration](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/)
/gear           | gear extension, float `0.0 .. 1.0` with `1.0` fully extended
/noseWheel      | direction of nose wheel in degrees, float, negative left, `0.0` straight ahead
/flaps          | flap extension, float `0.0 .. 1.0` with `1.0` fully extended
/spoiler        | spoiler extension, float `0.0 .. 1.0` with `1.0` fully extended
` `             | ` `
**light/**      | Optional object with a set of boolean values for the plane's lights
/taxi           | taxi light
/landing        | landing lights
/beacon         | beacon light
/strobe         | strobe lights
/nav            | navigation lights

#### Data Array

Traffic data recods can be sent individually, but to make better use of the network message
several records can also be put into a JSON array. You are esponsible for ensuring that the
total message doesn't exceed network buffers.
8192 bytes is typically safe, see `NetBufSize` config options.

A proper array message starts directly with `[` as root element like this:

```
[
  { "id" : 4711, ... },
  { "id" : "001c0abc", ... },
  { "id" : 1234, ...}
]
```
See `docs/XPPTraffic_Array.json` for an example.

### RTTFC

`RTTFC` is a comparibly simple CSV-style format defined by RealTraffic.
See `docs/RTTFC.csv` for an example.
See [RealTraffic's documentation](https://www.flyrealtraffic.com/RTdev2.0.pdf) for details, section "RTTFC/RTDEST format".
[LiveTraffic's `SendTraffic.py`](https://github.com/TwinFan/LiveTraffic/blob/master/Resources/SendTraffic.py) script
can send such data to any UDP port, ie. also to the port defined for XPPlanes in `NetBcstPort`.

`RTTFC` does not include fields for configuration or light information and also misses other
attributes like pitch or any attributes required for detailed [wake turbulence configuration](https://developer.x-plane.com/article/plugin-traffic-wake-turbulence/).
