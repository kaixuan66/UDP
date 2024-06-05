#include <winsock2.h>
#include <iostream>
#include <cstdlib>    // ����rand������srand����
#include <ctime>      // ����time����
#include <string>
#include <sstream>    // ���sstreamͷ�ļ��������ַ���������
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    char buffer[1024];
    int clientAddrSize = sizeof(clientAddr);

    // ��ʼ��Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // ����UDP�׽���
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "�׽��ִ���ʧ��" << std::endl;
        return 1;
    }

    // ���׽���
    serverAddr.sin_family = AF_INET;// ���÷�������ַ��ΪIPv4
    serverAddr.sin_port = htons(11111);
    serverAddr.sin_addr.s_addr = INADDR_ANY;// ���÷�������ַΪ�����ַ
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "UDP���������ڼ���" << std::endl;

    // �ͻ��˼��ϣ�ʹ��һ��unordered_map���洢ÿ���ͻ��˵�״̬
    std::unordered_map<std::string, bool> clients;

    while (true) {
        // �������Կͻ��˵�����
        int recvLen = recvfrom(serverSocket, buffer, 1024, 0, (sockaddr*)&clientAddr, &clientAddrSize);
        if (recvLen == SOCKET_ERROR) {
            std::cerr << "��������ʧ��" << std::endl;
            continue;
        }
        buffer[recvLen] = '\0';
        std::string clientKey = std::string(inet_ntoa(clientAddr.sin_addr)) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        std::cout << "��Ϣ���Կͻ��� " << clientKey << std::endl;

        if (strcmp(buffer, "SYN") == 0) {
            sendto(serverSocket, "SYN-ACK", strlen("SYN-ACK"), 0, (sockaddr*)&clientAddr, clientAddrSize);
            clients[clientKey] = true;
        }
        else if (strcmp(buffer, "ACK") == 0) {
            std::cout << "������ϵ " << clientKey << std::endl;
        }
        else if (strcmp(buffer, "FIN-ACK") == 0) {
            std::cout << "���ӹر�" << clientKey << std::endl;
            clients.erase(clientKey);
        }
        else if (clients.find(clientKey) != clients.end()) { // ����ͻ����Ѵ����ڼ�����
            std::string sbuffer(buffer);
            std::istringstream iss(sbuffer.substr(sbuffer.find_first_of("0123456789") + 1)); // ȥ����һ������

            // ���ʣ�������ֵ
            int num;
            while (iss >> num) {
                std::cout << "���к� " << num << std::endl;
               
            }
            if (strstr(buffer, "Request")) {
                int i = rand();
                if (i % 4 != 0) { // �����ĸ���Ϊ20%
                    // �������ӳ٣���ģ�������ӳ�
                    //int delay = rand() % 1000; // 0��999������ӳ�
                    sendto(serverSocket, buffer, recvLen, 0, (sockaddr*)&clientAddr, clientAddrSize);
                }
            }
        }
    }

    // �ر��׽���
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
