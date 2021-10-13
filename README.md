# Raspberry Pi 400 as a USB HID Keyboard

Hook your Pi 400 up to your PC somehow, using a USB Type-C cable into the *power* port.
Anker make good ones- I used a 3m white one for my tests.

Our USB-C to USB-A is great if you're using a USB-A port (but make sure it's a *high power* one): https://shop.pimoroni.com/products/usb-c-to-usb-a-cable-1m-black

## Quickstart (Ish)

Add `dtoverlay=dwc2` to `/boot/config.txt`

Reboot!

`sudo modprobe libcomposite`

`wget https://github.com/Gadgetoid/pi400kb/blob/main/pi400kb?raw=true`

`chmod +x pi400kb`

`sudo ./pi400kb`

YOUR PI 400 IS NOW A FREAKING KEYBOARD FOR YOUR PC WHAAAAT!?

Your keyboard input will be detached from your Pi while it's forwarded to your host computer.

Press `Ctrl + Raspberry` to exit and restore your keyboard on the Pi.

## Building & Contributing

### Building

```
sudo apt install libconfig-dev
git clone https://github.com/Gadgetoid/pi400kb
cd pi400kb
git submodule update --init
mkdir build
cd build
cmake ..
make
```