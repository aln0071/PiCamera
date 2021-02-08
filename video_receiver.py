#!/usr/bin/env python

#     This is the video receiver program that is to be executed on the system
#     listening to the Raspberry Pi video feed.
#     Copyright (C) 2021  Alan Kuriakose

#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.

#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.

#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <https://www.gnu.org/licenses/>.

from __future__ import division
import cv2
import numpy as np
import socket
import struct

MAX_DGRAM = 2 ** 16


def dump_buffer(s):
    """ Emptying buffer frame """
    while True:
        seg, addr = s.recvfrom(MAX_DGRAM)
        print(seg[0])
        if struct.unpack("B", seg[0:1])[0] == 1:
            print("finish emptying buffer")
            break


def main():
    """ Getting image udp frame &
    concate before decode and output image """

    # Set up socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('192.168.9.100', 12345))
    dat = b''
    dump_buffer(s)

    while True:
        seg, addr = s.recvfrom(MAX_DGRAM)
        dat += seg[1:]
        if struct.unpack("B", seg[0:1])[0] == 1:
            try:
                img = cv2.imdecode(np.frombuffer(dat, dtype=np.uint8), 1)
                cv2.imshow('frame', cv2.rotate(img, cv2.ROTATE_180))
            except cv2.error as error:
                print("invalid image data:")
                print(dat)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
            dat = b''

    # cap.release()
    cv2.destroyAllWindows()
    s.close()


if __name__ == "__main__":
    main()
