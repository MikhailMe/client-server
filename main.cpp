#include <iostream>
#include <cstring>
#include <thread>

int main() {
    int x = 5;
    std::thread t1;
    t1.join();
    bool flag = t1.joinable();
    std::cout << flag << std::endl;
    return 0;
}


/*

 #include "functions.h"

void client_read_handler(int desc_sock) {
    while (true) {
        char buffer[BUFFER_SIZE + 1];
        // слушаем сервер
        if (readn(desc_sock, buffer, BUFFER_SIZE, 0) == -1) {
            perror("Receive error");
            break;
        }
        // если сервер нас отключил
        if (strcmp(buffer, EXIT) == 0)
            break;
        std::cout << "Server's response: " << buffer << std::endl;
    }
    close(desc_sock);
}

int main(int argc, char **argv) {
    int sock;
    sockaddr_in addr{};

    // задали сокет
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Can't create socket");
        return -1;
    }

    // задали адрес
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // установление соединения с сервером со стороны клиента
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Сonnection error");
        return -2;
    }
    std::cout << "Client started.. Send message to server" << std::endl;
    std::string message;
    while (true) {
        char buffer[BUFFER_SIZE + 1];

        std::thread client_read([sock]() -> void {
            client_read_handler(sock);
        });

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        // пишем, что отправляем клиенту
        std::getline(std::cin, message);
        // хотим отключится и отправляем "shutdown"
        if (strcmp(message.c_str(), EXIT) == 0) {
            send(sock, message.c_str(), BUFFER_SIZE, 0);
            break;
        }
        // отправляем message на сервер
        if (send(sock, message.c_str(), BUFFER_SIZE, 0) == -1) {
            perror("Send error");
            break;
        }


        // печатаем переденаую и полученную инфу серверу
        //printClient(message, buffer);
    }
    std::cout << "Disconnected" << std::endl;
    std::cout << "Client closed" << std::endl;
    close(sock);
    return 0;
}

 */