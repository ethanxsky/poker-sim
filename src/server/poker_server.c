#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "poker_client.h"
#include "client_action_handler.h"
#include "game_logic.h"

#define BASE_PORT 2201
#define NUM_PORTS 6
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
} player_t;

game_state_t game; //global variable to store our game state info (this is a huge hint for you)


void handle_betting_round(game_state_t *game){
    int starting_player = game->current_player;
    do{
    for(int i = 0; i < game->num_players; i++){
        printf("\n\nRunning %d times\n\n", i + 1);
        char buffer[BUFFER_SIZE];
        int bytes_recieved = recv(game->sockets[game->current_player], buffer, sizeof(client_packet_t), 0);
        // if(bytes_recieved < 0){
        //     exit(EXIT_FAILURE);
        // }
        client_packet_t  *clientpkt = (client_packet_t *) buffer;
        server_packet_t serverpkt;
        if(handle_client_action(game, game->current_player, clientpkt, &serverpkt) < 0){
            printf("\nGOING AGAIN\n");
            i--;
        }
    }
    }while(check_betting_end(game) != 1);
    // if(check_betting_end(game) != 1){
    //     while(check_betting_end(game) != 1){
    //         char buffer[BUFFER_SIZE];
    //         int bytes_recieved = recv(game->sockets[game->current_player], buffer, sizeof(client_packet_t), 0);
    //         if(bytes_recieved < 0){
    //             exit(EXIT_FAILURE);
    //         }
    //         client_packet_t  *clientpkt = (client_packet_t *) buffer;
    //         server_packet_t serverpkt;
    //         handle_client_action(game, game->current_player, clientpkt, &serverpkt);
    //     }
    // }
    game->current_player = starting_player;
    
}




int main(int argc, char **argv) {
    int server_fds[NUM_PORTS], client_socket, player_count = 0;
    int opt = 1;
    struct sockaddr_in server_address;
    player_t players[MAX_PLAYERS];
    char buffer[BUFFER_SIZE] = {0};
    socklen_t addrlen = sizeof(struct sockaddr_in);

    //Setup the server infrastructre and accept the 6 players on ports 2201, 2202, 2203, 2204, 2205, 2206

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;


    for(int i = 0; i < NUM_PORTS; i++){
        server_fds[i] = socket(AF_INET, SOCK_STREAM, 0);

        if(server_fds[i] < 0){
            exit(EXIT_FAILURE);
        }
        printf("\nthe listening socket for player %d is %d", i, server_fds[i]);
        setsockopt(server_fds[i], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        server_address.sin_port = htons(BASE_PORT + i);

        if (bind(server_fds[i], (struct sockaddr *)&server_address, addrlen) < 0) {
            perror("bind() failed");
            printf("\nEXITED EARLY\n");
            exit(EXIT_FAILURE);
        }



    }

    for(int i = 0; i < NUM_PORTS; i++){
        if (listen(server_fds[i], 1) < 0){
            exit(EXIT_FAILURE);
        }
    }
    
    //Join state?

    int rand_seed = argc == 2 ? atoi(argv[1]) : 0;
    init_game_state(&game, 100, rand_seed);
    game.round_stage = ROUND_JOIN;


    for(int i = 0; i < NUM_PORTS; i++){

        struct sockaddr_in client_address;
        client_socket = accept(server_fds[player_count], (struct sockaddr*) &client_address, &addrlen);
        if(client_socket < 0){
            exit(EXIT_FAILURE);
        }
        printf("\nthe client socket for player %d is %d", player_count, client_socket);
        game.sockets[player_count] = client_socket;

        char buffer[BUFFER_SIZE];
        int bytes_recieved = recv(game.sockets[player_count], buffer, sizeof(client_packet_t), 0);
            
        if(bytes_recieved < 0){
            exit(EXIT_FAILURE);
        }

        client_packet_t *clientpkt = (client_packet_t *) buffer;
        server_packet_t serverpkt;
        if(clientpkt->packet_type == JOIN){
            players[player_count].socket = game.sockets[player_count];
            players[player_count].address = client_address;    
            game.player_status[player_count] = PLAYER_ACTIVE;                         
            player_count++;
        }

    
    }

    game.round_stage = ROUND_INIT;



    while (1) {
        //READY
        game.num_players = 0;
        
        for(int i = 0; i < player_count; i++){
            if (game.player_status[i] == PLAYER_LEFT){
                continue;
            }

            char buffer[BUFFER_SIZE];

            int bytes_recieved = recv(game.sockets[i], buffer, sizeof(client_packet_t), 0);
            
            if(bytes_recieved < 0){
                exit(EXIT_FAILURE);
            }
            client_packet_t *clientpkt = (client_packet_t *) buffer;

            if(clientpkt->packet_type == LEAVE){
                game.player_status[i] = PLAYER_LEFT;
                close(game.sockets[i]); 
            }

            if(clientpkt->packet_type == READY){
                game.num_players++;
            }

        }

        if(server_ready(&game) < 2){
            printf("\nENDING SERVER\n");
            break;
        }


        if(game.player_status[game.dealer_player] != PLAYER_ACTIVE){
            while(game.player_status[game.dealer_player] != PLAYER_ACTIVE){
                game.dealer_player = (game.dealer_player + 1) % MAX_PLAYERS;
            }
            game.current_player = (game.dealer_player + 1) % MAX_PLAYERS;
        }


        if(game.player_status[game.current_player] != PLAYER_ACTIVE || game.current_player == game.dealer_player){
            while(game.player_status[game.current_player] != PLAYER_ACTIVE && game.current_player != game.dealer_player){
                game.current_player = (game.current_player + 1) % MAX_PLAYERS;
            }
        }

        printf("\n\n\n\nCurrent number of players is %d\n\n\n", game.num_players);
        game.round_stage = ROUND_PREFLOP;

        
        // DEAL TO PLAYERS


        server_deal(&game);

        // PREFLOP BETTING
        handle_betting_round(&game);

        // PLACE FLOP CARDS
        game.round_stage = ROUND_FLOP;
        server_community(&game);
        

        // FLOP BETTING
        handle_betting_round(&game);

        // PLACE TURN CARDS
        game.round_stage = ROUND_TURN;
        server_community(&game);

        // TURN BETTING
        handle_betting_round(&game);


        // PLACE RIVER CARDS
        game.round_stage = ROUND_RIVER;
        server_community(&game);


        // RIVER BETTING
        handle_betting_round(&game);

        // ROUND_SHOWDOWN
        game.round_stage = ROUND_SHOWDOWN;
        server_end(&game);
        reset_game_state(&game);

        

    }
    
    //SEND HALT PACKAGES
    for(int i = 0; i < MAX_PLAYERS; i++){
        if(game.player_status[i] != PLAYER_LEFT){
            server_packet_t serverpkt;
            serverpkt.packet_type = HALT;
            send(game.sockets[i], &serverpkt, sizeof(serverpkt), 0);
        }
    }
        


        
    

    printf("[Server] Shutting down.\n");

    // Close all fds (you're welcome)
    for (int i = 0; i < MAX_PLAYERS; i++) {
        close(server_fds[i]);
        if (game.player_status[i] != PLAYER_LEFT) {
            close(game.sockets[i]);
        }
    }

    return 0;
}