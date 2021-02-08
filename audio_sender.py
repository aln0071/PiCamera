#!/usr/bin/env python

#     This is the audio sender program that is to be executed on the system
#     sending audio data to the Raspberry Pi.
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

import pyaudio
import socket

chunk = 1024    # Record in chunks of 1024 samples
sample_format = pyaudio.paInt16 # 16 bits per sample
channels = 2
fs = 44100  # Record at 44100 samples per second

UDP_IP_ADDRESS = "192.168.1.105"
UDP_PORT = 6789

audio_port = pyaudio.PyAudio()
stream = audio_port.open(format=sample_format,
                         channels=channels,
                         rate=fs,
                         frames_per_buffer=chunk,
                         input=True)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
    data = stream.read(chunk)
    s.sendto(data, (UDP_IP_ADDRESS, UDP_PORT))

# Stop and close the stream
stream.stop_stream()
stream.close()

# Terminate the PortAudio interface
audio_port.terminate()

# close socket
s.close()
