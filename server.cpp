#include <iostream>
#include <vector>

#include <algorithm>

#include <sstream>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <thread>
#include <mutex>

const int PORT = 12364;
const int BUFFER_SIZE = 1024;
std::vector<std::vector<char>> board;

void creat_board() 
{
    const int size = 3;
    board.resize(size, std::vector<char>(size, ' '));
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            board[i][j] = '1' + i * size + j;
        }
    }
}

void display() 
{
    std::cout << "-------------" << std::endl;
    std::cout << "| " << board[0][0] << " | " << board[0][1] << " | " << board[0][2] << " |" << std::endl;
    std::cout << "-------------" << std::endl;
    std::cout << "| " << board[1][0] << " | " << board[1][1] << " | " << board[1][2] << " |" << std::endl;
    std::cout << "-------------" << std::endl;
    std::cout << "| " << board[2][0] << " | " << board[2][1] << " | " << board[2][2] << " |" << std::endl;
    std::cout << "-------------" << std::endl;
    std::cout << std::endl;
}

std::string serialization() 
{
    const int size = 3;
    std::ostringstream os {};
    os << "  ------------------" << std::endl;
    for (int i = 0; i < size; i++) {
        os << " " << " | ";
        for (int j = 0; j < size; j++) {
            os << board[i][j] << "  |  ";     
        }
        os << std::endl;
        os << "  ------------------" << std::endl;
    }

    return os.str();;
}

std::pair<int, int> convert_to_coordinates(const int number) 
{
    int row = (number - 1) / 3;
    int col = (number - 1) % 3;
    return std::make_pair(row, col);
}

bool is_full_cell(int coordinate) 
{
    const auto [row, col] = convert_to_coordinates(coordinate);
    const char symbol = board[row][col];

    // Check if the symbol is present in the cell
    return std::find(std::begin({'X', 'O'}), std::end({'X', 'O'}), symbol) != std::end({'X', 'O'});
}

bool make_move(int coordinate, char symbol) 
{
    const auto [row, col] = convert_to_coordinates(coordinate);

    // Check if the coordinates are out of bounds or the cell is already filled
    if (row < 0 || row >= board.size() || col < 0 || col >= board[0].size() || is_full_cell(coordinate)) {
        return false;
    }

    // Make the move
    board[row][col] = symbol;
    return true;
}

bool check_win(char player) 
{
    const int size = 3;
    for (int i = 0; i < size; ++i) {
        if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
            (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
            return true; // Check rows and columns
        }
    }

    if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
        (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
        return true; // Check diagonals
    }

    return false;
}

bool is_draw() 
{
    const int size = 3;

    for (int i = 1; i <= size * size; ++i) {
        if (!is_full_cell(i)) {
            return false;
        }
    }
    return true;
}


void send_message(int client_socket, const std::string& message) 
{
    send(client_socket, message.c_str(), message.size(), 0);
}

std::string receive_message(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    recv(client_socket, buffer, sizeof(buffer), 0);
    return std::string(buffer);
}


std::mutex my_mutex;
void client_handler(int client_socket, char player, std::string& turn, std::string& wait) 
{
    static int total_moves = 0;  // Track the total number of moves made
    const int max_moves = 9;     // Maximum number of moves in a game

    while (true) {
        // Check if it's the player's turn based on the total number of moves
        if ((total_moves % 2 == 0 && player == 'X') || (total_moves % 2 == 1 && player == 'O')) {
            send_message(client_socket, turn);
        } else {
            send_message(client_socket, wait);
        }

        // Receive the player's move
        std::string move_str = receive_message(client_socket);
        int move = std::stoi(move_str);

        // Update the game board based on the player's move
        if (make_move(move, player)) {
            total_moves++;

            if (check_win(player)) {
                // Player wins
                send_message(client_socket, "You win!");
                break;
            } else if (is_drow()) {
                // It's a draw
                send_message(client_socket, "It's a draw!");
                break;
            }
        } else {
            // Invalid move, ask the player to try again
            send_message(client_socket, "Invalid move. Try again.");
        }
    }

    // Close the socket for this player
    close(client_socket);
}

int main() 
{
    std::cout << "|Tic tac toe|" << std::endl;
    creat_board();
    display();

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error creating server socket\n";
        return EXIT_FAILURE;
    }

    // Bind the socket to an IP address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket\n";
        close(server_socket);
        return EXIT_FAILURE;
    }


    // Listen for incoming connections
    if (listen(server_socket, 2) == -1) {
        std::cerr << "Error listening for connections\n";
        close(server_socket);
        return EXIT_FAILURE;
    }

    std::cout << "Waiting for players to connect..." << std::endl;

    int player_x = accept(server_socket, nullptr, nullptr);
    int player_o = accept(server_socket, nullptr, nullptr);

    std::string turn = "Your turn : ";
    std::string wait {};

    while (true) {
        std::thread thread_player_x(client_handler, player_x, 'X', turn, wait);
        std::thread thread_player_o(client_handler, player_o, 'O', wait, turn);

        thread_player_x.join();
        thread_player_o.join();
    }


    // Close sockets
    close(server_socket);
    close(player_x);
    close(player_o);


    return 0;
}