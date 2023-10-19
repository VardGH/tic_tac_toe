#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

int client_socket;
const int PORT = 12364;
const int BUFFER_SIZE = 1024;

void connect_to_server() 
{
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Error creating server socket\n";
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connect to server" << std::endl;
        return EXIT_FAILURE;
    }
}

void receive_message(char* buffer, size_t bufferSize) 
{
    int valread = read(client_socket, buffer, bufferSize);
    if (valread == -1) {
        std::cerr << "Error reading from server" << std::endl;
        close(client_socket);
        return EXIT_FAILURE;
    }
}

void send_message(const std::string& message) 
{
    send(client_socket, message.c_str(), message.size(), 0);
}

void display_board() 
{
    char buffer[BUFFER_SIZE] = {0};
    receive_message(buffer, sizeof(buffer));
    std::cout << buffer;
}

void play_game() 
{
    while (true) {
        display_board();

        std::cout << "Enter your move (1-9): ";
        int move;
        std::cin >> move;

        // Convert the move to a string
        std::string move_str = std::to_string(move);

        // Send the move to the server
        send_message(move_str);

        // Receive and print the updated board from the server
        receive_message(buffer, sizeof(buffer));
        std::cout << buffer;

        // Check if the game is over
        if (strstr(buffer, "Result") != nullptr) {
            break;
        }
    }
}

int main() 
{
    connect_to_server();

    char buffer[BUFFER_SIZE];

    // Receive and print the initial board
    receive_message(buffer, sizeof(buffer));
    std::cout << buffer;

    // Receive and print the player message
    receive_message(buffer, sizeof(buffer));
    std::cout << buffer;

    // Play the game
    play_game();

    close(client_socket);

    return 0;
}