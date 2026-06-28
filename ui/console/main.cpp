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

// CALLBACK для входящих сообщений (вызывается из фонового потока Hivemind)
void onMessageReceived(const char* sender, const char* message) {
    std::string msg(message);
    std::string snd(sender);
    
    // Системные сообщения не печатаем как обычные
    if (msg.find("[SYSTEM]") == 0) {
        return; // processIncomingPacket уже напечатал всё нужное
    }
    
    std::cout << "\n\x1b[32m[Received from " << snd << "]:\x1b[0m " << msg << std::endl;
    std::cout << "> " << std::flush;
}

void printHelp() {
    std::cout << "\n\x1b[33mCommands:\x1b[0m" << std::endl;
    std::cout << "  /beacon <IP> <PORT>      - Set beacon server address" << std::endl;
    std::cout << "  /pack                    - Request users.pack from beacon" << std::endl;
    std::cout << "  /users                   - Show all users in network" << std::endl;
    std::cout << "  /reg <name>              - Register your name (then /pack to verify)" << std::endl;
    std::cout << "  /msg <name> <message>    - Send message via relay" << std::endl;
    std::cout << "  /send <IP> <PORT> <msg>  - Send direct message by IP" << std::endl;
    std::cout << "  /find <name>             - Find user by name" << std::endl;
    std::cout << "  /myip                    - Show your public IP" << std::endl;
    std::cout << "  /nodeid                  - Show your Node ID" << std::endl;
    std::cout << "  /status                  - Show your status" << std::endl;
    std::cout << "  /sync                    - Sync with network" << std::endl;
    std::cout << "  /help                    - Show this help" << std::endl;
    std::cout << "  /quit                    - Exit" << std::endl;
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "\n\x1b[36m╔══════════════════════════════════════════════════════════╗\x1b[0m" << std::endl;
    std::cout << "\x1b[36m║   Hivemind P2P Messenger v0.6 (Beacon Relay Mode)        ║\x1b[0m" << std::endl;
    std::cout << "\x1b[36m║   STUN → Beacon Server → users.pack → Relay              ║\x1b[0m" << std::endl;
    std::cout << "\x1b[36m║   Type /beacon <IP> <PORT> to connect to beacon     ║\x1b[0m" << std::endl;
    std::cout << "\x1b[36m╚══════════════════════════════════════════════════════════╝\x1b[0m" << std::endl;
    std::cout << std::endl;
    
    g_hivemind = hivemind_create();
    if (!g_hivemind) {
        std::cerr << "Failed to create Hivemind" << std::endl;
        return 1;
    }
    
    // Устанавливаем callback ДО старта
    hivemind_set_message_callback(g_hivemind, onMessageReceived);
    
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
        std::cout << "  Public IP: \x1b[36m" << ip << "\x1b[0m" << std::endl;
    }
    if (hivemind_get_local_ip(g_hivemind, ip, sizeof(ip))) {
        std::cout << "  Local IP: \x1b[36m" << ip << "\x1b[0m" << std::endl;
    }
    std::cout << "  Node ID: \x1b[36m" << hivemind_get_node_id(g_hivemind) << "\x1b[0m" << std::endl;
    
    std::cout << std::endl;
    
    // НЕТ БОЛЬШЕ СВОЕГО receiveLoop! Все сообщения приходят через callback.
    
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
        else if (input.substr(0, 7) == "/beacon") {
            std::string rest = input.substr(8);
            size_t space = rest.find(' ');
            if (space == std::string::npos) {
                std::cout << "Usage: /beacon <IP> <PORT>" << std::endl;
                continue;
            }
            std::string beaconIp = rest.substr(0, space);
            uint16_t beaconPort = static_cast<uint16_t>(std::stoi(rest.substr(space + 1)));
            
            hivemind_set_beacon(g_hivemind, beaconIp.c_str(), beaconPort);
            std::cout << "\x1b[32m✓ Beacon set to " << beaconIp << ":" << beaconPort << "\x1b[0m" << std::endl;
            
            std::cout << "Requesting users.pack..." << std::endl;
            hivemind_request_users_pack(g_hivemind);
        }
        else if (input == "/pack") {
            if (!hivemind_is_beacon_configured(g_hivemind)) {
                std::cout << "\x1b[31m✗ Beacon not configured. Use /beacon <IP> <PORT> first\x1b[0m" << std::endl;
                continue;
            }
            std::cout << "Requesting users.pack from beacon..." << std::endl;
            if (hivemind_request_users_pack(g_hivemind)) {
                std::cout << "\x1b[32m✓ Request sent\x1b[0m" << std::endl;
            } else {
                std::cout << "\x1b[31m✗ Failed to send request\x1b[0m" << std::endl;
            }
        }
        else if (input == "/users") {
            int count = hivemind_get_network_users_count(g_hivemind);
            if (count == 0) {
                std::cout << "\x1b[33mNo users in network. Request /pack first.\x1b[0m" << std::endl;
                continue;
            }
            
            std::cout << "\n\x1b[33mNetwork Users (" << count << "):\x1b[0m" << std::endl;
            for (int i = 0; i < count; ++i) {
                char name[64], nodeId[64], userIp[64];
                uint16_t userPort;
                if (hivemind_get_network_user(g_hivemind, i, name, nodeId, userIp, &userPort)) {
                    bool isMe = (std::string(nodeId) == std::string(hivemind_get_node_id(g_hivemind)));
                    std::cout << "  " << (isMe ? "\x1b[32m[YOU]\x1b[0m " : "      ");
                    std::cout << name << " (" << std::string(nodeId).substr(0, 8) << "...) -> " 
                              << userIp << ":" << userPort << std::endl;
                }
            }
            std::cout << std::endl;
        }
        else if (input.substr(0, 4) == "/reg") {
            std::string newName = input.substr(5);
            if (!newName.empty()) {
                if (hivemind_register_name(g_hivemind, newName.c_str())) {
                    std::cout << "\x1b[32m✓ Local registration: " << newName << "\x1b[0m" << std::endl;
                    if (hivemind_is_beacon_configured(g_hivemind)) {
                        std::cout << "\x1b[33m  Sent to beacon. Wait 1-2 sec, then run /pack to verify.\x1b[0m" << std::endl;
                    }
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
            
            if (hivemind_send_via_relay(g_hivemind, targetName.c_str(), message.c_str())) {
                std::cout << "\x1b[32m✓ Sent via relay to " << targetName << "\x1b[0m" << std::endl;
            } else {
                std::cout << "\x1b[31m✗ Failed to send via relay\x1b[0m" << std::endl;
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
                std::cout << "\x1b[32m✓ Sent directly\x1b[0m" << std::endl;
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
                std::cout << "\x1b[33mUser not found locally: " << targetName << "\x1b[0m" << std::endl;
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
        else if (input == "/nodeid") {
            std::cout << "\x1b[32mNode ID: \x1b[36m" << hivemind_get_node_id(g_hivemind) << "\x1b[0m" << std::endl;
        }
        else if (input == "/status") {
            std::cout << "\n\x1b[36m=== Status ===\x1b[0m" << std::endl;
            std::cout << "  Node ID: " << hivemind_get_node_id(g_hivemind) << std::endl;
            std::cout << "  Beacon: " << (hivemind_is_beacon_configured(g_hivemind) ? "YES" : "NO") << std::endl;
            std::cout << "  In users.pack: " << (hivemind_is_in_users_pack(g_hivemind) ? "YES" : "NO") << std::endl;
            std::cout << "  Network users: " << hivemind_get_network_users_count(g_hivemind) << std::endl;
            std::cout << "\x1b[36m==============\x1b[0m" << std::endl;
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
    
    std::cout << "\n\x1b[36mGoodbye!\x1b[0m" << std::endl;
    return 0;
}