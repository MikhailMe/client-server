#include "functions.h"

// вывод всех команд, которые можно использовать на клиенте
void client_help() {
    std::cout << "************************************************" << std::endl;
    std::cout << "*   Hello from server!                         *" << std::endl;
    std::cout << "*                                              *" << std::endl;
    std::cout << "*   You can write any messages to server       *" << std::endl;
    std::cout << "************************************************" << std::endl;
}

int main(int argc, char **argv) {
    int client_socket;
    sockaddr_in addr{};

    // задали сокет
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Can't create socket");
        return -1;
    }

    // задали адрес
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // установление соединения с сервером со стороны клиента
    if (connect(client_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Сonnection error");
        return -2;
    }
    std::cout << "Client started.. Send message to server" << std::endl;

    // вывод команд, доступных для клиента
    client_help();

    // номер запроса
    int request_client_number = 0;

    // отдельный поток на чтение сообщений с сервера
    pthread_t client_thread;
    pthread_create(&client_thread, nullptr, client_read_handler, (void *) (intptr_t) client_socket);

    // здесь пишем сообщух эхо-серверу
    while (true) {
        std::string message;
        // пишем, что отправляем клиенту
        std::getline(std::cin, message);
        // хотим отключится и отправляем "shutdown"
        if (strcmp(message.c_str(), EXIT) == 0) {
            send(client_socket, EXIT, BUFFER_SIZE, 0);
            break;
        }
        // отправляем message на сервер
        if (send(client_socket, message.c_str(), BUFFER_SIZE, 0) == -1) {
            perror("Send error");
            break;
        }
        std::cout << "Client's request#" << ++request_client_number << ": " << message << std::endl;
    }

    pthread_join(client_thread, nullptr);
    close(client_socket);
    std::cout << "Disconnected" << std::endl;
    std::cout << "Client closed" << std::endl;
    return 0;
}