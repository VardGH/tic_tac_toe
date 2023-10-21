#include "action.hpp"
#include "common.hpp"

#include <pthread.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>


int player_count = 0;
pthread_mutex_t mutexcount;

void send_client_message(int cli_sockfd, int value)
{
    int n = write(cli_sockfd, &value, sizeof(int));
    if (n < 0) {
        std::cout << "ERROR writing msg to client socket" << std::endl;
    }
}

void send_client_message(int cli_sockfd, action act)
{
    auto value = action_to_string(act);
    int n = write(cli_sockfd, value.c_str(), value.size());
    if (n < 0) {
        std::cout << "ERROR writing msg to client socket" << std::endl;
    }
}

void send_clients_message(int * cli_sockfd, action act)
{
    send_client_message(cli_sockfd[0], act);
    send_client_message(cli_sockfd[1], act);
}

void write_clients_int(int * cli_sockfd, int msg)
{
    send_client_message(cli_sockfd[0], msg);
    send_client_message(cli_sockfd[1], msg);
}

int setup_listener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        std::cout << "ERROR opening listener socket." << std::endl;
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;	
    serv_addr.sin_addr.s_addr = INADDR_ANY;	
    serv_addr.sin_port = htons(portno);		

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "ERROR binding listener socket." << std::endl;
    }

    return sockfd;
}

void get_clients(int lis_sockfd, int * cli_sockfd)
{
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    int num_conn = 0;
    while(num_conn < 2) {
	    listen(lis_sockfd, 253 - player_count);
        
        memset(&cli_addr, 0, sizeof(cli_addr));

        clilen = sizeof(cli_addr);
        cli_sockfd[num_conn] = accept(lis_sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (cli_sockfd[num_conn] < 0) {
            std::cout << "ERROR accepting a connection from a client." << std::endl;
        }
        
        write(cli_sockfd[num_conn], &num_conn, sizeof(int));
        
        pthread_mutex_lock(&mutexcount);
        ++player_count;
        std::cout << "Number of players is now" << player_count << std::endl;
        pthread_mutex_unlock(&mutexcount);

        if (num_conn == 0) {
            send_client_message(cli_sockfd[0], action::HOLD);
        }
        ++num_conn;
    }
}

int get_player_move(int cli_sockfd)
{
    send_client_message(cli_sockfd, action::TURN);
    return recv_int(cli_sockfd);
}

int check_move(char board[][3], int move, int player_id)
{
    if ((move == 9) || (board[move/3][move%3] == ' ')) { 
        return 1;
    }
    return 0;
}

void update_board(char board[][3], int move, int player_id)
{
    board[move / 3][move % 3] = player_id ? 'X' : 'O';
    draw_board(board);
}

void send_update(int * cli_sockfd, int move, int player_id)
{    
    send_clients_message(cli_sockfd, action::UPDATE);
    write_clients_int(cli_sockfd, player_id);
    write_clients_int(cli_sockfd, move);
}

void send_player_count(int cli_sockfd)
{
    send_client_message(cli_sockfd, action::COUNT);
    send_client_message(cli_sockfd, player_count);
}

bool check_board(char board[][3], int last_move)
{  
    int row = last_move / 3;
    int col = last_move % 3;

    if ( board[row][0] == board[row][1] && board[row][1] == board[row][2] ) { 
        return true;
    } else if ( board[0][col] == board[1][col] && board[1][col] == board[2][col] ) { 
        return true;
    } else if (!(last_move % 2)) {
        if ( (last_move == 0 || last_move == 4 || last_move == 8) && (board[1][1] == board[0][0] && board[1][1] == board[2][2]) ) {
            return true;
        }
        if ( (last_move == 2 || last_move == 4 || last_move == 6) && (board[1][1] == board[0][2] && board[1][1] == board[2][0]) ) {
            return true;
        }
    }
    return false;
}

void *run_game(void *thread_data) 
{
    int *cli_sockfd = (int*)thread_data; 
    char board[3][3] = { {' ', ' ', ' '},  
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    std::cout << "Starting Game!" << std::endl;
    
    send_clients_message(cli_sockfd, action::START);

    draw_board(board);
    
    int prev_player_turn = 1;
    int player_turn = 0;
    bool game_over = false;
    int turn_count = 0;
    while(!game_over) {
        
        if (prev_player_turn != player_turn) {
            send_client_message(cli_sockfd[(player_turn + 1) % 2], action::WAIT);
        }

        int valid = 0;
        int move = 0;
        while(!valid) {
            move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1) {
                break;
            }
            std::cout << "Player " << player_turn << " played position " << move << std::endl;
                
            valid = check_move(board, move, player_turn);
            if (!valid) { 
                send_client_message(cli_sockfd[player_turn], action::INVALID);
            }
        }

	    if (move == -1) { 
            std::cout << "Player disconnected." << std::endl;
            break;
        } else if (move == 9) {
            prev_player_turn = player_turn;
            send_player_count(cli_sockfd[player_turn]);
        } else {
            update_board(board, move, player_turn);
            send_update( cli_sockfd, move, player_turn );
            game_over = check_board(board, move);
            
            if (game_over) {
                send_client_message(cli_sockfd[player_turn], action::WIN);
                send_client_message(cli_sockfd[(player_turn + 1) % 2], action::LOSE);
            } else if (turn_count == 8) {
                send_clients_message(cli_sockfd, action::DRAW);
                game_over = true;
            }

            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
        }
    }

    std::cout << "Game over.\n" << std::endl;

	close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count -= 2;
    pthread_mutex_unlock(&mutexcount);
    
    free(cli_sockfd);

    pthread_exit(nullptr);
}


int main(int argc, char *argv[])
{   
    if (argc < 2) {
        std::cout << "ERROR, no port provided\n" << std::endl;
        return 0;
    }
    
    int lis_sockfd = setup_listener(atoi(argv[1])); 
    pthread_mutex_init(&mutexcount, nullptr);

    while (true) {
        int* cli_sockfd = (int*)malloc(2 * sizeof(int));
        memset(cli_sockfd, 0, 2 * sizeof(int));
        get_clients(lis_sockfd, cli_sockfd);

        pthread_t thread;
        int result = pthread_create(&thread, nullptr, run_game, (void *)cli_sockfd);
        if (result){
            std::cout << "[INFO] Failed to create thread" << std::endl;
            return 0;
        }
    }

    close(lis_sockfd);

    pthread_mutex_destroy(&mutexcount);
    pthread_exit(nullptr);
}