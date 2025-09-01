
// Windows platform task processing code

#ifdef _WIN32

#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"
// Clipboard and socket
#include <windows.h>
#include <psapi.h> // Memory management
// Encoding conversion
#include <codecvt>
std::wstring_convert<std::codecvt_utf8<wchar_t>> conv_Ustr_Wstr; // Bidirectional converter between string utf-8 and wstring utf-16
// Socket
#pragma comment(lib, "ws2_32.lib")

namespace PaddleOCR
{

    // ==================== Utility functions ====================

    // Convert multibyte ANSI character array to wide character array
    wchar_t *char_2_wchar(char *c)
    {
        setlocale(LC_ALL, "");                   // Set program's locale to current Windows system locale
        size_t lenWchar = mbstowcs(NULL, c, 0);  // Get length after converting to wide string
        wchar_t *wc = new wchar_t[lenWchar + 1]; // Wide string to store filename
        int n = mbstowcs(wc, c, lenWchar + 1);   // Convert multibyte to wide character
        setlocale(LC_ALL, "C");                  // Restore locale to default
        return wc;
    }

    // Specially for message wstring to string conversion, return default prompt text when conversion fails
    std::string msg_wstr_2_ustr(std::wstring &msg)
    {
        try
        {
            std::string msgU8 = conv_Ustr_Wstr.to_bytes(msg); // Convert back to u8
            return msgU8;
        }
        catch (...)
        {
            return "wstring failed to convert to utf-8 string";
        }
    }

    // Check if pathW is a file, return true if yes
    bool is_exists_wstr(std::wstring pathW)
    {
        struct _stat buf;
        int result = _wstat((wchar_t *)pathW.c_str(), &buf);
        if (result != 0)
        { // Error occurred
            return false;
        }
        if (S_IFREG & buf.st_mode)
        { // Is file
            return true;
        }
        // else if (S_IFDIR & buf.st_mode) { // Is directory
        // return false;
        // }
        return false;
    }

    // ==================== Class implementation ====================

    // Get current memory usage. Return integer in MB. Return -1 on failure.
    int Task::get_memory_mb()
    {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            return static_cast<int>(pmc.WorkingSetSize / (1024 * 1024)); // Unit MB
        }
        else
        {
            std::cerr << "Error: GetProcessMemoryInfo" << std::endl;
            return -1; // Get failed
        }
    }

    // Replace cv::imread, read an image from pathW. pathW must be unicode wstring
    cv::Mat Task::imread_wstr(std::wstring pathW, int flag)
    {
        std::string pathU8 = msg_wstr_2_ustr(pathW); // Convert back to utf-8 for error output.
        // ↑ Since this function may be reused by clipboard CF_UNICODETEXT, caller may only provide wstring, so convert once more.
        if (!is_exists_wstr(pathW))
        {                                                               // Path does not exist
            set_state(CODE_ERR_PATH_EXIST, MSG_ERR_PATH_EXIST(pathU8)); // Report status: path does not exist and cannot output
            return cv::Mat();
        }
        FILE *fp = _wfopen((wchar_t *)pathW.c_str(), L"rb"); // Cast wpath to whar_t, try to open file
        if (!fp)
        {                                                             // Open failed
            set_state(CODE_ERR_PATH_READ, MSG_ERR_PATH_READ(pathU8)); // Report status: cannot read
            return cv::Mat();
        }
        // Read file into memory
        fseek(fp, 0, SEEK_END);                // Set file position of stream fp to SEEK_END end of file
        long sz = ftell(fp);                   // Get current file position of stream fp, i.e. total size (bytes)
        char *buf = new char[sz];              // Store file byte content
        fseek(fp, 0, SEEK_SET);                // Set file position of stream fp to SEEK_SET beginning of file
        long n = fread(buf, 1, sz, fp);        // Read data from given stream fp to array pointed by buf, return total number of elements successfully read
        cv::_InputArray arr(buf, sz);          // Convert to OpenCV array
        cv::Mat img = cv::imdecode(arr, flag); // Decode memory data into cv::Mat data
        delete[] buf;                          // Release buf space
        fclose(fp);                            // Close file
        if (img.empty())
        {
            set_state(CODE_ERR_PATH_DECODE, MSG_ERR_PATH_DECODE(pathU8)); // Report status: decode failed
        }
        return img;
    }

    // Replace cv imread, receive utf-8 string input, return Mat.
    cv::Mat Task::imread_u8(std::string pathU8, int flag)
    {
#if defined(_WIN32) && defined(ENABLE_CLIPBOARD)
        if (pathU8 == u8"clipboard")
        { // If clipboard task
            return imread_clipboard(flag);
        }
#endif
        // string u8 to wchar_t
        std::wstring wpath;
        try
        {
            wpath = conv_Ustr_Wstr.from_bytes(pathU8); // Use converter to convert
        }
        catch (...)
        {
            set_state(CODE_ERR_PATH_CONV, MSG_ERR_PATH_CONV(pathU8)); // Report status: convert to wstring failed
            return cv::Mat();
        }
        return imread_wstr(wpath);
    }

