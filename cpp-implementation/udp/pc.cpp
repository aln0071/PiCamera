// compile command:
// g++ -std=c++11 pc.cpp `pkg-config --libs --cflags opencv4` -o pc -lssl -lcrypto
#include <cmath>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

using namespace std;
using namespace cv;

#define PORT 8080
const int MAX_DGRAM = pow(2, 16);
const int MAX_DATA_SIZE = MAX_DGRAM - 64;

// define structure for receiving data
struct Message
{
    unsigned char data[MAX_DATA_SIZE + AES_BLOCK_SIZE];
    int sequence;
    int size;
};

int main()
{
    // encryption
    // Key and IV
    unsigned char key[] = "0123456789abcdef";
    unsigned char iv[] = "abcdefghijklmnop";

    // Encryption context
    EVP_CIPHER_CTX *ctx;

    // socket programming
    int sockfd;
    struct sockaddr_in servaddr;
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // vector to store current frame
    vector<char> myFrame;

    struct Message *message = (Message *)malloc(sizeof(struct Message));
    int n;
    socklen_t len;

    while (1)
    {
        // Initialize the decryption context
        ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);

        // receive data
        n = recvfrom(sockfd, message, sizeof(Message), MSG_WAITALL, (struct sockaddr *)&servaddr, &len);

        cout << "received data from server with size " << n << endl;

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
            vector<uchar> jpegData(myFrame.begin(), myFrame.end());
            Mat frame = imdecode(jpegData, IMREAD_UNCHANGED);
            imshow("v", frame);
            // waitKey(0);
            if (waitKey(1) == 'q')
            {
                break;
            }
            myFrame.clear();
        }

        // Clean up the decryption context
        EVP_CIPHER_CTX_free(ctx);
    }

    destroyAllWindows();
    return 0;
}