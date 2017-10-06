#include "functions.h"

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

    // отдельный поток на чтение сообщений с сервера
    std::thread client_read([sock]() -> void {
        client_read_handler(sock);
    });

    while (true) {

        // пишем, что отправляем клиенту
        std::getline(std::cin, message);

        // хотим отключится и отправляем "shutdown"
        if (strcmp(message.c_str(), EXIT) == 0) {
            send(sock, EXIT, BUFFER_SIZE, 0);
            break;
        }

        // отправляем message на сервер
        if (send(sock, message.c_str(), BUFFER_SIZE, 0) == -1) {
            perror("Send error");
            break;
        }

        std::cout << "Client's request : " << message << std::endl;
    }
    if (client_read.joinable())
        client_read.join();

    close(sock);
    std::cout << "Disconnected" << std::endl;
    std::cout << "Client closed" << std::endl;
    return 0;
}