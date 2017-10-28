#pragma once

#define EXIT      "shutdown"

const size_t BUFFER_SIZE = 512;
const size_t PORT = 7777;
const int TIMEOUT_SEC = 0;
const int TIMEOUT_USEC = 100;

std::mutex mutex;
std::mutex client_mutex;
std::unordered_map<int, pthread_t> clients;
std::unordered_map<int, bool> client_states;