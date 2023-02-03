# Fork notes

This fork of leikareipa's VCS aim to:
- maintain a 64-bit Windows version, and provide executables for it

This branch differs from VCS v2.6.1 in the following way:
- Qt5 replaced with Qt6 (6.5.1)

Below is the original README:

# VCS

A well-featured control application for Datapath's VisionRGB capture cards, with a feature-set targeted especially at retro enthusiasts.

![VCS 2.4](./images/vcs-2.4-with-dialogs.png)\
*A screenshot of VCS 2.4 showing the capture window (in the background) and some of the control dialogs.*

## Features

- Various feature and usability improvements over Datapath's default capture application to maximize capture quality
- Unlimited video presets with programmable activation
- Several scaling modes and image filters
- Variable refresh rate output to match the input
- Works with modern and legacy VisionRGB hardware
- Runs on Windows and Linux
- Free and open source

## Supported capture hardware

VCS is compatible with at least the following Datapath capture cards:

- VisionRGB-PRO1
- VisionRGB-PRO2
- VisionRGB-E1
- VisionRGB-E2
- VisionRGB-E1S
- VisionRGB-E2S
- VisionRGB-X2

## Manuals

### User's manual

The VCS user's manual is available for the following versions of VCS:

- [master branch](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/) (includes changes not yet released in a versioned distributable)
- [2.6.0](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.6.0/)
- [2.5.2](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.2/)
- [2.5.1](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.1/)
- [2.5.0](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.0/)
- [2.4.0](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.4.0/)

### Developer's manual

- [Source](./docs/developer-manual/)
- [Pre-compiled](https://www.tarpeeksihyvaesoft.com/vcs/docs/developer-manual/) (updated with a delay)

## Building

Run `$ qmake && make` in the repo's root; or open [vcs.pro](vcs.pro) in Qt Creator. You may need to modify some of the include paths in [vcs.pro](vcs.pro) to match your system.

### Dependencies

The VCS codebase depends on the following third-party libraries and frameworks:

1. Qt 5
2. OpenCV 3.2.0*
3. The Datapath capture API (RGBEasy on Windows, Vision/Video4Linux on Linux), as distributed with their capture drivers**

\* Later versions of OpenCV may also work. See [this issue](https://github.com/leikareipa/vcs/issues/30) for details.\
\** Only needed if compiling for Datapath hardware, which is the default option. See [The capture backend](#the-capture-backend) for details.

I use the following toolchains when developing VCS:

| OS      | Compiler           | Qt   | OpenCV |
| ------- | ------------------ | ---- | ------ |
| Linux   | GCC 9.4 (64-bit)   | 5.12 | 3.2.0  |

### Release build vs. debug build

The default configuration in [vcs.pro](vcs.pro) produces a debug build, with &ndash; among other things &ndash; more copious run-time bounds-checking of memory accesses. The run-time debugging features are expected to reduce performance to some extent, but can help reveal programming errors.

Defining `RELEASE_BUILD` globally will produce a release build, with fewer debugging checks in performance-critical sections of the program. Simply uncomment `DEFINES += RELEASE_BUILD` at the top of [vcs.pro](vcs.pro) and execute a full rebuild.

To confirm whether the program is running in release or debug mode, check the About dialog (right-click VCS's output window and select "About VCS&hellip;"). For debug builds, the program's version will be reported as "VCS x.x.x (non-release build)", whereas for release builds it'll be "VCS x.x.x".

### The capture backend

A capture backend in VCS is an implementation providing support for a particular capture device. For example, Datapath's VisionRGB capture cards are supported in VCS on Windows by the [RGBEasy capture backend](./src/capture/rgbeasy/), which uses the Datapath RGBEasy API.

One (and only one) capture backend must be active in any build of VCS. To select which one it is, set its identifier in the `DEFINES` variable in [vcs.pro](vcs.pro). By default, VCS uses Datapath's RGBEasy (CAPTURE_BACKEND_RGBEASY) on Windows and Vision (CAPTURE_BACKEND_VISION_V4L) on Linux, supporting the VisionRGB range of capture cards on those platforms.

The following capture backends are available:

| Identifier                  | Explanation |
| --------------------------- | ----------- |
| CAPTURE_BACKEND_RGBEASY     | Supports the Datapath VisionRGB range of capture cards on Windows; requires the Datapath VisionRGB driver to be installed. If used when building on Linux, will provide a debug implementation that announces in the terminal which RGBEasy API functions would be called if you were to run the program on Windows. |
| CAPTURE_BACKEND_VISION_V4L  | Supports the Datapath VisionRGB range of capture cards on Linux; requires the Datapath Vision driver to be installed. |
| CAPTURE_BACKEND_VIRTUAL     | Provides a device-independent capture source that generates a test image. Useful for debugging, and doesn't require capture drivers to be installed. |
| CAPTURE_BACKEND_DOSBOX_MMAP | Allows capturing DOSBox's frame buffer on Linux. Intended for debugging. See [this blog post](https://www.tarpeeksihyvaesoft.com/blog/capturing-dosboxs-frame-buffer-via-shared-memory/) for details.|

## Program flow

VCS is a mostly single-threaded application whose event loop is synchronized to the capture device's rate of output. In general, VCS's main loop polls the capture device until a capture event (e.g. new frame) occurs, then processes the event, and returns to the polling loop.

![](./images/diagrams/code-flow.png)

 The above diagram shows a general step-by-step view of VCS's program flow. `Capture` communicates with the capture device (which will typically be running in its own thread), receiving frame data and setting hardware-side capture parameters. `Main` polls `Capture` to receive information about capture events. When it receives a new frame from `Capture`, `Main` sends the frame's data to `Scale` for scaling, which then forwards it to `Filter` for image filtering, which in turn sends it to `Display` to be rendered onto VCS's output window.

| System  | Corresponding source code          |
| ------- | ---------------------------------- |
| Main    | [src/main.cpp](./src/main.cpp)     |
| Capture | [src/capture/*](./src/capture/)    |
| Scale   | [src/scaler/*](./src/scaler/)      |
| Filter  | [src/filter/*](./src/filter/)      |
| Display | [src/display/*](./src/display/)    |
