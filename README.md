<!--
- SPDX-FileCopyrightText: None
- SPDX-License-Identifier: CC0-1.0
-->


# Plasma Camera <img src="logo.png" width="40"/>
A [libcamera](https://libcamera.org/) based camera application built for Plasma Mobile.

This application interfaces with `libcamera` directly, allowing for tighter integration with the camera stack and the implementation of hardware specific features. For a camera application that works at a higher level for simpler usecases, see [Kamoso](https://invent.kde.org/multimedia/kamoso).

## Features
* Taking photos
* Filming videos

## Links
* Project page: https://invent.kde.org/plasma-mobile/plasma-camera
* Development channel: https://matrix.to/#/#plasmamobile:matrix.org

## Requirements

* [libcamera](https://libcamera.org/)

## Installing

```
mkdir build
cd build
cmake .. # add -DCMAKE_BUILD_TYPE=Release to compile for release
make
sudo make install
```
