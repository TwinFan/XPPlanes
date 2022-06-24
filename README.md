# Hello-World-SDK-3

[![Build all Platforms](https://github.com/TwinFan/HelloWorld-XP/actions/workflows/build.yml/badge.svg)](https://github.com/TwinFan/HelloWorld-XP/actions/workflows/build.yml)

This is a "Hello World" example plugin for use with X-Plane and
can serve as a basis for your own plugin development.
Its code is originally taken from
[X-Plane's developer site](https://developer.x-plane.com/code-sample/hello-world-sdk-3/).

Added are an IDE project for XCode as well as a `CMakeList.txt` setup
for use with MS Visual Studio, the provided Docker environment for cross platform build,
and GitHub Actions.

## Credits
The project is based on
- [X-Plane SDK](https://developer.x-plane.com/sdk/plugin-sdk-documents/) to integrate with X-Plane
- Laminar's [Hello World example](https://developer.x-plane.com/code-sample/hello-world-sdk-3/)
- [GitHub Actions](https://docs.github.com/en/actions)
- and countless blog entries with crucial tips&tricks of various developers and
  CD/CI engineers, which I unfortunately missed to keep track of.

## Build

### IDE

- MacOS: Just open `HelloWorld.xcodeproj` in Xcode.
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
