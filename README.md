# Raspberry Pi 400 as a USB HID Keyboard

Hook your Pi 400 up to your PC somehow, using a USB Type-C cable into the *power* port.
Anker make good ones- I used a 3m white one for my tests.

## Setup

Add `dtoverlay=dwc2` to `/boot/config.txt`

Reboot!

Run `sudo modprobe libcomposite`

Run `wget https://gist.github.com/Gadgetoid/5a8ceb714de8e630059d30612503653f/raw/1f150cc0f5cf2ad5f937cefafcdf222090d811a0/pi400kb`

Run `sudo ./pi400kb`

YOUR PI 400 IS NOW A FREAKING KEYBOARD FOR YOUR PC WHAAAAT!?

Press `Ctrl + Raspberry` to exit.

## Detach keyboard from Pi

Run `wget https://gist.github.com/Gadgetoid/5a8ceb714de8e630059d30612503653f/raw/1f150cc0f5cf2ad5f937cefafcdf222090d811a0/runpi400.sh`

Run `xinput list` and find the ID of `Raspberry Pi Internal Keyboard`, also take note of the number in the brackets (it's probably 3).

Edit `runpi400.sh` with the ID and second number for the float/reattach commands.

Run `sudo ./runpi400.sh` and your keyboard will be detached from X, it will re-attach when you exit `pi400kb` with `Ctrl + Raspberry`
