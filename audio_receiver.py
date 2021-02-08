#!/usr/bin/env python

#     This is the audio receiver program that is to be executed on the Raspberry Pi.
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
from threading import Thread

chunk = 1024    # Record in chunks of 1024 samples
sample_format = pyaudio.paInt16 # 16 bits per sample
channels = 2
fs = 44100  # Record at 44100 samples per second

frames = []

UDP_IP_ADDRESS = "127.0.0.1"
UDP_PORT = 6789

audio_port = pyaudio.PyAudio()
stream = audio_port.open(format=sample_format,
                         channels=channels,
                         rate=fs,
                         frames_per_buffer=chunk,
                         output=True)
#                         output_device_index=0)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

s.bind((UDP_IP_ADDRESS, UDP_PORT))

def get_data():
    while True:
        data, addr = s.recvfrom(chunk * 2 * channels)
        frames.append(data)

def play_sound():
    while True:
        if len(frames) >= 10:
            while len(frames) > 0:
                stream.write(frames.pop(0), chunk)

# while True:
#     data, addr = s.recvfrom(chunk * 2 * channels)
#     frames.append(data)
#     if len(frames) >= 10:
#         stream.write(frames.pop(0), chunk)

# # Stop and close the stream
# stream.stop_stream()
# stream.close()
#
# # Terminate the PortAudio interface
# audio_port.terminate()
#
# # close socket
# s.close()

getter_thread = Thread(target=get_data)
player_thread = Thread(target=play_sound)

getter_thread.setDaemon(True)
player_thread.setDaemon(True)

getter_thread.start()
player_thread.start()

getter_thread.join()
player_thread.join()
