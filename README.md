# alarm

Alarm clock interface for Raspberry PI

The Raspberry PI 1B CPU is slow, but the board is surprisingly fast for OpenGL rendering.
The goal is to create an alarm clock:

- without the need of network. The time must be set in the interface
- which outputs to a small screen
- which keeps the CPU low (hence the C++ and some attention on the performances, where the memory allocation is performed...).  
  At 25fps, it takes ~2% of CPU on a Raspberry PI 1B.

For this project, I have chosen to use these parts:

- Raspberry PI 1B (recycling my good old board)
- TFT screen driven by a ILI9341 chip
- AM2320 temperature / humidity sensor
- we will see in the future when the basic minimal part is done

**Remark** I have not yet received many parts, this is still work in progress. Right now, it runs better on a Linux computer.

This programs is aimed at running on a Raspbian for Raspberry PI 1B without X11/Wayland. To ease the development, it also runs on a Debian / Ubuntu x86.

Tested on:

- Debian 10 (Buster): tested on x86\_64
- Ubuntu 18.04 (Bionic Beaver): tested on x86\_64. To build the unittests, you have to remove `-lgtest` from the Makefile as Google Test is an old version
- Raspbian 10 (Buster): work in progress

## Features

In the end, I expect to have the following features:

- display output on TFT screen
- input from the TFT touchscreen
- sound output by the Raspberry Jack
- music for the alarms stored on the Raspberry PI's filesystem
- temperature sensor by the [thermal API](https://www.kernel.org/doc/html/latest/driver-api/thermal/sysfs-api.html) (`/sys/class/thermal`)
- temperature / humidity by [hwmon API](https://www.kernel.org/doc/html/latest/hwmon/hwmon-kernel-api.html) (`/sys/class/hwmon`)

## Dependencies

This programs needs a C++17 compiler (tested with G++ 6 and 8).

The dependencies:

- a graphical library to create an OpenGL context. Either:
  - [SDL2](https://www.libsdl.org/), which is nicer for development and the most compatible. License: [ZLIB](http://libsdl.org/license.php)
  - [Wayland](https://wayland.freedesktop.org/) window as it uses EGL like Raspberry PI, but you have to run Wayland. License: [MIT Expat](https://wayland.freedesktop.org/faq.html)
  - Raspberry PI's [dispman](https://github.com/raspberrypi/firmware/blob/master/opt/vc/src/hello_pi/hello_triangle2/triangle2.c), but there is no good documentation. License: [BSD](https://github.com/raspberrypi/firmware/blob/master/opt/vc/include/bcm_host.h)
- [ALSA](https://alsa-project.org/wiki/Main_Page): play sound. License: [LGPL](https://www.alsa-project.org/wiki/Introduction) for the part we link against
- [RapidJSON](https://rapidjson.org/): decode/encode Configuration in JSON. License: [MIT](https://rapidjson.org/)
- [libmodplug](http://modplug-xmms.sourceforge.net/): decode MOD. License: [Public domain](https://sourceforge.net/p/modplug-xmms/git/ci/master/tree/libmodplug/COPYING)
- [mpg123](https://mpg123.org/): decode MP3. License: [LGPL](http://mpg123.org/)
- [vorbis](https://xiph.org/vorbis/): decode OGG Vorbis. License: [BSD](https://github.com/xiph/vorbis/blob/master/COPYING)
- EGL / OpenGL ES 2

For the tests:

- [Google Test](https://github.com/google/googletest). License: [BSD](https://github.com/google/googletest/blob/master/LICENSE)

Imagemagick, Inkscape, FFMpeg, LCOV are not linked against any binary.

```bash
# build main program
$ apt install build-essential rapidjson-dev libasound2-dev libmodplug-dev libmpg123-dev libvorbis-dev

# build assets
$ apt install imagemagick inkscape

# build + run tests
$ apt install libgtest-dev lcov ffmpeg

# for x86 development, choose one of these (don't do it for Raspberry PI)
$ apt install libsdl2-dev
$ apt install libwayland-dev
```

## Compile

There are several environment variables to build:

- `DEBUG` change the build flags. It can have several values:
  - `0`: turn on compilation optimizations + meant to be used with `make install` (default). You will have to update the generated configuration file by hand to be able to use it without install
  - `1`: turn on compilation optimizations and add symbols. To be run on callgrind
  - `2`: turn off all optimization, add symbols, turn on GCOV
- `VERBOSE` only change the make output, it doesn't do anything on the final binaries:
  - `0` default: turn off verbose compilation output (default)
  - `1`: turn on verbose compilation output

The usual make targets:

- `all` build the program and the assets (default)
- `alarm` build only the program
- `test` build and run the tests. If run with `DEBUG=2`, it also generates the GCOV report

```bash
# build the tests + run them + generate GCOV
$ make clean
$ DEBUG=2 make test -j2

# build the program in DEBUG=1 mode + run it under callgrind
$ make clean
$ DEBUG=1 make -j2
$ valgrind --tool=callgrind ./alarm config_debug.json

# build and install the program under /opt/local/alarm
$ make clean
$ make install -j2
$ /opt/local/alarm/alarm config.json
```

## Run

### Command line

To run, simply execute the following commands. The config.json is created with default values if it does not exist.

```bash
# if only DEBUG=1 make or DEBUG=2 make
$ ./alarm config.json

# if make install
$ /opt/local/alarm/alarm config.json
```

### Configuration file

The configuration file is a JSON file.

```json
{
    "alsa_device": "default",
    "assets_folder": "/opt/local/alarm/assets",
    "display_driver": "sdl",
    "display_width": 320,
    "display_height": 240,
    "display_seconds": true,
    "frames_per_second": 25,
    "sensor_thermal": "INT3400 Thermal",
    "hand_clock_color": [
        255,
        0,
        0
    ],
    "alarms": [
        {
            "active": true,
            "file": "MENU.MOD",
            "hours": 8,
            "minutes": 0,
            "duration_minutes": 59
        },
        {
            "active": true,
            "file": "MENU.MOD",
            "hours": 17,
            "minutes": 0,
            "duration_minutes": 59
        }
    ]
}
```

The entries:

- `alsa_device` you may change it if you want another ALSA device. `default` should be OK for most.
- `assets_folder` where the assets (`shader`, `music`, `textures`) are located
- `display_driver` can be either:
  - `sdl` for SDL2 driver
  - `wayland` for very basic Wayland driver
  - `raspberrypi_hdmi` to output to Raspberry's Dispman. This is only to run in a console. **There is currently no mouse input**
  - `raspberrypi_tft` to output to `/dev/fb1` which is in my case a TFT touchscreen connected by SPI. You have to perform some configuration on Raspbian before using it. **It is completely untested due to a pending postal delivery and it misses the touchscreen part :/**
- `display_width` / `display_height` to scale the display, mostly for development
- `display_seconds` to display the seconds in the main screen along with hours and minutes
- `frames_per_second` fixed frames per seconds to save CPU. We don't need 200fps for an alarm clock
- `sensor_thermal` name of the thermal sensor in `/sys/class/thermal`. It is set in a screen in the interface
- `hand_clock_color` color of the clock hands. Bright red by default
- `alarms` list of alarms set. It is set in a screen in the interface

### Screens

There are few screens. From left to right (navigation by clicking on upper-right/upper-left corners of the screen):

- `screen_main` default screen. We see a clock, the temperature if configured and the next alarm if any. You may leave this screen by clicking on the upper-right corner of the screen
- `screen_set_alarm` add/remove alarms, set the time and disable/enable them
- `screen_set_alarm_file` browse the available musics for the alarms
- `screen_set_date` as the device is meant to be run without network, one has to set the hour. This needs `CAP_SYS_TIME` privilege to run. Useless if you use NTP to get the date and time
- `screen_set_sensor` choose the sensor from the ones available in `/sys/class/thermal`

### Add musics

The musics must be in a handled format and put in the folder `<assets_folder>/music` where `<assets_folder>` is the entry in config.json:

```json
{
    "assets_folder": "some_folder"
}
```

## Develop

The code is organized this way:

- `src` contains all the C++ source code
- `test` contains all the C++ unittests
- `assets/shader` contains all the shaders 
- `assets/textures` contains the source of the textures (font + svg)
- `build` is created at `make` time. It contains the object files and assets

### src

The code is organized to avoid useless `#include`. Not many headers are included in the `.hpp` files.

Many classes have a member `std::unique_ptr<Impl> pimpl` to avoid exposing headers.
OpenGL classes have a member `Guard guard` to destroy the OpenGL resource at object destruction time.

Main loop:

- `app.hpp` / `app.cpp` contain the main application with the main loop. It owns the objects
- `context.hpp` / `context.cpp` is a big context for the application
- `alarm.hpp` / `alarm.cpp` handle which alarms to run and when to start / stop them

Configuration:

- `config*` stores the configuration. It can be serialized
- `serializable.hpp` / `serializable.hpp` is the base class for all serializable objects
- `serializer*` handle the serialization (only implementation: JSON)

Audio part:

- `audio_read*` convert from a file on the hard drive to a PCM buffer
- `audio.hpp` / `audio.cpp` handles the interactions with ALSA, fills the audio buffers from `audio_read*`

Graphical part:

- `gl_*` handle the interactions with OpenGL
- `renderer*` render the elements on screen
- `screen*` 1 class per screen on the application. The main screen is `screen_main.hpp` / `screen_main.cpp`, the others are for configuration
- `window*` create an OpenGL context and get the events (TODO for Raspberry PI)

Misc:

- `toolbox*` various tools for compatibility, centralization of headers and aliases
- `error.hpp` / `error.cpp` exception with file, line, and function
- `event.hpp` / `event.cpp` simple struct to get the events from the Window objects

### test

The test files are names the same way as the files in `src` with a `test_` prefix.

To run the test with code coverage:

```bash
$ make clean
$ DEBUG=2 make test -j2
```

### assets/shader

The code is very basic. We are just displaying sprites and rotating, no wonderful 3D effect.

### assets/textures

The textures are stored as a "source": TTF or SVG.  
During the compilation, the DDS files are generated on the fly.

## Common errors

### Startup: std::exception: Could not open file

If you get this kind of output at startup:

```
...
std::exception: Could not open file /opt/local/alarm/assets/shader/print_texture.vert
```

Check in the `config.json` file the following line:

```json
{
    "assets_folder": "some_folder"
}
```

`assets_folder` should be existing and have 3 sub-folders:
- `music`: empty by default, where you put your musics
- `shader`: contains `*.vert` and `*.frag` files
- `textures`: contains `*.dds` files

If compiled in `DEBUG=0` or `DEBUG` unset, the default folder is `/opt/local/alarm/assets`, like in the example above.
If you don't want to `make install`, just update the `assets_folder` entry in the config.json file to `build/assets` and re-run `./alarm config.json`.

If compiled in `DEBUG=1` or `DEBUG=2`, the folder is `<something>/build/assets`. It is intended to be for development only, so no `make install`. Again you need to change the value in the `config.json`

## TODO

Code:

- Raspberry HDMI must use `/dev/input/mice` for the events
- Better tests for the graphical (screen + renderer + window) and audio part (but hardware dependent)
- Internationalization. For now this is in French :)
- Unittest `audio_read_mod.hpp` / `audio_read_mod.cpp`: play with a tracker to add a 1 second file with a single sound
- Unittest more classes, but I don't want to use GMock as it would need `virtual` in a lot of methods :/
- Create a Debian package to ease the deployment once the code is stable enough

Due to an some package delivery:

- TFT output (need to output with dispman to a layer then copy to `/dev/fb1`)
- PWM for the TFT backlight?
- TFT touchscreen input
- AM2320 sensor (plus a Linux hwmon driver, plus support `/sys/class/hwmon`)
- DS3231 real time clock (there is an [official Linux driver](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/rtc/rtc-ds1307.c) so it should be transparent)

## License

MIT. See [LICENSE](LICENSE) file
