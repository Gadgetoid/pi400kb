# Raspberry Pi 400 as a USB HID Keyboard

Hook your Pi 400 up to your PC somehow, using a USB Type-C cable into the *power* port.
Anker make good ones- I used a 3m white one for my tests.

## Setup

Add `dtoverlay=dwc2` to `/boot/config.txt`

Reboot!

Run `sudo modprobe libcomposite`

RUn `wget https://gist.github.com/Gadgetoid/5a8ceb714de8e630059d30612503653f/raw/35fde8da7fcd88e7ccd3913c729f2b14bbd4a0a7/pi400kb`

Run `sudo ./pi400kb`

YOUR PI 400 IS NOW A FREAKING KEYBOARD FOR YOUR PC WHAAAAT!?

Your keyboard input will be detached from your Pi while it's forwarded to your host computer.

Press `Ctrl + Raspberry` to exit and restore your keyboard on the Pi.
