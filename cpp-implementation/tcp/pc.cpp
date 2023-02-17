// Client - compile command:
// g++ -std=c++11 pc.cpp `pkg-config --libs --cflags opencv4` -o pc -lssl -lcrypto
#include <cmath>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/imgproc.hpp"

#include <iostream>

#define PORT 8080
#define ADDRESS "127.0.0.1"

using namespace std;
using namespace cv;

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

char *hello = "Hello from client";

std::vector<char> buffer;

int readAvailableData(int sock)
{
    // Wait for the buffer to reach a certain size
    int max_buffer_size = sizeof(struct Message);
    while (buffer.size() < max_buffer_size)
    {
        // Wait for some data to be available on the socket
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);
        timeval timeout = {};
        timeout.tv_sec = 5;
        int select_result = select(sock + 1, &read_set, nullptr, nullptr, &timeout);
        if (select_result == -1)
        {
            std::cerr << "Error waiting for data: " << strerror(errno) << std::endl;
            close(sock);
            return 1;
        }
        else if (select_result == 0)
        {
            std::cerr << "Timed out waiting for data" << std::endl;
            close(sock);
            return 1;
        }

        // Read the available data into the buffer
        char tmp_buffer[max_buffer_size - buffer.size()];
        ssize_t bytes_read = recv(sock, tmp_buffer, sizeof(tmp_buffer), 0);
        if (bytes_read == -1)
        {
            std::cerr << "Error reading data: " << strerror(errno) << std::endl;
            close(sock);
            return 1;
        }
        else if (bytes_read == 0)
        {
            std::cerr << "Server closed the connection" << std::endl;
            close(sock);
            return 1;
        }

        buffer.insert(buffer.end(), tmp_buffer, tmp_buffer + bytes_read);
    }
    return 0;
}

int receiveData(int sockfd)
{
    int n;
    // vector to store current frame
    vector<char> myFrame;

    struct Message *message = (Message *)malloc(sizeof(struct Message));
    cout << "size of message: " << sizeof(struct Message) << endl;

    while (1)
    {
        // Initialize the decryption context
        ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);

        // receive data
        int result = readAvailableData(sockfd);
        if (result != 0)
        {
            return result;
        }
        while (buffer.size() >= sizeof(struct Message))
        {
            vector<char>::const_iterator first = buffer.begin();
            vector<char>::const_iterator last = buffer.begin() + sizeof(struct Message);
            vector<char> subvector = vector<char>(first, last);
            cout << "buffer size before erasing: " << buffer.size() << endl;
            buffer.erase(first, last);
            const Message *buffer_as_struct = reinterpret_cast<const Message *>(subvector.data());
            memcpy(message, buffer_as_struct, sizeof(Message));

            cout << "received; size: " << n << ", segment: " << message->sequence << endl;
            cout << "remaining buffer size: " << buffer.size() << endl;

            // Buffer for plaintext
            unsigned char plaintext[message->size];
            int plaintext_len;
            // decrypt message
            // Initialize the decryption context
            ctx = EVP_CIPHER_CTX_new();
            EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);

            // Decrypt the encrypted message
            EVP_DecryptUpdate(ctx, plaintext, &plaintext_len, message->data, message->size);

            // Finalize the decryption
            EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len, &plaintext_len);

            // add data to frame vector
            myFrame.insert(myFrame.end(), plaintext, plaintext + message->size - AES_BLOCK_SIZE);

            if (message->sequence == 0)
            {
                break;
            }

            // Clean up the decryption context
            EVP_CIPHER_CTX_free(ctx);
        }
        if (message->sequence == 0)
        {
            vector<uchar> jpegData(myFrame.begin(), myFrame.end());
            Mat frame = imdecode(jpegData, IMREAD_UNCHANGED);
            double angle = 180;

            // get the center coordinates of the image to create the 2D rotation matrix
            Point2f center((frame.cols - 1) / 2.0, (frame.rows - 1) / 2.0);
            // using getRotationMatrix2D() to get the rotation matrix
            Mat rotation_matix = getRotationMatrix2D(center, angle, 1.0);

            // we will save the resulting image in rotated_image matrix
            Mat rotated_image;
            // rotate the image using warpAffine
            warpAffine(frame, rotated_image, rotation_matix, frame.size());
            imshow("v", rotated_image);
            myFrame.clear();
            break;
        }
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, ADDRESS, &serv_addr.sin_addr) <= 0)
    {
        printf(
            "\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client_fd = connect(sock, (struct sockaddr *)&serv_addr,
                             sizeof(serv_addr))) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    while (1)
    {
        // send intend to receive
        send(sock, hello, strlen(hello), 0);
        int result = receiveData(sock);
        if (result != 0)
        {
            break;
        }
        if (waitKey(1) == 'q')
        {
            break;
        }
    }

    // closing the connected socket
    destroyAllWindows();
    close(client_fd);
    return 0;
}