#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <mutex>

#include "constants.h"

// читаем столько, сколько пришло
ssize_t readn(int socket, char *message, size_t length, int flags) {
    ssize_t received = 0;
    while (received < length) {
        char buffer[length];
        ssize_t size = recv(socket, buffer, length, flags);
        if (size <= 0) return size;
        for (int i = 0; i < size; i++)
            message[i + received] = buffer[i];
        received += size;
    }
    return received;
}

// выводим всех подключенных клиентов
void list() {
    if (clients.empty())
        std::cout << "List if clients is empty" << std::endl;
    else {
        std::cout << "List of clients: ";
        for (auto &&client : clients)
            std::cout << client.first << " ,";
    }
    std::cout << std::endl;
}

// корректный ввод дескриптора сокета
int enter() {
    int desc_sock;
    while (true) {
        std::cout << "Enter id of client: ";
        std::cin >> desc_sock;
        if (!std::cin) {
            std::cout << "Please enter valid id of client" << std::endl;
            std::cin.clear();
            while (std::cin.get() != '\n');
        } else if (clients.count(desc_sock) > 0) {
            break;
        }
    }
    return desc_sock;
}

// пишем клиенту по дескриптору сокета
void write(int desc_sock) {
    std::string message;
    std::cout << "Enter message to " << desc_sock << " client: ";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, message);
    send(desc_sock, message.c_str(), BUFFER_SIZE, 0);
    std::cout << "Your message sent" << std::endl;
}

// пишем сообщение определённому клиенту
void write() {
    int desc_sock = enter();
    if (clients.count(desc_sock) > 0)
        write(desc_sock);
    else std::cout << "Sorry, but this client doesn't exist" << std::endl;
}

// убиваем определённого клиента
void kill() {
    int desc_sock = enter();
    std::unique_lock<std::mutex> lock(client_mutex);
    if (client_states.count(desc_sock) > 0)
        client_states[desc_sock] = true;
    std::cout << "You kill " << desc_sock << " client";
}

// убиваем всех клиентов
void killall() {
    std::unique_lock<std::mutex> lock(client_mutex);
    for (auto &&client : clients)
        client_states[client.first] = true;
    std::cout << "All clients are disabled" << std::endl;
}

// вывод отосланной и принятой информации клиента
void printServer(int desc_sock, char request[], char response[], int &request_server_number) {
    std::cout << "Client's " << desc_sock << " request #" << request_server_number << ": " << request << std::endl;
    std::cout << "Server's response #" << request_server_number << ": " << response << "\n" << std::endl;
}

// хэндлер каждого клиента
void *connection_handler(void *ds) {
    // кастим к int, к нормальному виду дескриптора сокета
    auto &&desc_sock = (int) (intptr_t) ds;
    // номер запроса
    int request_server_number = 0;
    while (true) {
        char buffer[BUFFER_SIZE];
        ssize_t read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0 || strcmp(buffer, EXIT) == 0) {
            client_states[desc_sock] = true;
            break;
        }
        send(desc_sock, buffer, BUFFER_SIZE, 0);
        printServer(desc_sock, buffer, buffer, ++request_server_number);
    }
    close(desc_sock);
    pthread_exit(nullptr);
}

// если клиент сам отключился, то удаляем его из двух мап через 0,01 секунды
// работает всегда
void cleanup() {
    std::unique_lock<std::mutex> lock(client_mutex);
    auto &&it = std::begin(client_states);
    while (it != std::end(client_states)) {
        auto &&entry = *it;
        auto &&fd = entry.first;
        auto &&needClose = entry.second;
        if (needClose) {
            // закрываем сокета
            shutdown(fd, SHUT_RDWR);
            close(fd);
            // находим поток в основной паме по дескиптору сокета
            auto &&thread = clients.find(fd)->second;
            // джойним потоу
            pthread_join(thread, nullptr);
            std::cout << "\nClient " << fd << " disconnected" << std::endl;
            // удаляем из мапы состяний
            it = client_states.erase(it);
            // удаляем из основной мапы
            clients.erase(fd);
            continue;
        }
        ++it;
    }
}

// если произошла исключительная ситуация, закрываем сокет и пишем сообщение об ошибке
void closing(int server_socket, const char *closing_message) {
    perror(closing_message);
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
}

// хэдлер сервера
void *server_handler(void *serv_sock) {
    // кастим к int, к нормальному виду дескриптора сокета
    auto &&server_socket = (int) (intptr_t) serv_sock;
    int new_socket;
    while (true) {
        // задаёем таймаут
        timeval timeout{TIMEOUT_SEC, TIMEOUT_USEC};
        // задаём множество сокетов
        fd_set server_sockets{};
        FD_ZERO(&server_sockets); // NOLINT
        FD_SET(server_socket, &server_sockets);
        // выбираем сокет, у которого возник какой-то ивент
        auto &&result = select(server_socket + 1, &server_sockets, nullptr, nullptr, &timeout);
        cleanup();
        if (result < 0) {
            closing(server_socket, "Select failed");
            break;
        }
        if (result == 0 || !FD_ISSET(server_socket, &server_sockets))
            continue;
        // асептим новый сокет
        new_socket = accept(server_socket, nullptr, nullptr);
        if (new_socket < 0) {
            closing(server_socket, "Accept failed");
            break;
        }
        std::cout << "\nConnection accepted" << std::endl;
        std::cout << "In list we have: " << clients.size() + 1 << "\n" << std::endl;
        // так как new_socket пока не нужно убивать ставим false
        {
            std::unique_lock<std::mutex> lock(client_mutex);
            client_states[new_socket] = false;
        }
        // добавляем клиентов в мапу и даём каждому поток
        {
            std::unique_lock<std::mutex> lock(mutex);
            pthread_t clients_thread;
            if (pthread_create(&clients_thread, nullptr, connection_handler, (void *) (intptr_t) new_socket) == 0)
                clients.emplace(new_socket, clients_thread);
            else client_states[new_socket] = true;
        }
    }
    pthread_exit(nullptr);
}

// хэндлер клиента для чтения в отдельном потоке
void *client_read_handler(void *ds) {
    // кастим к int, к нормальному виду дескриптора сокета
    auto &&desc_sock = (int) (intptr_t) ds;
    // номер запроса
    int request_client_number = 0;
    char buffer[BUFFER_SIZE + 1];
    while (true) {
        bzero(buffer, BUFFER_SIZE + 1);
        // слушаем сервер
        if (readn(desc_sock, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("Readn failed");
            close(desc_sock);
            break;
        }
        // если сервер прислал "shutdown", то умираем
        if (strcmp(buffer, EXIT) == 0)
            break;
        std::cout << "Server's response#" << ++request_client_number << ": " << buffer << std::endl;
    }
    close(desc_sock);
    pthread_exit(nullptr);
}
