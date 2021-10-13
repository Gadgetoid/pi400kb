import time
import os
import struct

t_start = time.time()

device = "/dev/input/by-id/usb-DELL_Alienware_610M-event-mouse"
packet_size = 12
chunk_size = 6

cap_start = False

with open(device, "rb") as f:
    os.set_blocking(f.fileno(), False)
    while True:
        data = f.read(packet_size)
        if data and len(data) == packet_size:
            if not cap_start:
                print(f"{time.time() - t_start:.4f}\n")
            cap_start = True
            for offset in range(0, packet_size, chunk_size):
                chunk = data[offset:offset + chunk_size]
                fmt = "{:02x} " * len(chunk)
                print("  " + fmt.format(*chunk))
            if data[0:5] == b"\x00\x00\x00\x00\x04":
                print("  - Mouse Button {}".format(data[8]))
            elif data[0:5] == b"\x00\x00\x00\x00\x01":
                value = struct.unpack("<i", data[8:12])[0]
                state = "Pressed" if value else "Released"
                print(f"  - {value} ({state})")
            elif data[0:5] == b"\x00\x00\x00\x00\x02":
                value = struct.unpack("<i", data[8:12])[0]
                print("  - Mouse Axis {} ({})".format(data[6], value))
            elif data[0:5] == b"\x00\x00\x00\x00\x00":
                pass
            else:
                pass
            print("")
        else:
            if cap_start:
                print("")
                cap_start = False