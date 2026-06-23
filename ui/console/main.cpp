#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include "../libs/hivemind/hivemind.h"

std::atomic<bool> running(true);
Hivemind* g_hivemind = nullptr;

void receiveLoop() {
    char sender_ip[16];
    uint16_t sender_port;
    char message[4096];
    
    while (running) {
        if (hivemind_receive(g_hivemind, sender_ip, &sender_port, message, sizeof(message))) {
            std::cout << "\n\x1b[32m[Received from " << sender_ip << ":" << sender_port << "]:\x1b[0m " 
                      << message << std::endl;
            std::cout << "> " << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    std::cout << "\n\x1b[36m=== Platform Messenger ===\x1b[0m" << std::endl;
    std::cout << "Simple peer-to-peer messenger using Hivemind" << std::endl;
    std::cout << std::endl;
    
    // Создаём экземпляр
    g_hivemind = hivemind_create();
    if (!g_hivemind) {
        std::cerr << "Failed to create Hivemind instance" << std::endl;
        return 1;
    }
    
    // Запрашиваем порт
    int port;
    std::cout << "Enter your listening port (e.g., 9999): ";
    std::cin >> port;
    
    // Запускаем
    if (!hivemind_start(g_hivemind, port)) {
        std::cerr << "Failed to start on port " << port << std::endl;
        hivemind_destroy(g_hivemind);
        return 1;
    }
    
    std::cout << "\n\x1b[32m✓ Hivemind started on port " << port << "\x1b[0m" << std::endl;
    std::cout << "\n\x1b[33mCommands:\x1b[0m" << std::endl;
    std::cout << "  /send <IP> <PORT> <message>  - Send message to peer" << std::endl;
    std::cout << "  /quit                        - Exit messenger" << std::endl;
    std::cout << "\n\x1b[36mExample:\x1b[0m /send 127.0.0.1 9999 Hello!" << std::endl;
    std::cout << std::endl;
    
    // Запускаем поток приёма
    std::thread receiver(receiveLoop);
    std::cin.ignore();
    
    // Главный цикл
    std::string input;
    while (running) {
        std::cout << "\x1b[36m>\x1b[0m ";
        std::getline(std::cin, input);
        
        if (input == "/quit") {
            running = false;
            break;
        }
        else if (input.substr(0, 5) == "/send") {
            // Парсим команду: /send 192.168.1.100 9999 Hello world
            std::string rest = input.substr(6);
            
            size_t first_space = rest.find(' ');
            if (first_space == std::string::npos) {
                std::cout << "\x1b[31mUsage: /send <IP> <PORT> <message>\x1b[0m" << std::endl;
                continue;
            }
            
            std::string ip = rest.substr(0, first_space);
            rest = rest.substr(first_space + 1);
            
            size_t second_space = rest.find(' ');
            if (second_space == std::string::npos) {
                std::cout << "\x1b[31mUsage: /send <IP> <PORT> <message>\x1b[0m" << std::endl;
                continue;
            }
            
            std::string port_str = rest.substr(0, second_space);
            std::string message = rest.substr(second_space + 1);
            
            try {
                uint16_t target_port = static_cast<uint16_t>(std::stoi(port_str));
                
                if (hivemind_send_to_ip(g_hivemind, ip.c_str(), target_port, message.c_str())) {
                    std::cout << "\x1b[32m✓ Sent to " << ip << ":" << target_port << "\x1b[0m" << std::endl;
                } else {
                    std::cout << "\x1b[31m✗ Failed to send\x1b[0m" << std::endl;
                }
            } catch (...) {
                std::cout << "\x1b[31mInvalid port number\x1b[0m" << std::endl;
            }
        }
        else if (!input.empty()) {
            std::cout << "\x1b[33mUnknown command. Use /send or /quit\x1b[0m" << std::endl;
        }
    }
    
    // Очистка
    hivemind_stop(g_hivemind);
    hivemind_destroy(g_hivemind);
    receiver.join();
    
    std::cout << "\n\x1b[36mGoodbye!\x1b[0m" << std::endl;
    return 0;
}
