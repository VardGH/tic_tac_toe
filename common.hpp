#include "action.hpp"

#include <unistd.h>
#include <iostream>
#include <cstring>

action read_action(int sockfd)
{
    char msg[4];
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3) {
        std::cout << "ERROR reading message from server socket.";
    }
    return string_to_action(msg);
}

void draw_board(char board[][3])
{
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
    printf("-----------\n");
}

int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) {
        return -1;
    }    
    return msg;
}