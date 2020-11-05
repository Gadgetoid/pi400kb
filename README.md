Hook your Pi 400 up to your PC somehow, using a USB Type-C cable into the *power* port.
Anker make good ones- I used a 3m white one for my tests.

Add `dtoverlay=dwc2` to `/boot/config.txt`

Run `sudo modprobe libcomposite`

Run `sudo ./pi400kb`

YOUR PI 400 IS NOW A FREAKING KEYBOARD FOR YOUR PC WHAAAAT!?