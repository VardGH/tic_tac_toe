#include "action.hpp"
#include "common.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0) {
        std::cout << "ERROR writing int to server socket" << std::endl;
        exit(0);
    }
}

int connect_to_server(char * hostname, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sockfd < 0) {
        std::cout << "ERROR opening socket for server." << std::endl;
    }
    
    server = gethostbyname(hostname);
	
    if (server == nullptr) {
        std::cout << "ERROR, no such host " << hostname << std::endl; 
        exit(0);
    }
	
	memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    memmove(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

   if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "ERROR connecting to server" << std::endl;
   }

    return sockfd;
}

void take_turn(int sockfd)
{
    char buffer[10];
    while (1) { 
        std::cout << "Enter 0-8 to make a move, or 9 for number of active players: ";
	    fgets(buffer, 10, stdin);
	    int move = buffer[0] - '0';
        if (move <= 9 && move >= 0){
            std::cout << std::endl;
            write_server_int(sockfd, move);   
            break;
        } else {
            std::cout << "\nInvalid input. Try again.\n" << std::endl;
        }
    }
}

void clear_screen()
{
    system("clear");
}

void update_board(int sockfd, char board[][3])
{
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);
    board[move/3][move%3] = player_id ? 'X' : 'O';
    clear_screen();    
    draw_board(board);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
       std::cout << "usage " << argv[0] << " hostname port\n";
       exit(0);
    }

    int sockfd = connect_to_server(argv[1], atoi(argv[2]));
    int id = recv_int(sockfd);
    char board[3][3] = { {' ', ' ', ' '}, 
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    printf("Tic-Tac-Toe\n------------\n");

    action act = action::HOLD;
    do {
        act = read_action(sockfd);
        if (act == action::HOLD) {
            printf("Waiting for a second player...\n");
        }
    } while ( act == action::START );

    /* The game has begun. */
    printf("Game on!\n");
    printf("Your are %c's\n", id ? 'X' : 'O');


    draw_board(board);

    while(true) {
        act = read_action(sockfd);
        switch(act) {
        case action::TURN: {
	        std::cout << "Your move...\n" << std::endl;
	        take_turn(sockfd);
            break;
        } case action::INVALID: {
            std::cout << "That position has already been played. Try again.\n" << std::endl;
            break;
        } case action::COUNT: {
            int num_players = recv_int(sockfd);
            std::cout << "There are currently " <<  num_players << " active players." << std::endl;
            break;
        } case action::UPDATE: {
            update_board(sockfd, board);
            break;
        } case action::WAIT: {
            std::cout << "Waiting for other players move..." << std::endl;
            break;
        } case action::WIN: {
            std::cout << "You win!" << std::endl;
            return 0;
        } case action::LOSE: {
            std::cout << "You lost." << std::endl;
            return 0;
        } case action::DRAW: {
            std::cout << "Draw." << std::endl;
            return 0;
        } default:
            std::cout << "Unknown message." << std::endl;
            break;
        }
    }
    
    std::cout << "Game over." << std::endl;
    close(sockfd);
    return 0;
}