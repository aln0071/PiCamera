# Pi Camera
This project aims at building a low-cost, low-latency CCTV camera using the Raspberry Pi.

## Materials required
1) Raspberry Pi Zero W (Rs. 1299) - https://robu.in/product/raspberry-pi-zero-w/
2) Raspberry Pi Camera Module (Rs. 378) - https://www.amazon.in/gp/product/B00E1GGE40/ref=ppx_yo_dt_b_asin_title_o01_s01?ie=UTF8&psc=1
3) Raspberry Pi Camera Case (Rs. 170) - https://robu.in/product/official-raspberry-pi-zero-case/
4) Raspberry Pi Power Supply - I used a 5 volts mobile charger
5) Mini HDMI to HDMI cable (Rs. 120 - optional) - https://robu.in/product/mini-hdmi-hdmi-cable-1-8-meter-round-high-quality-copper-clad-steel-black/
6) OTG Cable (Rs. 50 - optional) - I bought this from a local mobile store.

## Setup

### Setp 1: Initial Software Setup
The Raspberry Pi Zero W has no standard USB port. So, I will explain how to set it up without connecting keyboard and mouse.
1) Download Raspbian image from https://www.raspberrypi.org/software/operating-systems/#raspberry-pi-os-32-bit. Image with desktop is better.
2) Download Etcher from https://www.balena.io/etcher/.
3) Insert the SD card and burn the downloaded Raspbian image onto the SD card using Etcher.
4) Mount the SD card again and open the boot partition.
5) Create an empty file 'ssh' to enable ssh access on boot.
6) Create a file 'wpa_supplicant.conf' with the following content:
```properties
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
 ssid="SSID"
 scan_ssid=1
 psk="PASSWORD"
 key_mgmt=WPA-PSK
}
```
Set the ssid and password to that of your wireless router.

7) Insert the SD card into the Raspberry Pi and boot it up. The Pi will automatically connect to the router specified in Step 6.
8) Connect to the Raspberry Pi using an ssh client like PuTTY. Default username is 'pi' and password is 'raspberry'.
9) Run command 'sudo raspi-config' command to open the Raspberry Pi configuration and enable the camera module from there.
10) Reboot the Pi for the changes to take effect.

### Step 2: Hardware Setup
Connect the Raspberry Pi to the Camera Module. Now, secure the camera in the case using some hot glue. Close the case with the Raspberry Pi and the camera inside it. If you place your camera close to your TV, then you can connect the Pi to the TV using the Mini HDMI to HDMI cable. Otherwise, this cable is not required. The OTG cable can be used to connect the Pi to a keyboard and mouse. This is also optional.

### Step 3: Running the Camera Code
1) Install OpenCV on the Raspberry Pi using the following command:
```shell script
sudo apt-get install python3-opencv
```
2) Run the camera sender code using Python 3.
3) Start the camera receiver code on your local system.

> Note: Make sure that both your local system and the Raspberry Pi are connected to the same network for any communication to take place.