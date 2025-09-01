// Linux platform task processing code

#if defined(_LINUX) || defined(__linux__)

#include <iostream>
#include <fstream>

#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"

// Socket
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
// Memory management
#include <fstream>
#include <string>

#undef INVALID_SOCKET
#define INVALID_SOCKET -1

namespace PaddleOCR
{
    // Get current memory usage. Return integer in MB. Return -1 on failure.
    int Task::get_memory_mb()
    {
        try
        {
            // Open /proc/self/statm file, which contains current process memory usage
            std::ifstream statm("/proc/self/statm");
            if (!statm.is_open())
            {
                std::cerr << "Error: \"/proc/self/statm\" can not open" << std::endl;
                return -1;
            }

            long rss;            // Used to store RSS (Resident Set Size)
            statm >> rss >> rss; // Skip first value, read second value (RSS)
            statm.close();       // Close file
                                 // Verify if RSS value is valid
            if (rss < 0)
            {
                std::cerr << "Error: Invalid RSS value" << std::endl;
                return -1;
            }

            // Get system page size and convert to KB
            long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // In case x86-64 config uses 2MB pages
            // Calculate memory usage, convert to MB and return
            return static_cast<int>(rss * page_size_kb / 1024); // Convert to MB
        }
        catch (const std::exception &e)
        { // Catch standard exception
            std::cerr << "Error: " << e.what() << std::endl;
            return -1;
        }
        catch (...)
        { // Catch all other exceptions
            std::cerr << "Error: Linux get_memory_mb" << std::endl;
            return -1;
        }
    }

    // Replace cv imread, receive utf-8 string input, return Mat.
    cv::Mat Task::imread_u8(std::string pathU8, int flag)
    {
        std::ifstream fileInput;
        size_t fileLength;
        char *buffer;

        // Try to read pathU8 into memory
        try
        {
            fileInput.open(pathU8, std::ios::binary);
            // Path does not exist and cannot output
            if (!fileInput)
            {
                set_state(CODE_ERR_PATH_EXIST, MSG_ERR_PATH_EXIST(pathU8));
                return cv::Mat();
            }

            // Get file size and read file into memory
            fileInput.seekg(0, fileInput.end);
            fileLength = fileInput.tellg();
            fileInput.seekg(0, fileInput.beg);
            buffer = new char[fileLength];
            fileInput.read(buffer, fileLength);

            // Cannot read
            if (!fileInput)
            {
                set_state(CODE_ERR_PATH_READ, MSG_ERR_PATH_READ(pathU8));
                return cv::Mat();
            }
        }
        catch (std::exception &err)
        {
            // File open failed or read failed, default to read failed here
            set_state(CODE_ERR_PATH_READ, MSG_ERR_PATH_READ(pathU8));
            return cv::Mat();
        }

        // Decode memory data into cv::Mat data
        cv::_InputArray array(buffer, fileLength);
        cv::Mat image = cv::imdecode(array, flag);
        // Release buffer space must be after cv::imdecode(),
        // because cv::_InputArray() does not copy elements in buffer,
        // and cv::imdecode() will allocate new memory and copy decoded data when decoding
        delete[] buffer;
        fileInput.close();

        // Decode failed
        if (image.empty())
        {
            set_state(CODE_ERR_PATH_DECODE, MSG_ERR_PATH_DECODE(pathU8));
            return cv::Mat();
        }

        return image;
    }

    int Task::socket_mode()
    {
        // Create socket, protocol family TCP/IP
        int socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketFd == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket." << std::endl;
            close(socketFd);
            return -1;
        }

        // Configure address and port number
        struct sockaddr_in socketAddr;
        // Address family: IPv4
        socketAddr.sin_family = AF_INET;
        // IP address mode: loopback/any/other IPv4
        if (addr_to_uint32(FLAGS_addr, socketAddr.sin_addr.s_addr) < 0)
        {
            std::cerr << "Failed to parse input address." << std::endl;
            close(socketFd);
            return -1;
        }
        // Port number
        socketAddr.sin_port = htons(FLAGS_port);

        // Bind address and port number to socket handle socketFd
        if (bind(socketFd, (struct sockaddr *)&socketAddr, sizeof(socketAddr)) == INVALID_SOCKET)
        {
            std::cerr << "Failed to bind address." << std::endl;
            close(socketFd);
            return -1;
        }

        // Set socket socketFd to listen state, allow only 1 client to queue connection
        if (listen(socketFd, 1) == INVALID_SOCKET)
        {
            std::cerr << "Failed to set listen." << std::endl;
            close(socketFd);
            return -1;
        }

        // Get actual server ip and port
        struct sockaddr_in serverAddr;
        socklen_t len = sizeof(serverAddr);
        if (getsockname(socketFd, (sockaddr *)&serverAddr, &len) == INVALID_SOCKET)
        {
            std::cerr << "Failed to get sockname." << std::endl;
            close(socketFd);
            return -1;
        }
        // Get port number & ip address
        int serverPort = ntohs(serverAddr.sin_port);
        char *serverIp = inet_ntoa(socketAddr.sin_addr);
        std::cout << "Socket init completed. " << serverIp << ":" << serverPort << std::endl;

        // Loop waiting to receive connections
        while (true)
        {
            // Accept connection request
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientFd = accept(socketFd, (sockaddr *)&clientAddr, &clientAddrLen);
            if (clientFd == INVALID_SOCKET)
            {
                std::cerr << "Failed to accept connection." << std::endl;
                continue;
            }

            // Get actual client ip and port
            char *clientIp = inet_ntoa(clientAddr.sin_addr);
            int clientPort = ntohs(clientAddr.sin_port);
            std::cerr << "Client connected. IP address: " << clientIp << ":" << clientPort << std::endl;

            // Receive arbitrary length data
            std::string strIn; // Data storage location
            char buffer[1024]; // Buffer
            int bytesRecv = 0;
            while (true)
            {
                bytesRecv = recv(clientFd, buffer, sizeof(buffer), 0);

                // No data received, client may have disconnected or connection error
                if (bytesRecv <= 0)
                    break;

                // Append received data to end of storage
                strIn.append(buffer, bytesRecv);

                // Reach terminator, consider all data received
                if (buffer[bytesRecv - 1] == '\0' || buffer[bytesRecv - 1] == '\n')
                    break;
            }

            if (bytesRecv == 0) // Client gracefully shutdown socket
            {
                std::cerr << "Client has gracefully shutdown the socket." << std::endl;
            }
            else if (bytesRecv < 0) // Connection error
            {
                std::cerr << "Failed to receive data, error code: " << errno << std::endl;
                close(clientFd);
                continue;
            }
            std::cerr << "Get string. Length: " << strIn.length() << std::endl;

            // =============== OCR start ===============
            set_state(); // Initialize state
            // Get ocr result
            std::string strOut = run_ocr(strIn);
            // Received exit command, close connection, exit main loop, end server
            if (is_exit)
            {
                close(clientFd);
                break;
            }
            // =============== OCR end ===============

            // Send data
            std::cerr << strOut << std::endl;
            int bytesSent = send(clientFd, strOut.c_str(), strlen(strOut.c_str()), 0);
            // No data sent | sent 0 bytes
            if (bytesSent <= 0)
                std::cerr << "Failed to send data." << std::endl;

            // Close connection
            close(clientFd);
            // Check, cleanup memory
            Task::memory_check_cleanup();
        }

        // Close socket
        close(socketFd);

        return 0;
    }
}

#endif