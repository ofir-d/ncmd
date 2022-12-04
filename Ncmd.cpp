// Ncmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <string_view>
#include <tuple>
#pragma comment(lib, "Ws2_32.lib")
constexpr unsigned int kBufferLen = 1024;
constexpr unsigned int kMaximumBytesToRead = 1024;
const char* ip = "127.0.0.1";
const char* port= "6969";


int main(int argc, char** argv)
{
    int recvLen{ kBufferLen };
    

    char recvBuffer[kBufferLen];
    char sendBuffer[kBufferLen];
    STARTUPINFOA cmd_si;
    PROCESS_INFORMATION cmd_pi{};
    SecureZeroMemory(&cmd_si, sizeof(cmd_si));
    SecureZeroMemory(&cmd_pi, sizeof(cmd_pi));
    cmd_si.cb = sizeof(cmd_si);
    cmd_si.dwFlags |= STARTF_USESTDHANDLES;
    HANDLE pipeStdIn_Rd, pipeStdIn_Wr, pipeStdOut_Rd, pipeStdOut_Wr = NULL;
    SECURITY_ATTRIBUTES pipeSA{};
    SecureZeroMemory(&pipeSA, sizeof(pipeSA));
    pipeSA.nLength = sizeof(pipeSA);
    pipeSA.bInheritHandle = TRUE;    
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.S_un.S_addr = inet_addr(ip);
    server.sin_port = htons(atoi(port));
    int status_code = 0;

    if (WSAStartup(wVersionRequested, &wsaData))
    {
        std::cout << "Starting winsock failed with error: " << WSAGetLastError();
        return 1;
    }
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Creating server socket failed: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    status_code = bind(serverSocket, (SOCKADDR*)&server, sizeof(server));
    if (status_code == SOCKET_ERROR)
    {
        std::cout << "Bind failed: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    status_code = listen(serverSocket, SOMAXCONN);
    if (status_code == SOCKET_ERROR)
    {
        std::cout << "Listen failed: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    SOCKET clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Accept failed: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    
    if (!CreatePipe(&pipeStdIn_Rd, &pipeStdIn_Wr, &pipeSA, 0) || !CreatePipe(&pipeStdOut_Rd, &pipeStdOut_Wr, &pipeSA, 0))
    {
        std::cout << "Pipe failed with: " << GetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    if (!SetHandleInformation(pipeStdIn_Rd, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) || !SetHandleInformation(pipeStdOut_Wr, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
    {
        std::cout << "Could not change pipe permissions" << GetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    cmd_si.hStdInput = pipeStdIn_Rd;
    cmd_si.hStdOutput = pipeStdOut_Wr;
    cmd_si.hStdError = pipeStdOut_Wr;
    if (!CreateProcessA("C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &cmd_si, &cmd_pi))
    {
        std::cout << "Creating process failed with: " << GetLastError();
        closesocket(serverSocket);
        return 1;
    }
    DWORD numberOfBytesWritten{ 0 };
    DWORD numberOfBytesRead{ kBufferLen };
    do {
        SecureZeroMemory(recvBuffer, recvLen);
        SecureZeroMemory(sendBuffer, numberOfBytesRead);
        Sleep(100);
        if (!ReadFile(pipeStdOut_Rd, sendBuffer, kMaximumBytesToRead, &numberOfBytesRead, NULL))
        {
            std::cout << "Could not read from pipe with error: " << GetLastError();
            closesocket(serverSocket);
            WSACleanup();
            if (TerminateProcess(cmd_pi.hProcess, 1))
            {
                std::cout << "Could not terminate process with error: " << GetLastError();
            }
        }
        if (send(clientSocket, sendBuffer, numberOfBytesRead, 0) == SOCKET_ERROR)
        {
            std::cout << "Sending Output failed with error: " << WSAGetLastError();
            closesocket(serverSocket);
            WSACleanup();
            if (TerminateProcess(cmd_pi.hProcess, 1))
            {
                std::cout << "Could not terminate process with error: " << GetLastError();
            }
            return 1;
        }
        recvLen = recv(clientSocket, recvBuffer, kBufferLen, 0);
        if (recvLen > 0)
        {
            if (!WriteFile(pipeStdIn_Wr, recvBuffer, recvLen, &numberOfBytesWritten,NULL))
                std::cout << GetLastError();

        }
        else if (recvLen == SOCKET_ERROR)
        {
            std::cout << "Recieving failed with error: " << WSAGetLastError();
            closesocket(serverSocket);
            WSACleanup();
            if (TerminateProcess(cmd_pi.hProcess, 1))
            {
                std::cout << "Could not terminate process with error: " << GetLastError();
            }
            return 1;
        }

    } while (recvLen > 0);
    TerminateProcess(cmd_pi.hProcess,1);
    CloseHandle(cmd_pi.hProcess);
    CloseHandle(cmd_pi.hThread);
    closesocket(serverSocket);
    closesocket(clientSocket);
    CloseHandle(pipeStdIn_Rd);
    CloseHandle(pipeStdOut_Wr);
    CloseHandle(pipeStdOut_Rd);
    CloseHandle(pipeStdIn_Wr);

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
