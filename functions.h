#pragma once

#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>

#define EXIT "shutdown"

const size_t BUFFER_SIZE = 512;
const size_t PORT = 32000;
const int TIMEOUT_SEC = 0;
const int TIMEOUT_USEC = 100;

std::mutex mutex;
std::unordered_map<int, std::thread> clients;
std::mutex client_mutex;
std::unordered_map<int, bool> client_states;

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
            std::cout << "Please enter valid id of client" << std::endl;
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

// убиваем клиента по дескриптору сокета
void kill(int desc_sock) {
    if (clients.count(desc_sock) > 0) {
        shutdown(desc_sock, SHUT_RDWR);
        close(desc_sock);
        auto &&thread = clients.find(desc_sock)->second;
        if (thread.joinable())
            thread.join();
        std::cout << "\nClient " << desc_sock << " disconnected" << std::endl;
    } else std::cout << "Sorry, but this client doesn't exist" << std::endl;
}

void lazy_kill(int desc_sock) {
    if (client_states.count(desc_sock) > 0)
        client_states[desc_sock] = true;
}

// убиваем определённого клиента
void kill() {
    int desc_sock = enter();
    std::unique_lock<std::mutex> lock(client_mutex);
    lazy_kill(desc_sock);
    std::cout << "You kill " << desc_sock << " client";
}

// убиваем всех клиентов
void killall() {
    std::unique_lock<std::mutex> lock(client_mutex);
    for (auto &&client : clients)
        lazy_kill(client.first);
    std::cout << "All clients are disabled" << std::endl;
}

// вывод всех команд, которые можно использовать на сервере
void help() {
    std::cout << "You can use the following commands:" << std::endl;
    std::cout << "list     -  show list of all clients" << std::endl;
    std::cout << "write    -  send message to certain client" << std::endl;
    std::cout << "kill     -  remove certain client" << std::endl;
    std::cout << "killall  -  remove all clients" << std::endl;
    std::cout << "shutdown -  server shutdown" << std::endl;
}

// вывод отосланной и принятой информации клиента
void printServer(int desc_sock, char request[], char response[]) {
    std::cout << "Client's " << desc_sock << " request : " << request << std::endl;
    std::cout << "Server's response: " << response << std::endl;
}

// хэндлер каждого клиента
void connection_handler(int desc_sock) {
    while (true) {
        char buffer[BUFFER_SIZE];
        ssize_t read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0 || strcmp(buffer, EXIT) == 0)
            break;
        send(desc_sock, buffer, BUFFER_SIZE, 0);
        printServer(desc_sock, buffer, buffer);
    }
    close(desc_sock);
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
            kill(fd);
            it = client_states.erase(it);
            clients.erase(fd);
            continue;
        }
        ++it;
    }
}

void closing(int server_socket, const char *closing_message) {
    perror(closing_message);
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
}

// хэдлер сервера
void server_handler(int server_socket) {
    int new_socket;
    while (true) {
        // задаёем таймаут
        timeval timeout{TIMEOUT_SEC, TIMEOUT_USEC};
        // задаём множество сокетов
        fd_set server_sockets{};
        FD_ZERO(&server_sockets);
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
        std::cout << "In list we have: " << clients.size() + 1 << std::endl;
        // так как new_socket пока не нужно убивать ставим false
        {
            std::unique_lock<std::mutex> lock(client_mutex);
            client_states[new_socket] = false;
        }
        // добавляем клиентов в мапу и даём каждому поток
        {
            std::unique_lock<std::mutex> lock(mutex);
            clients.emplace(new_socket, [new_socket]() -> void {
                try {
                    connection_handler(new_socket);
                } catch (...) {}
                // необходимо убить new_socket, поэтому ставим true
                client_states[new_socket] = true;
            });
        }
    }
}

// хэндлер клиента для чтения в отдельном потоке
void client_read_handler(int desc_sock) {
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
        std::cout << "Server's response: " << buffer << std::endl;
    }
    close(desc_sock);
}
