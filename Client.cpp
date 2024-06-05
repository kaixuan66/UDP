#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>      // ����ʱ�亯��
#include <cstring>    // ����strerror����
#include <numeric>    // ����std::accumulate
#include <algorithm>  // ����std::min_element��std::max_element
#include <cmath>      // ����sqrt
#include <thread>     // ���ڶ��߳�
#include <atomic>     // ����ԭ�Ӳ���
#pragma warning(disable:4996) 

#pragma comment(lib, "ws2_32.lib")

std::vector<std::string> myStrings;
std::atomic<bool> running(true); // ���ڿ����߳�����

enum req {
    Seqno = 0,
    Ver = 2,
};

void handleUserInput(SOCKET clientSocket, sockaddr_in serverAddr, int serverAddrSize) {
    std::string input;
    while (running) {
        std::getline(std::cin, input);
        if (input == "exit") {// ���������"exit"�˳�
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

    // ��ʼ��Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // ����UDP�׽���
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "�׽��ִ���ʧ��" << std::endl;
        return 1;
    }

    // ���û�����IP��ַ�Ͷ˿ں�
    std::string ip;
    int port;
    std::cout << "�������������IP��ַ: ";
    std::cin >> ip;
    std::cout << "������������Ķ˿ں�: ";
    std::cin >> port;
    std::cin.ignore();  // ������з�

    // ��������ַ
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // ����SYN
    sendto(clientSocket, "SYN", strlen("SYN"), 0, (sockaddr*)&serverAddr, serverAddrSize);

    // ����SYN-ACK
    int recvLen = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverAddrSize);
    buffer[recvLen] = '\0';
    if (strcmp(buffer, "SYN-ACK") == 0) {
        sendto(clientSocket, "ACK", strlen("ACK"), 0, (sockaddr*)&serverAddr, serverAddrSize);
        std::cout << "�����Ѿ�����" << std::endl;

        // ����һ���߳��������û�����
        std::thread inputThread(handleUserInput, clientSocket, serverAddr, serverAddrSize);

        // ���ڼ���RTT��ʱ���
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
            std::cout << "������������к� " << Seqno + i << std::endl;
            ++sent_packets;

            // ��ʼ��ʱ
            start = std::clock();
            // �������ӳ٣���ģ�������ӳ�
            int delay = rand() % 1000; // 0��999������ӳ�
            Sleep(delay);

            timeval timeout;
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            recvLen = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverAddrSize);
            // �ں��ʵ�λ�û�ȡ��ǰʱ��
            std::time_t currentTime = std::time(nullptr);
            struct std::tm* timeInfo = std::localtime(&currentTime);
            char timeBuffer[9];
            std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeInfo);

            // ������ʱ
            end = std::clock();

            // ����RTT���Ժ���Ϊ��λ
            double rtt = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
            if (recvLen == SOCKET_ERROR) {
                // ��ʱ�����·�������
                for (int retry = 0; retry < 2 && running; ++retry) {
                    sendto(clientSocket, request.c_str(), request.length(), 0, (sockaddr*)&serverAddr, serverAddrSize);
                    //std::cout << "������������к�: " << Seqno + i << std::endl;
                    std::cout << Seqno + i << "  request time out " << std::endl;
                    ++sent_packets;
                    // ��ʼ��ʱ
                    start = std::clock();
                    // �������ӳ٣���ģ�������ӳ�
                    int delay = rand() % 1000; // 0��999������ӳ�
                    Sleep(delay);

                    timeout.tv_sec = 2;
                    timeout.tv_usec = 0;
                    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
                    recvLen = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverAddrSize);

                    // ������ʱ
                    end = std::clock();

                    // ����RTT���Ժ���Ϊ��λ
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
                std::cout << "��ǰʱ��: " << timeBuffer << std::endl;
                ++received_packets;
                if (first_response_time == 0) {
                    first_response_time = currentTime;
                }
                last_response_time = currentTime;
            }
            else {
                // �����ʱ��Ϣ
                std::cout << "�����ش�" << std::endl;
            }
        }

        // ����ͳ����Ϣ
        double max_rtt = *std::max_element(rtt_values.begin(), rtt_values.end());
        double min_rtt = *std::min_element(rtt_values.begin(), rtt_values.end());
        double avg_rtt = std::accumulate(rtt_values.begin(), rtt_values.end(), 0.0) / rtt_values.size();
        double sum_sq_diff = std::accumulate(rtt_values.begin(), rtt_values.end(), 0.0, [avg_rtt](double acc, double rtt) {
            return acc + (rtt - avg_rtt) * (rtt - avg_rtt);
            });
        double stddev_rtt = std::sqrt(sum_sq_diff / rtt_values.size());
        double packet_loss_rate = (1.0 - (double)received_packets / sent_packets) * 100.0;
        double total_response_time = std::difftime(last_response_time, first_response_time);

        // ��ӡͳ����Ϣ
        std::cout << "���յ���UDP packets��Ŀ: " << received_packets << std::endl;
        std::cout << "������: " << packet_loss_rate << "%" << std::endl;
        std::cout << "���RTT: " << max_rtt << " milliseconds" << std::endl;
        std::cout << "��СRTT: " << min_rtt << " milliseconds" << std::endl;
        std::cout << "ƽ��RTT: " << avg_rtt << " milliseconds" << std::endl;
        std::cout << "RTT�ı�׼��: " << stddev_rtt << " milliseconds" << std::endl;
        std::cout << "server��������Ӧʱ��: " << total_response_time << " seconds" << std::endl;
        std::cout << std::endl;

        // �ȴ������߳̽���
        inputThread.join();
    }

    // �ر��׽���
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
