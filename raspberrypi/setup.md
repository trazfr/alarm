# Setup

This file is to setup the Rasperry PI

## Wiring

The GPIOs are the ones in `/sys/class/gpio/` ([source](https://elinux.org/RPi_Low-level_peripherals#General_Purpose_Input.2FOutput_.28GPIO.29)).

![GPIO headers](40pingpio.svg)

| TFT      | PI                    |
|----------|-----------------------|
| LED      | Pin12 (GPIO18)        |
| SCK      | Pin23 (GPIO11) = SCLK |
| MOSI     | Pin19 (GPIO10) = MOSI |
| DC       | Pin18 (GPIO24)        |
| RESET    | Pin22 (GPIO25)        |
| CS       | Pin24 (GPIO8) = CE0   |
| GND      | Pin25 (GND)           |
| VCC      | Pin17 (3.3V)          |

## Commands

### Rights

```bash
# rights to read from /dev/input/event*
$ sudo usermod -a -G input $USER
```

### Enable the framebuffer

```bash
# in root
$ sed -i 's/^#\(dtparam=spi=on\)$/\1/' /boot/config.txt
$ echo "fbtft_device" >| /etc/modules-load.d/fbtft_device.conf
$ echo "options fbtft_device name=fb_ili9341 gpios=reset:25,dc:24,led:18 speed=48000000 txbuflen=32768 custom=1 rotate=90 fps=20 bgr=0" >| /etc/modprobe.d/fbtft.conf
$ systemctl disable getty@tty1.service

$ sudo reboot
```
