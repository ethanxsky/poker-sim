#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>


#include "client_action_handler.h"
#include "game_logic.h"


void next_active_player(game_state_t *game) {
    int count = 0;

    game->current_player = ((game->current_player) + 1) % MAX_PLAYERS;

    if(game->player_status[game->current_player] != PLAYER_ACTIVE){
        printf("\nTHIS IS RUNNING\n");
        while(game->player_status[game->current_player] != PLAYER_ACTIVE){
            game->current_player = ((game->current_player) + 1) % MAX_PLAYERS;
            count++;
            if(count > MAX_PLAYERS){
                printf("\n\nSOMETHING GOING WRONG\n\n");
                return;
            }

        }
    }

}

/**
 * @brief Processes packet from client and generates a server response packet.
 * 
 * If the action is valid, a SERVER_INFO packet will be generated containing the updated game state.
 * If the action is invalid or out of turn, a SERVER_NACK packet is returned with an optional error message.
 * 
 * @param pid The ID of the client/player who sent the packet.
 * @param in Pointer to the client_packet_t received from the client.
 * @param out Pointer to a server_packet_t that will be filled with the response.
 * @return 0 if successful processing, -1 on NACK or error.
 */




int handle_client_action(game_state_t *game, player_id_t pid, const client_packet_t *in, server_packet_t *out) {
    //Optional function, see documentation above. Strongly reccomended.
    server_packet_t ACKpkt;
    ACKpkt.packet_type = ACK;
    server_packet_t NACKpkt;
    NACKpkt.packet_type = NACK;

    switch(in->packet_type){
        
        case RAISE:

        send(game->sockets[pid], &ACKpkt, sizeof(server_packet_t), 0);

        next_active_player(game);


        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->player_status[i] != PLAYER_LEFT){
                build_info_packet(game, i, out);
                send(game->sockets[i], out, sizeof(*out), 0);
            }
        }



        return 0;

        case CALL:

        send(game->sockets[pid], &ACKpkt, sizeof(server_packet_t), 0);

        next_active_player(game);


        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->player_status[i] != PLAYER_LEFT){
                build_info_packet(game, i, out);
                send(game->sockets[i], out, sizeof(*out), 0);
            }
        }
  
        return 0;

        case CHECK:

        send(game->sockets[pid], &ACKpkt, sizeof(server_packet_t), 0);

        next_active_player(game);
        printf("\nthe new current player is %d\n", game->current_player);


        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->player_status[i] != PLAYER_LEFT){
                build_info_packet(game, i, out);
                send(game->sockets[i], out, sizeof(*out), 0);
            }
        }

        
        return 0;

        case FOLD:

        send(game->sockets[pid], &ACKpkt, sizeof(server_packet_t), 0);
        game->player_status[pid] = PLAYER_FOLDED;
        next_active_player(game);


        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->player_status[i] != PLAYER_LEFT){
                build_info_packet(game, i, out);
                send(game->sockets[i], out, sizeof(*out), 0);
            }
        }
        
        return 0;



        default:

        //send(game->sockets[pid], &NACKpkt, sizeof(server_packet_t), 0);
        return -1;

    }

    return -1;
}

void build_info_packet(game_state_t *game, player_id_t pid, server_packet_t *out) {
    //Put state info from "game" (for player pid) into packet "out"

    memset(out, 0, sizeof(server_packet_t));

    out->packet_type = INFO;
    game_state_t game_copy = *game;
    game_state_t *end = &game_copy;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE || game->player_status[i] == PLAYER_ALLIN) {
            end->player_status[i] = 1;
        } else if (game->player_status[i] == PLAYER_FOLDED) {
            end->player_status[i] = 0;
        } else {
            end->player_status[i] = 2;
        }
    }
    
    out->info.player_cards[0] = game->player_hands[pid][0];
    out->info.player_cards[1] = game->player_hands[pid][1];
    
    for(int i = 0; i < 5; i++){
        out->info.community_cards[i] = game->community_cards[i];
    }

    

    for(int i = 0; i < MAX_PLAYERS; i++){
        out->info.player_stacks[i] = game->player_stacks[i];
    }
    out->info.pot_size = game->pot_size;
    out->info.dealer = game->dealer_player;
    out->info.player_turn = game->current_player;
    out->info.bet_size = game->highest_bet;
    for(int i = 0; i < MAX_PLAYERS; i++){
        out->info.player_bets[i] = game->current_bets[i];
    }
    for(int i = 0; i < MAX_PLAYERS; i++){
        out->info.player_status[i] = end->player_status[i];
    }
}

void build_end_packet(game_state_t *game, player_id_t winner, server_packet_t *out) {
    //Put state info from "game" (and calculate winner) into packet "out"

    out->packet_type = END;
    game_state_t game_copy = *game;
    game_state_t *end = &game_copy;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE || game->player_status[i] == PLAYER_ALLIN) {
            end->player_status[i] = 1;
        } else if (game->player_status[i] == PLAYER_FOLDED) {
            end->player_status[i] = 0;
        } else {
            end->player_status[i] = 2;
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++){
        for(int x = 0; x < 2; x++){
            out->end.player_cards[i][x] = game->player_hands[i][x];
        }
    }

    for (int i = 0; i < 5; i++){
        out->end.community_cards[i] = game->community_cards[i];
    }

    for(int i = 0; i < MAX_PLAYERS; i++){
        out->end.player_stacks[i] = game->player_stacks[i];
    }

    out->end.pot_size = game->pot_size;
    out->end.dealer = game->dealer_player;
    out->end.winner = winner;

    for(int i = 0; i < MAX_PLAYERS; i++){
        out->end.player_status[i] = end->player_status[i];
    }


}