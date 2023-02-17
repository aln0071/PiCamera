# Pi Camera CPP Implementation

This is the CPP implementation of the Pi Camera project. As of now, I have only programmed the video streaming part and have not done anything about the audio. But this implementation has the following advantages:

- Faster program as it is implemented in C++.
- No need to install OpenCV on the Raspberry Pi as it uses V4L2 which is the inbuild video driver of linux.
- Has AES-256 encryption to add security to the stream.

pc.cpp is the program that has to be run on the laptop/computer receiving the video. pi.cpp is to be run on the Raspberry Pi.

My laptop was running Ubuntu 22.04 and to compile the pc.cpp program, I had to initially install OpenCV. The steps to install OpenCV on Ubuntu 22.04 are as follows:

Step 1: Install required build tools:

```
sudo apt-get install build-essential cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
```

Step 2: Download OpenCV source code from GitHub.

```
mkdir ~/opencv_build && cd ~/opencv_build
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
```

Step 3: Create a temporary build directory and navigate to it:

```
cd ~/opencv_build/opencv
mkdir -p build && cd build
```

Step 4: Setup OpenCV build with CMake:

```
cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D OPENCV_EXTRA_MODULES_PATH=~/opencv_build/opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..
```

Step 5: Start the compilation process:

```
make -j8
```

> Note: The -j flag mentions the number of processors available. It depends on your system. To find this value, run `nproc` command which will return the number of processors available on your system. Say for example it returns number 4, then you should run `make -j4` instead of `make -j8`.

Step 6: Install OpenCV

```
sudo make install
```

<hr/>
To verify the opencv installation, run:
C++ bindings:
```
pkg-config --modversion opencv4
```
Python bindings:
```
python3 -c "import cv2; print(cv2.__version__)"
```

If the installation was successful, the above commands will show the OpenCV version.

References used to install OpenCV:

- [https://linuxize.com/post/how-to-install-opencv-on-ubuntu-20-04/](https://linuxize.com/post/how-to-install-opencv-on-ubuntu-20-04/)
- [https://www.geeksforgeeks.org/how-to-install-opencv-in-c-on-linux/](https://www.geeksforgeeks.org/how-to-install-opencv-in-c-on-linux/)
- [http://techawarey.com/programming/install-opencv-c-c-in-ubuntu-18-04-lts-step-by-step-guide/](http://techawarey.com/programming/install-opencv-c-c-in-ubuntu-18-04-lts-step-by-step-guide/)

For AES-256 encryption, we will have to install the required OpenSSL libraries using the following command:

```
sudo apt-get install libssl-dev
```

Now, compile the program using the following command:

```
g++ -std=c++11 pc.cpp `pkg-config --libs --cflags opencv4` -o pc -lssl -lcrypto
```

Now, we have our client ready. Next step is to put the required code on the Raspberry Pi and start it. To do so, place the pi.cpp file in the Raspberry Pi. Install OpenSSL on Raspberry Pi with the following command:

```
sudo apt-get install libssl-dev
```

Now, compile the pi.cpp program with following command:

```
g++ pi.cpp -o pi -lssl -lcrypto
```

Now, start the program on the Raspberry Pi with the following command:

```
./pi
```

Now, start the client on your personal laptop/pc with the following command:

```
./pc
```

If you did everything correct, you should see the feed from the Raspberry Pi camera on your pc/laptop.

> Be sure to modify the IP address in the pc.cpp program so that it listens to the address of the Raspberry Pi. Change port number is required. By default, the port is 8080. Change the AES-256 key and IV. To generate a new random key, use the command `pwgen -N 1 -s 256` on Ubuntu.

References used:
[https://gist.github.com/sammy17/b391c68a91f381aad0d149e325e6a87e](https://gist.github.com/sammy17/b391c68a91f381aad0d149e325e6a87e)
[https://medium.com/@athul929/capture-an-image-using-v4l2-api-5b6022d79e1d](https://medium.com/@athul929/capture-an-image-using-v4l2-api-5b6022d79e1d)
[https://gist.github.com/maxlapshin/1253534](https://gist.github.com/maxlapshin/1253534)
[https://web.archive.org/web/20110520211256/http://v4l2spec.bytesex.org/spec/capture-example.html](https://web.archive.org/web/20110520211256/http://v4l2spec.bytesex.org/spec/capture-example.html)
[http://derekmolloy.ie/beaglebone/beaglebone-video-capture-and-image-processing-on-embedded-linux-using-opencv/](http://derekmolloy.ie/beaglebone/beaglebone-video-capture-and-image-processing-on-embedded-linux-using-opencv/)

The original implementation was using UDP which is in the `udp` folder. I implemented TCP because my ISP is not opening UDP ports. The TCP implementation is little glitchy as it is a connection-oriented protocol. I am not sure if there can be a better implementation without glitches using TCP, but you are welcome to rase a PR with better implementation if you have one.

The UDP version is not fully tested as I could not test it on the real Raspberry Pi due to the port forwarding issue. So, it might need few upgrades to actually be deployed.

> Future upgrades: This implementation is single threaded and so, cannot support multiple parallel streams. I did not care to make it multithreaded as I am using a Raspberry Pi Zero W and I doubt if it can handle so much of load. Also I am the only user who would view my Raspberry Pi :D. Multithreading can be added if you have a Pi with better processing power and if your usecase needs it.
