#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

void connectToServer(int thread_id) {
    const char* server_ip = "127.0.0.1";  // 替换为实际 IP 地址
    const int server_port = 8033;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Thread " << thread_id << ": Failed to create socket\n";
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "Thread " << thread_id << ": Invalid address/ Address not supported\n";
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Thread " << thread_id << ": Connection failed\n";
        close(sock);
        return;
    }

    std::cout << "Thread " << thread_id << ": Connected to server successfully\n";

    close(sock);
}

int main() {
    const int num_threads = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(connectToServer, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "All threads completed\n";
    return 0;
}
