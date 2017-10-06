#include "functions.h"

int main() {
    // дескрипторы сокетов
    int server_socket, new_socket;
    struct sockaddr_in addr{};

    // создали сокет, который будет слушать
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket\n");
        return -1;
    }

    // задали адрес и порт
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // привязали сокет к адресу
    if (bind(server_socket, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("Bind failed\n");
        return -2;
    }

    // server ждёт запросов со стороны клиентов
    if (listen(server_socket, 3) != 0) {
        close(server_socket);
        perror("Listen error\n");
        return -3;
    }

    std::cout << "Server started on port " << PORT << std::endl;
    std::cout << "Waiting for incoming connections..." << std::endl;

    // вывод команд, доступных для сервера
    help();

    // отдельный поток для сервера
    std::thread server_thread([server_socket]() -> void {
        server_handler(server_socket);
    });

    std::string command;

    while (true) {
        std::getline(std::cin, command);
        if (command == "list") list();
        else if (command == "write") write();
        else if (command == "kill") kill();
        else if (command == "killall") killall();
        else if (command == "shutdown") {
            shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
            break;
        }
    }
    if (server_thread.joinable())
        server_thread.join();
    std::cout << "Goodbye" << std::endl;
    return 0;
}