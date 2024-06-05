#include <winsock2.h>
#include <iostream>
#include <cstdlib>    // 用于rand函数和srand函数
#include <ctime>      // 用于time函数
#include <string>
#include <sstream>    // 添加sstream头文件，用于字符串流操作
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    char buffer[1024];
    int clientAddrSize = sizeof(clientAddr);

    // 初始化Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 创建UDP套接字
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "套接字创建失败" << std::endl;
        return 1;
    }

    // 绑定套接字
    serverAddr.sin_family = AF_INET;// 设置服务器地址族为IPv4
    serverAddr.sin_port = htons(11111);
    serverAddr.sin_addr.s_addr = INADDR_ANY;// 设置服务器地址为任意地址
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "UDP服务器正在监听" << std::endl;

    // 客户端集合，使用一个unordered_map来存储每个客户端的状态
    std::unordered_map<std::string, bool> clients;

    while (true) {
        // 接收来自客户端的数据
        int recvLen = recvfrom(serverSocket, buffer, 1024, 0, (sockaddr*)&clientAddr, &clientAddrSize);
        if (recvLen == SOCKET_ERROR) {
            std::cerr << "接收数据失败" << std::endl;
            continue;
        }
        buffer[recvLen] = '\0';
        std::string clientKey = std::string(inet_ntoa(clientAddr.sin_addr)) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        std::cout << "消息来自客户端 " << clientKey << std::endl;

        if (strcmp(buffer, "SYN") == 0) {
            sendto(serverSocket, "SYN-ACK", strlen("SYN-ACK"), 0, (sockaddr*)&clientAddr, clientAddrSize);
            clients[clientKey] = true;
        }
        else if (strcmp(buffer, "ACK") == 0) {
            std::cout << "建立联系 " << clientKey << std::endl;
        }
        else if (strcmp(buffer, "FIN-ACK") == 0) {
            std::cout << "连接关闭" << clientKey << std::endl;
            clients.erase(clientKey);
        }
        else if (clients.find(clientKey) != clients.end()) { // 如果客户端已存在于集合中
            std::string sbuffer(buffer);
            std::istringstream iss(sbuffer.substr(sbuffer.find_first_of("0123456789") + 1)); // 去掉第一个整数

            // 输出剩余的整数值
            int num;
            while (iss >> num) {
                std::cout << "序列号 " << num << std::endl;
               
            }
            if (strstr(buffer, "Request")) {
                int i = rand();
                if (i % 4 != 0) { // 丢包的概率为20%
                    // 添加随机延迟，以模拟网络延迟
                    //int delay = rand() % 1000; // 0到999毫秒的延迟
                    sendto(serverSocket, buffer, recvLen, 0, (sockaddr*)&clientAddr, clientAddrSize);
                }
            }
        }
    }

    // 关闭套接字
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
