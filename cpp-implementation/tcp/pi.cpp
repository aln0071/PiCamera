// Server - compile command:
// g++ pi.cpp -o pi -lssl -lcrypto
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cmath>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

#define PORT 8080

using namespace std;

// const int MAX_DGRAM = pow(2, 16);
// const int MAX_DATA_SIZE = MAX_DGRAM - 64 - (2 * 4) - AES_BLOCK_SIZE;
const int MAX_DATA_SIZE = 1024;

// define structure for receiving data
struct Message
{
    unsigned char data[MAX_DATA_SIZE + AES_BLOCK_SIZE];
    int sequence;
    int size;
};

// encryption
// Key and IV
unsigned char key[] = "0123456789abcdef";
unsigned char iv[] = "abcdefghijklmnop";

// Encryption context
EVP_CIPHER_CTX *ctx;

int sendData(int sockfd, v4l2_buffer bufferinfo, int fd, char *buffer)
{
    struct Message message = Message();

    // Queue the buffer
    if (ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0)
    {
        perror("Could not queue buffer, VIDIOC_QBUF");
        return 1;
    }

    // Dequeue the buffer
    if (ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0)
    {
        perror("Could not dequeue the buffer, VIDIOC_DQBUF");
        return 1;
    }

    int bufPos = 0, outFileMemBlockSize = 0;        // the position in the buffer and the amoun to copy from
                                                    // the buffer
    int remainingBufferSize = bufferinfo.bytesused; // the remaining buffer size, is decremented by
                                                    // memBlockSize amount on each loop so we do not overwrite the buffer
    char *outFileMemBlock = NULL;                   // a pointer to a new memory block
    int itr = 0;
    int sequenceNumber = ceil(remainingBufferSize / MAX_DATA_SIZE); // counts the number of iterations
    while (remainingBufferSize > 0)
    {
        // Initialize the encryption context
        ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);

        bufPos += outFileMemBlockSize; // increment the buffer pointer on each loop
                                       // initialise bufPos before outFileMemBlockSize so we can start
                                       // at the begining of the buffer

        outFileMemBlockSize = min(MAX_DATA_SIZE, remainingBufferSize); // set the output block size to a preferable size. max size possible

        // Buffer for ciphertext
        int ciphertext_len;

        // Encrypt the input character array
        EVP_EncryptUpdate(ctx, message.data, &ciphertext_len, (const unsigned char *)buffer + bufPos, outFileMemBlockSize);

        // Finalize the encryption
        EVP_EncryptFinal_ex(ctx, message.data + ciphertext_len, &ciphertext_len);

        // send data using tcp:
        message.sequence = sequenceNumber;
        message.size = outFileMemBlockSize + AES_BLOCK_SIZE;
        send(sockfd, (struct Message *)&message, sizeof(message), 0);

        // calculate the amount of memory left to read
        // if the memory block size is greater than the remaining
        // amount of data we have to copy
        if (outFileMemBlockSize > remainingBufferSize)
            outFileMemBlockSize = remainingBufferSize;

        // subtract the amount of data we have to copy
        // from the remaining buffer size
        remainingBufferSize -= outFileMemBlockSize;

        // display the remaining buffer size
        cout << itr++ << " Remaining bytes: " << remainingBufferSize << endl;
        sequenceNumber--;

        // Clean up the encryption context
        EVP_CIPHER_CTX_free(ctx);
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    // socket programming
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buf[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // v4l2 setup
    // 1.  Open the device
    int fd; // A file descriptor to the video device
    fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device, OPEN");
        return 1;
    }

    // 2. Ask the device if it can capture frames
    v4l2_capability capability;
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0)
    {
        // something went wrong... exit
        perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
        return 1;
    }

    // 3. Set Image format
    v4l2_format imageFormat;
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = 1024;
    imageFormat.fmt.pix.height = 1024;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
    // tell the device you are using this format
    if (ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0)
    {
        perror("Device could not set format, VIDIOC_S_FMT");
        return 1;
    }

    // 4. Request Buffers from the device
    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count = 1;                          // one request buffer
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // request a buffer wich we an use for capturing frames
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0)
    {
        perror("Could not request buffer from device, VIDIOC_REQBUFS");
        return 1;
    }

    // 5. Quety the buffer to get raw data ie. ask for the you requested buffer
    // and allocate memory for it
    v4l2_buffer queryBuffer = {0};
    queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queryBuffer.memory = V4L2_MEMORY_MMAP;
    queryBuffer.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0)
    {
        perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
        return 1;
    }
    // use a pointer to point to the newly created buffer
    // mmap() will map the memory address of the device to
    // an address in memory
    char *buffer = (char *)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                fd, queryBuffer.m.offset);
    memset(buffer, 0, queryBuffer.length);

    // 6. Get a frame
    // Create a new buffer type so the device knows whichbuffer we are talking about
    v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));
    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        // Activate streaming
        int type = bufferinfo.type;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
        {
            perror("Could not start streaming, VIDIOC_STREAMON");
            return 1;
        }

        while (1)
        {
            // Wait for some data to be available on the socket
            fd_set read_set;
            FD_ZERO(&read_set);
            FD_SET(new_socket, &read_set);
            timeval timeout = {};
            timeout.tv_sec = 5;
            int select_result = select(new_socket + 1, &read_set, nullptr, nullptr, &timeout);
            cout << "select_result: " << select_result << endl;
            if (select_result == -1)
            {
                std::cerr << "Error waiting for data: " << strerror(errno) << std::endl;
                close(new_socket);
                break;
            }
            else if (select_result == 0)
            {
                std::cerr << "Timed out waiting for data" << std::endl;
                close(new_socket);
                break;
            }
            // read the available data
            valread = read(new_socket, buf, 1024);
            cout << "after valread, valread: " << valread << endl;
            if (valread == 0)
            {
                cout << "Error reading data";
                close(new_socket);
                break;
            }
            else if (valread == -1)
            {
                cout << "Client closed the connection";
                close(new_socket);
                break;
            }
            else
            {
                printf("%s\n", buf);
                int result = sendData(new_socket, bufferinfo, fd, buffer);
                if (result == 0)
                    printf("Picture sent\n");
                else
                    cout << "Failed to send picture";
            }
        }

        // end streaming
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
        {
            perror("Could not end streaming, VIDIOC_STREAMOFF");
            return 1;
        }
        cout.flush();
    }
    // closing the listening socket
    close(fd);
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}