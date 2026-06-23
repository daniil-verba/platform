// Platform/console/main.cpp
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <signal.h>
#include "../../libs/hivemind/hivemind.h"

std::atomic<bool> running(true);
Hivemind* g_hivemind = nullptr;

void signalHandler(int sig) {
    (void)sig;
    std::cout << "\nCaught signal, exiting..." << std::endl;
    running = false;
}

void receiveLoop() {
    char sender_ip[16];
    uint16_t sender_port;
    char message[4096];
    
    while (running) {
        if (hivemind_receive(g_hivemind, sender_ip, &sender_port, message, sizeof(message))) {
            std::string msg(message);
            
            // Обработка системных сообщений
            if (msg.find("PRESENCE:") == 0) {
                // Формат: PRESENCE:NAME:IP:PORT
                std::string rest = msg.substr(9);
                size_t pos1 = rest.find(':');
                if (pos1 != std::string::npos) {
                    std::string name = rest.substr(0, pos1);
                    rest = rest.substr(pos1 + 1);
                    size_t pos2 = rest.find(':');
                    if (pos2 != std::string::npos) {
                        std::string ip = rest.substr(0, pos2);
                        uint16_t port = static_cast<uint16_t>(std::stoi(rest.substr(pos2 + 1)));
                        
                        // Автоматически добавляем пользователя
                        hivemind_register_name(g_hivemind, name.c_str());  // Обновит свой реестр
                        std::cout << "\n\x1b[36m[System] " << name << " is online (" << ip << ":" << port << ")\x1b[0m" << std::endl;
                        std::cout << "> " << std::flush;
                        continue;
                    }
                }
            }
            
            std::cout << "\n\x1b[32m[Received from " << sender_ip << ":" << sender_port << "]:\x1b[0m " 
                      << msg << std::endl;
            std::cout << "> " << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void printHelp() {
    std::cout << "\n\x1b[33mCommands:\x1b[0m" << std::endl;
    std::cout << "  /reg <name>              - Register your name" << std::endl;
    std::cout << "  /msg <name> <message>         - Send message by name" << std::endl;
    std::cout << "  /send <IP> <PORT> <message>   - Send message by IP" << std::endl;
    std::cout << "  /find <name>                  - Find user by name" << std::endl;
    std::cout << "  /myip                         - Show your public IP" << std::endl;
    std::cout << "  /users                        - Show all known users" << std::endl;
    std::cout << "  /sync                         - Sync with network" << std::endl;
    std::cout << "  /help                         - Show this help" << std::endl;
    std::cout << "  /quit                         - Exit" << std::endl;
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "\n\x1b[36m╔══════════════════════════════════════════════════════════╗\x1b[0m" << std::endl;
    std::cout << "\x1b[36m║   Platform P2P Messenger v0.5 (STUN only)                ║\x1b[0m" << std::endl;
    std::cout << "\x1b[36m║   Russian STUN → Google STUN → Local IP                  ║\x1b[0m" << std::endl;
    std::cout << "\x1b[36m║   Type /reg <name> to join the network              ║\x1b[0m" << std::endl;
    std::cout << "\x1b[36m╚══════════════════════════════════════════════════════════╝\x1b[0m" << std::endl;
    std::cout << std::endl;
    
    g_hivemind = hivemind_create();
    if (!g_hivemind) {
        std::cerr << "Failed to create Hivemind" << std::endl;
        return 1;
    }
    
    hivemind_enable_hole_punching(g_hivemind, 1);
    
    std::cout << "Enter your port (e.g., 9999): ";
    int port;
    std::cin >> port;
    std::cin.ignore();
    
    if (!hivemind_start(g_hivemind, port)) {
        std::cerr << "Failed to start" << std::endl;
        return 1;
    }
    
    std::cout << "\n\x1b[32m✓ Started on port " << port << "\x1b[0m" << std::endl;
    
    char ip[64];
    if (hivemind_get_public_ip(g_hivemind, ip, sizeof(ip))) {
        std::cout << "  Your public IP: \x1b[36m" << ip << "\x1b[0m" << std::endl;
    }
    
    hivemind_sync_with_network(g_hivemind);
    hivemind_start_auto_sync(g_hivemind, 600);
    
    printHelp();
    std::cout << std::endl;
    
    std::thread receiver(receiveLoop);
    
    std::string input;
    while (running) {
        std::cout << "\x1b[36m>\x1b[0m ";
        std::getline(std::cin, input);
        
        if (input == "/quit" || input == "/q") {
            running = false;
            break;
        }
        else if (input == "/help" || input == "/h") {
            printHelp();
        }
        else if (input.substr(0, 4) == "/reg") {
            std::string newName = input.substr(5);
            if (!newName.empty()) {
                if (hivemind_register_name(g_hivemind, newName.c_str())) {
                    std::cout << "\x1b[32m✓ Registered as: " << newName << "\x1b[0m" << std::endl;
                } else {
                    std::cout << "\x1b[31m✗ Failed to register\x1b[0m" << std::endl;
                }
            } else {
                std::cout << "Usage: /reg <name>" << std::endl;
            }
        }
        else if (input.substr(0, 4) == "/msg") {
            std::string rest = input.substr(5);
            size_t space = rest.find(' ');
            if (space == std::string::npos) {
                std::cout << "Usage: /msg <name> <message>" << std::endl;
                continue;
            }
            std::string targetName = rest.substr(0, space);
            std::string message = rest.substr(space + 1);
            
            if (hivemind_send_to_user(g_hivemind, targetName.c_str(), message.c_str())) {
                std::cout << "\x1b[32m✓ Sent to " << targetName << "\x1b[0m" << std::endl;
            } else {
                std::cout << "\x1b[31m✗ User not found: " << targetName << "\x1b[0m" << std::endl;
            }
        }
        else if (input.substr(0, 5) == "/send") {
            std::string rest = input.substr(6);
            size_t first = rest.find(' ');
            size_t second = rest.find(' ', first + 1);
            if (first == std::string::npos || second == std::string::npos) {
                std::cout << "Usage: /send <IP> <PORT> <message>" << std::endl;
                continue;
            }
            std::string targetIp = rest.substr(0, first);
            std::string portStr = rest.substr(first + 1, second - first - 1);
            std::string message = rest.substr(second + 1);
            
            uint16_t targetPort = static_cast<uint16_t>(std::stoi(portStr));
            if (hivemind_send_to_ip(g_hivemind, targetIp.c_str(), targetPort, message.c_str())) {
                std::cout << "\x1b[32m✓ Sent\x1b[0m" << std::endl;
            } else {
                std::cout << "\x1b[31m✗ Failed to send\x1b[0m" << std::endl;
            }
        }
        else if (input.substr(0, 5) == "/find") {
            std::string targetName = input.substr(6);
            char foundIp[64];
            uint16_t foundPort;
            if (hivemind_find_user(g_hivemind, targetName.c_str(), foundIp, &foundPort)) {
                std::cout << "\x1b[32mFound: " << targetName << " -> " << foundIp << ":" << foundPort << "\x1b[0m" << std::endl;
            } else {
                std::cout << "\x1b[33mUser not found: " << targetName << "\x1b[0m" << std::endl;
            }
        }
        else if (input == "/myip") {
            char myIp[64];
            if (hivemind_get_public_ip(g_hivemind, myIp, sizeof(myIp))) {
                std::cout << "\x1b[32mPublic IP: \x1b[36m" << myIp << "\x1b[0m" << std::endl;
            } else {
                std::cout << "\x1b[33mCould not determine public IP\x1b[0m" << std::endl;
            }
        }
        else if (input == "/users") {
            // Показываем всех известных пользователей (заглушка — позже реализуем)
            std::cout << "\x1b[33mUse /find <name> to search for specific user\x1b[0m" << std::endl;
        }
        else if (input == "/sync") {
            hivemind_sync_with_network(g_hivemind);
        }
        else if (!input.empty()) {
            std::cout << "\x1b[33mUnknown command. Type /help\x1b[0m" << std::endl;
        }
    }
    
    hivemind_stop(g_hivemind);
    hivemind_destroy(g_hivemind);
    receiver.join();
    
    std::cout << "\n\x1b[36mGoodbye!\x1b[0m" << std::endl;
    return 0;
}