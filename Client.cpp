#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>      // 用于时间函数
#include <cstring>    // 用于strerror函数
#include <numeric>    // 用于std::accumulate
#include <algorithm>  // 用于std::min_element和std::max_element
#include <cmath>      // 用于sqrt
#include <thread>     // 用于多线程
#include <atomic>     // 用于原子操作
#pragma warning(disable:4996) 

#pragma comment(lib, "ws2_32.lib")

std::vector<std::string> myStrings;
std::atomic<bool> running(true); // 用于控制线程运行

enum req {
    Seqno = 0,
    Ver = 2,
};

void handleUserInput(SOCKET clientSocket, sockaddr_in serverAddr, int serverAddrSize) {
    std::string input;
    while (running) {
        std::getline(std::cin, input);
        if (input == "exit") {// 如果输入是"exit"退出
            sendto(clientSocket, "FIN-ACK", strlen("FIN-ACK"), 0, (sockaddr*)&serverAddr, serverAddrSize);
            running = false;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    char buffer[1024];
    int serverAddrSize = sizeof(serverAddr);

    // 初始化Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 创建UDP套接字
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "套接字创建失败" << std::endl;
        return 1;
    }

    // 让用户输入IP地址和端口号
    std::string ip;
    int port;
    std::cout << "请输入服务器的IP地址: ";
    std::cin >> ip;
    std::cout << "请输入服务器的端口号: ";
    std::cin >> port;
    std::cin.ignore();  // 清除换行符

    // 服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 发送SYN
    sendto(clientSocket, "SYN", strlen("SYN"), 0, (sockaddr*)&serverAddr, serverAddrSize);

    // 接收SYN-ACK
    int recvLen = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverAddrSize);
    buffer[recvLen] = '\0';
    if (strcmp(buffer, "SYN-ACK") == 0) {
        sendto(clientSocket, "ACK", strlen("ACK"), 0, (sockaddr*)&serverAddr, serverAddrSize);
        std::cout << "连接已经建立" << std::endl;

        // 创建一个线程来处理用户输入
        std::thread inputThread(handleUserInput, clientSocket, serverAddr, serverAddrSize);

        // 用于计算RTT的时间点
        std::clock_t start;
        std::clock_t end;
        std::vector<double> rtt_values;
        int sent_packets = 0;
        int received_packets = 0;
        std::time_t first_response_time = 0;
        std::time_t last_response_time = 0;

        for (int i = 1; i <= 12 && running; ++i) {
            std::string request = std::to_string(Ver) + std::to_string(Seqno + i) + "Request ";
            sendto(clientSocket, request.c_str(), request.length(), 0, (sockaddr*)&serverAddr, serverAddrSize);
            myStrings.push_back(request);
            std::cout << "发送请求包序列号 " << Seqno + i << std::endl;
            ++sent_packets;

            // 开始计时
            start = std::clock();
            // 添加随机延迟，以模拟网络延迟
            int delay = rand() % 1000; // 0到999毫秒的延迟
            Sleep(delay);

            timeval timeout;
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            recvLen = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverAddrSize);
            // 在合适的位置获取当前时间
            std::time_t currentTime = std::time(nullptr);
            struct std::tm* timeInfo = std::localtime(&currentTime);
            char timeBuffer[9];
            std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeInfo);

            // 结束计时
            end = std::clock();

            // 计算RTT，以毫秒为单位
            double rtt = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
            if (recvLen == SOCKET_ERROR) {
                // 超时，重新发送两次
                for (int retry = 0; retry < 2 && running; ++retry) {
                    sendto(clientSocket, request.c_str(), request.length(), 0, (sockaddr*)&serverAddr, serverAddrSize);
                    //std::cout << "发送请求包序列号: " << Seqno + i << std::endl;
                    std::cout << Seqno + i << "  request time out " << std::endl;
                    ++sent_packets;
                    // 开始计时
                    start = std::clock();
                    // 添加随机延迟，以模拟网络延迟
                    int delay = rand() % 1000; // 0到999毫秒的延迟
                    Sleep(delay);

                    timeout.tv_sec = 2;
                    timeout.tv_usec = 0;
                    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
                    recvLen = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverAddrSize);

                    // 结束计时
                    end = std::clock();

                    // 计算RTT，以毫秒为单位
                    rtt = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
                    if (recvLen != SOCKET_ERROR) {
                        break;
                    }
                }
            }

            if (recvLen != SOCKET_ERROR) {
                buffer[recvLen] = '\0';
                char* ipStr = inet_ntoa(serverAddr.sin_addr);
                int port = ntohs(serverAddr.sin_port);

             

                std::cout << "RTT: " << rtt << " milliseconds" << std::endl;
                std::cout << Seqno + i << ", Server IP: " << ipStr << ", Port: " << port << std::endl;
                rtt_values.push_back(rtt);
                std::cout << "当前时间: " << timeBuffer << std::endl;
                ++received_packets;
                if (first_response_time == 0) {
                    first_response_time = currentTime;
                }
                last_response_time = currentTime;
            }
            else {
                // 输出超时信息
                std::cout << "放弃重传" << std::endl;
            }
        }

        // 计算统计信息
        double max_rtt = *std::max_element(rtt_values.begin(), rtt_values.end());
        double min_rtt = *std::min_element(rtt_values.begin(), rtt_values.end());
        double avg_rtt = std::accumulate(rtt_values.begin(), rtt_values.end(), 0.0) / rtt_values.size();
        double sum_sq_diff = std::accumulate(rtt_values.begin(), rtt_values.end(), 0.0, [avg_rtt](double acc, double rtt) {
            return acc + (rtt - avg_rtt) * (rtt - avg_rtt);
            });
        double stddev_rtt = std::sqrt(sum_sq_diff / rtt_values.size());
        double packet_loss_rate = (1.0 - (double)received_packets / sent_packets) * 100.0;
        double total_response_time = std::difftime(last_response_time, first_response_time);

        // 打印统计信息
        std::cout << "接收到的UDP packets数目: " << received_packets << std::endl;
        std::cout << "丢包率: " << packet_loss_rate << "%" << std::endl;
        std::cout << "最大RTT: " << max_rtt << " milliseconds" << std::endl;
        std::cout << "最小RTT: " << min_rtt << " milliseconds" << std::endl;
        std::cout << "平均RTT: " << avg_rtt << " milliseconds" << std::endl;
        std::cout << "RTT的标准差: " << stddev_rtt << " milliseconds" << std::endl;
        std::cout << "server的整体响应时间: " << total_response_time << " seconds" << std::endl;
        std::cout << std::endl;

        // 等待输入线程结束
        inputThread.join();
    }

    // 关闭套接字
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