#if defined(_WIN32) && defined(ENABLE_CLIPBOARD)
    // Read an image from clipboard, return Mat. Note flag is invalid for clipboard memory image, only valid for clipboard path image.
    cv::Mat Task::imread_clipboard(int flag)
    {
        // Reference documentation: https://docs.microsoft.com/zh-cn/windows/win32/dataxchg/using-the-clipboard

        // Try to open clipboard, lock to prevent other applications from modifying clipboard content
        if (!OpenClipboard(NULL))
        {
            set_state(CODE_ERR_CLIP_OPEN, MSG_ERR_CLIP_OPEN); // Report status: clipboard open failed
        }
        else
        {
            static UINT auPriorityList[] = {
                // Allowed clipboard formats:
                CF_BITMAP, // Bitmap
                CF_HDROP,  // File list handle (file manager selected files copy)
            };
            int auPriorityLen = sizeof(auPriorityList) / sizeof(auPriorityList[0]);  // List length
            int uFormat = GetPriorityClipboardFormat(auPriorityList, auPriorityLen); // Get current clipboard content format
            // Assign different tasks based on format.
            //     If task succeeds, release all resources, close clipboard, return image mat.
            //     If task fails, release opened resources and lock, report status, break from switch, uniformly close clipboard and return empty mat
            switch (uFormat)
            {

            case CF_BITMAP:
            {                                                     // 1. Bitmap ===================================================================
                HBITMAP hbm = (HBITMAP)GetClipboardData(uFormat); // 1.1. Get pointer from clipboard, get file handle
                if (hbm)
                {
                    // GlobalLock(hbm); // Return value is always invalid, reading bitmap seems not to need lock?
                    // https://social.msdn.microsoft.com/Forums/vstudio/en-US/d2a6aa71-68d7-4db0-8b1f-5d1920f9c4ce/globallock-and-dib-transform-into-hbitmap-issue?forum=vcgeneral
                    BITMAP bmp;                           // Store pointer to buffer, buffer receives information about specified graphics object
                    GetObject(hbm, sizeof(BITMAP), &bmp); // 1.2. Get graphics object information (not including image content itself)
                    if (!hbm)
                    {
                        set_state(CODE_ERR_CLIP_GETOBJ, MSG_ERR_CLIP_GETOBJ); // Report status: retrieve graphics object information failed
                        break;
                    }
                    int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8; // Calculate channels based on color depth, 32bit is 4, 24bit is 3
                    // 1.3. Copy bitmap from handle hbm to buffer
                    long sz = bmp.bmHeight * bmp.bmWidth * nChannels;                                // Image size (bytes)
                    cv::Mat mat(cv::Size(bmp.bmWidth, bmp.bmHeight), CV_MAKETYPE(CV_8U, nChannels)); // Create empty matrix, pass bitmap size and depth
                    long getsz = GetBitmapBits(hbm, sz, mat.data);                                   // Copy sz bytes from handle hbm to buffer img.data
                    if (!getsz)
                    {
                        set_state(CODE_ERR_CLIP_BITMAP, MSG_ERR_CLIP_BITMAP); // Report status: get bitmap data failed
                        break;
                    }
                    CloseClipboard(); // Release resources
                    // 1.4. Return appropriate channels
                    if (mat.data)
                    {
                        if (nChannels == 1 || nChannels == 3)
                        { // 1 or 3 channels, PPOCR can recognize, return directly
                            return mat;
                        }
                        else if (nChannels == 4)
                        { // 4 channels, PPOCR cannot recognize, remove alpha to 3 channels then return
                            cv::Mat mat_c3;
                            cv::cvtColor(mat, mat_c3, cv::COLOR_BGRA2BGR); // Color space conversion
                            return mat_c3;
                        }
                        set_state(CODE_ERR_CLIP_CHANNEL, MSG_ERR_CLIP_CHANNEL(nChannels)); // Report status: channel count abnormal
                        break;
                    }
                    // In theory, !getsz should have broken, won't reach here. For safety, report once more
                    set_state(CODE_ERR_CLIP_BITMAP, MSG_ERR_CLIP_BITMAP); // Report status: get bitmap data failed
                    break;
                }
                set_state(CODE_ERR_CLIP_DATA, MSG_ERR_CLIP_DATA); // Report status: get clipboard data failed
                break;
            }

            case CF_HDROP:
            {                                                   // 2. File list handle ===========================================================
                HDROP hClip = (HDROP)GetClipboardData(uFormat); // 2.1. Get file list handle
                if (hClip)
                {
                    // https://docs.microsoft.com/zh-CN/windows/win32/api/shellapi/nf-shellapi-dragqueryfilea
                    GlobalLock(hClip);                                      // 2.2. Lock global memory object
                    int iFiles = DragQueryFile(hClip, 0xFFFFFFFF, NULL, 0); // 2.3. 0xFFFFFFFF means return file list count
                    if (iFiles != 1)
                    { // Only allow 1 file
                        GlobalUnlock(hClip);
                        set_state(CODE_ERR_CLIP_FILES, MSG_ERR_CLIP_FILES(iFiles)); // Report status: file count not 1
                        break;
                    }
                    // for (int i = 0; i < iFiles; i++) {
                    int i = 0;                                       // Only take the 1st file
                    UINT lenChar = DragQueryFile(hClip, i, NULL, 0); // 2.4. Get buffer size needed to read the i-th filename (bytes)
                    wchar_t *nameW = new wchar_t[lenChar + 1];       // Store filename byte content
                    DragQueryFileW(hClip, i, nameW, lenChar + 1);    // 2.5. Read the i-th filename
                    cv::Mat mat = imread_wstr(nameW, flag);          // 2.6. Try to read file
                    // Release resources
                    delete[] nameW;
                    GlobalUnlock(hClip); // 2.x.1 Release file list handle
                    CloseClipboard();    // 2.x.2 Close clipboard
                    return mat;
                }
                set_state(CODE_ERR_CLIP_DATA, MSG_ERR_CLIP_DATA); // Report status: get clipboard data failed
                break;
            }

            case NULL:                                              // Clipboard is empty
                set_state(CODE_ERR_CLIP_EMPTY, MSG_ERR_CLIP_EMPTY); // Report status: clipboard is empty
                break;
            case -1:                                                  // Other unsupported formats
            default:                                                  // Unknown
                set_state(CODE_ERR_CLIP_FORMAT, MSG_ERR_CLIP_FORMAT); // Report status: clipboard format not supported
                break;
            }
            CloseClipboard(); // Close clipboard for break cases, so other windows can continue to access clipboard.
        }
        return cv::Mat();
    }
