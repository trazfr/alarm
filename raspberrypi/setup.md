# Setup

This file is to setup the Rasperry PI. Hardware:

 - Raspberry PI 1B
 - TFT screen controller: ILI9341
 - Touchscreen controller: XPT2046 (Linux driver: ADS7846)

## Wiring

The GPIOs are the ones in `/sys/class/gpio/` ([source](https://elinux.org/RPi_Low-level_peripherals#General_Purpose_Input.2FOutput_.28GPIO.29)).

![GPIO headers](40pingpio.svg)

| TFT      | PI (SPI0.0)           |
|----------|-----------------------|
| LED      | Pin12 (GPIO18)        |
| SCK      | Pin23 (GPIO11) = SCLK |
| MOSI     | Pin19 (GPIO10) = MOSI |
| DC       | Pin18 (GPIO24)        |
| RESET    | Pin22 (GPIO25)        |
| CS       | Pin24 (GPIO8) = CE0   |
| GND      | Pin25 (GND)           |
| VCC      | Pin17 (3.3V)          |

| Touch    | PI (SPI0.1)           |
|----------|-----------------------|
| T_IRQ    | Pin11 (GPIO17)        |
| T_DO     | Pin21 (GPIO9) = MISO  |
| T_DIN    | Pin19 (GPIO10) = MOSI |
| T_CS     | Pin26 (GPIO7) = CE1   |
| T_CLK    | Pin23 (GPIO11) = SCLK |

## Commands

### Rights

```bash
# rights to read from /dev/input/event*
$ sudo usermod -a -G input $USER
```

### Enable the TFT framebuffer

```bash
# as root
$ echo "dtparam=spi=on" >> /boot/config.txt
$ echo "fbtft_device" >| /etc/modules-load.d/fbtft_device.conf
$ echo "options fbtft_device name=fb_ili9341 gpios=reset:25,dc:24,led:18 speed=64000000 txbuflen=32768 custom=1 rotate=90 fps=20 bgr=0" >| /etc/modprobe.d/fbtft.conf
$ systemctl disable getty@tty1.service
```

### Enable the touchscreen

```bash
# as root
$ echo "dtoverlay=ads7846,penirq=17,speed=100000,penirq_pull=2,xohms=80,swapxy=1" >> /boot/config.txt
```

### Temperature + humidity

I have a AM2320, but the Linux module is am2315, but it is not part of Raspberry's packages. We can see that the dependencies are here.

Install module:

```bash
sudo apt-get install raspberrypi-kernel-headers
$ mkdir /dev/shm/build_am2315
$ cd /dev/shm/build_am2315
$ curl -O https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/drivers/iio/humidity/am2315.c
$ echo 'obj-m+=am2315.o' >| Makefile
$ make -C /lib/modules/$(uname -r)/build M=$PWD modules
$ sudo make -C /lib/modules/$(uname -r)/build M=$PWD modules_install
$ sudo depmod -A
```

Configure it:

```bash
# as root
$ echo "dtparam=i2c=on" >> /boot/config.txt
$ echo "am2315" >| /etc/modules-load.d/am2315.conf
$ echo 'ACTION=="add", SUBSYSTEM=="i2c", ATTR{name}=="bcm2835 I2C adapter", ATTR{new_device}="am2315 0x5c"' >| /etc/udev/rules.d/99-i2c-am2315.rules
```
