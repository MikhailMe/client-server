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
const size_t PORT = 19000;

std::mutex mutex;
std::unordered_map<int, std::thread> clients;

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
        std::cout << "List of clients: " << std::endl;
        for (auto &&client : clients)
            std::cout << client.first << std::endl;
    }
}

// корректный ввод дескриптора сокета
int enter(int desc_sock) {
    while (true) {
        std::cout << "Enter id of client: ";
        std::cin >> desc_sock;
        if (!std::cin) {
            std::cout << "Please enter valid id of client" << std::endl;
            std::cin.clear();
            while (std::cin.get() != '\n');
        }
        else break;
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
    int desc_sock = enter(desc_sock);
    if (clients.count(desc_sock) > 0)
        write(desc_sock);
    else std::cout << "Sorry, but this client doesn't exist" << std::endl;
}

// убиваем клиента по дескриптору сокета
void kill(int desc_sock) {
    if (clients.count(desc_sock) > 0) {
        std::string string = "shutdown";
        send(desc_sock, string.c_str(), BUFFER_SIZE, 0);
        shutdown(desc_sock, SHUT_RDWR);
        close(desc_sock);
        auto &&thread = clients.find(desc_sock)->second;
        thread.join();
    } else std::cout << "Sorry, but this client doesn't exist" << std::endl;
}

// убиваем определённого клиента
void kill() {
    int desc_sock = enter(desc_sock);
    kill(desc_sock);
    clients.erase(desc_sock);
    std::cout << "You kill " << desc_sock << " client" << std::endl;
}

// убиваем всех клиентов
void killall() {
    for (auto &&client : clients)
        kill(client.first);
    clients.clear();
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
        if (read_size <= 0) {
            break;
        }
        if (strcmp(buffer, EXIT) == 0) {
            std::unique_lock<std::mutex> lock(mutex);
            kill(desc_sock);
            std::cout << "\nClient " << desc_sock << " disconnected" << std::endl;
        } else {
            if (read_size == 0) {
                perror("Client disconnected\n");
                break;
            } else if (read_size == -1) {
                perror("Readn failed\n");
                break;
            }
            send(desc_sock, buffer, BUFFER_SIZE, 0);
            printServer(desc_sock, buffer, buffer);
        }
    }
    close(desc_sock);
}

// хэдлер сервера
void server_handler(int server_socket) {
    int new_socket;
    while (true) {
        new_socket = accept(server_socket, nullptr, nullptr);
        if (new_socket < 0) {
            perror("Accept failed\n");
            shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
            break;
        }
        std::cout << "\nConnection accepted" << std::endl;
        std::cout << "In list we have: " << clients.size() + 1 << std::endl;
        std::flush(std::cout);
        // добавляем клиентов в мапу и даём каждому поток
        {
            std::unique_lock<std::mutex> lock(mutex);
            clients.emplace(new_socket, [new_socket]() -> void {
                connection_handler(new_socket);
            });
        }
    }
    // сюда попадаем, когда вылетел accept => был вызван shutdown сервера
    // поэтому необходимо закрыть все сокеты для клиентов
    killall();
}

// хэндлер клиента для чтения в отдельном потоке
void client_read_handler(int desc_sock) {
    while (true) {
        char buffer[BUFFER_SIZE + 1];
        // слушаем сервер
        if (readn(desc_sock, buffer, BUFFER_SIZE, 0) == -1) {
            perror("Readn error");
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