#endif

    // Socket mode
    int Task::socket_mode()
    {
        // Initialize Winsock library
        WSADATA wsa_data; // Winsock structure
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            std::cerr << "Failed to initialize Winsock." << std::endl;
            return -1;
        }
        // Create socket, protocol family is TCP/IP
        SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_fd == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket." << std::endl;
            WSACleanup();
            return -1;
        }
        // Configure address and port number
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; // Address family: IPv4
        // IP address mode: local loopback/any available/other IPv4
        unsigned int my_s_addr;
        if (addr_to_uint32(FLAGS_addr, my_s_addr) < 0)
        {
            std::cerr << "Failed to parse input address." << std::endl;
            closesocket(server_fd);
            return -1;
        }
        addr.sin_addr.s_addr = static_cast<ULONG>(my_s_addr);
        addr.sin_port = htons(FLAGS_port); // Port number
        // Bind address and port number to socket handle server_fd
        if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            std::cerr << "Failed to bind address." << std::endl;
            closesocket(server_fd);
            WSACleanup();
            return -1;
        }
        // Set socket server_fd to listen state, only allow 1 client to queue connection
        if (listen(server_fd, 1) == SOCKET_ERROR)
        {
            std::cerr << "Failed to set listen." << std::endl;
            closesocket(server_fd);
            WSACleanup();
            return -1;
        }
        // Get actual server ip and port
        struct sockaddr_in server_addr;
        int len = sizeof server_addr;
        if (getsockname(server_fd, (SOCKADDR *)&server_addr, &len) != 0)
        {
            std::cerr << "Failed to get sockname." << std::endl;
            closesocket(server_fd);
            WSACleanup();
            return -1;
        }
        int server_port = ntohs(server_addr.sin_port); // Get port number
        char *server_ip = inet_ntoa(addr.sin_addr);    // Get ip address
        // int server_port = ntohs(addr.sin_port);
        std::cout << "Socket init completed. " << server_ip << ":" << server_port << std::endl;

        // Loop to wait for receiving connections
        while (true)
        {
            // Accept connection request
            struct sockaddr_in client_addr;
            int client_addr_len = sizeof(client_addr);
            SOCKET client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_addr_len);
            if (client_fd == INVALID_SOCKET)
            {
                std::cerr << "Failed to accept connection." << std::endl;
                continue;
            }
            // Get actual client ip and port
            char *client_ip = inet_ntoa(client_addr.sin_addr);
            int client_port = ntohs(client_addr.sin_port);
            std::cerr << "Client connected. IP address: " << client_ip << ":" << client_port << std::endl;

            // Receive arbitrary length data
            std::string str_in; // Data storage
            char buffer[1024];  // Buffer
            int n = 0;
            while (true)
            {
                n = recv(client_fd, buffer, sizeof(buffer), 0);
                if (n <= 0)
                { // Client may have disconnected or connection error
                    break;
                }
                str_in.append(buffer, n); // Append received data to end of storage
                if (buffer[n - 1] == '\0' || buffer[n - 1] == '\n')
                { // Reach end marker, consider data fully received
                    break;
                }
            }

            if (n == 0) // Client gracefully shutdown socket (end of file)
            {
                std::cerr << "Client has gracefully shutdown the socket." << std::endl;
            }
            else if (n < 0) // Connection error
            {
                std::cerr << "Failed to receive data, error code: " << n << std::endl;
                closesocket(client_fd);
                continue;
            }
            std::cerr << "Get string. Length: " << str_in.length() << std::endl;

            // =============== OCR start ===============
            set_state(); // Initialize state
            // Get ocr result
            std::string str_out = run_ocr(str_in);
            if (is_exit)
            { // Exit
                // Close connection
                closesocket(client_fd);
                break;
            }
            // =============== OCR end ===============

            // Send data
            std::cerr << str_out << std::endl;
            int m = send(client_fd, str_out.c_str(), strlen(str_out.c_str()), 0);
            if (m <= 0)
            {
                std::cerr << "Failed to send data." << std::endl;
            }

            // Close connection
            closesocket(client_fd);
            // Check and cleanup memory
            Task::memory_check_cleanup();
        }

        // Close socket
        closesocket(server_fd);

        // Cleanup Winsock library resources
        WSACleanup();

        return 0;
    }

} // namespace PaddleOCR

#endif