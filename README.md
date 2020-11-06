Hook your Pi 400 up to your PC somehow, using a USB Type-C cable into the *power* port.
Anker make good ones- I used a 3m white one for my tests.

Add `dtoverlay=dwc2` to `/boot/config.txt`

Run `sudo modprobe libcomposite`

Run `wget https://gist.github.com/Gadgetoid/5a8ceb714de8e630059d30612503653f/raw/48050eb8fcf6b32d30033402000ccbcadf322dc9/pi400kb`

Run `sudo ./pi400kb`

YOUR PI 400 IS NOW A FREAKING KEYBOARD FOR YOUR PC WHAAAAT!?

Press `Ctrl + Raspberry` to exit.

Use `sudo ./runpi400.sh` to detach your keyboard from X. You might need to run `xinput list` and find the ID of "Raspberry Pi Internal Keyboard" here.
