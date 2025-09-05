#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "poker_client.h"
#include "client_action_handler.h"
#include "game_logic.h"

//Feel free to add your own code. I stripped out most of our solution functions but I left some "breadcrumbs" for anyone lost

// for debugging
void print_game_state( game_state_t *game){
    (void) game;
}

void init_deck(card_t deck[DECK_SIZE], int seed){ //DO NOT TOUCH THIS FUNCTION
    srand(seed);
    int i = 0;
    for(int r = 0; r<13; r++){
        for(int s = 0; s<4; s++){
            deck[i++] = (r << SUITE_BITS) | s;
        }
    }
}

void shuffle_deck(card_t deck[DECK_SIZE]){ //DO NOT TOUCH THIS FUNCTION
    for(int i = 0; i<DECK_SIZE; i++){
        int j = rand() % DECK_SIZE;
        card_t temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

//You dont need to use this if you dont want, but we did.
void init_game_state(game_state_t *game, int starting_stack, int random_seed){
    memset(game, 0, sizeof(game_state_t));
    init_deck(game->deck, random_seed);
    shuffle_deck(game->deck);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->player_stacks[i] = starting_stack;
    }
    for (int i = 0; i < MAX_COMMUNITY_CARDS; i++){
        game->community_cards[i] = NOCARD;
    }
}

void reset_game_state(game_state_t *game) {
    shuffle_deck(game->deck);
    for(int i = 0; i < MAX_PLAYERS; i++){
        for(int x = 0; x < HAND_SIZE; x++){
            game->player_hands[i][x] = 0;
        }
        if(game->player_status[i] != PLAYER_LEFT){
            game->player_status[i] = PLAYER_ACTIVE;
        }
        game->current_bets[i] = 0;

    }

    for(int i = 0; i < MAX_COMMUNITY_CARDS; i++){
        game->community_cards[i] = NOCARD;
    }

    game->next_card = 0;
    
    game->highest_bet = 0;
    game->pot_size = 0;
    game->round_stage = ROUND_INIT;
    //Call this function between hands.
    //You should add your own code, I just wanted to make sure the deck got shuffled.
}

void server_join(game_state_t *game) {
    //This function was called to get the join packets from all players
    (void) game;
}

int server_ready(game_state_t *game) {
    //This function updated the dealer and checked ready/leave status for all players
    //It will also set the current player to one after dealer
    int ready_players = 0;
    for(int i = 0 ; i < MAX_PLAYERS; i++){
        if(game->player_status[i] == PLAYER_ACTIVE){
            ready_players++;
        }
    }

    if(ready_players >= 2 && game->round_stage == ROUND_INIT){
        game->current_player = (game->dealer_player + 1) % MAX_PLAYERS;
        return ready_players;
    }

    if(ready_players >= 2 && game->round_stage == ROUND_SHOWDOWN){
        game->dealer_player = (game->current_player) % MAX_PLAYERS;
        game->current_player = (game->current_player + 1) % MAX_PLAYERS;

    }

    return ready_players;
}

//This was our dealing function with some of the code removed (I left the dealing so we have the same logic)
void server_deal(game_state_t *game) {
    int starting_player = game->current_player;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            game->player_hands[i][0] = game->deck[game->next_card++];
            game->player_hands[i][1] = game->deck[game->next_card++];
        }
    }
    for(int i = 0; i < MAX_PLAYERS; i++){
        if(game->player_status[i] != PLAYER_LEFT){
            server_packet_t serverpkt;
            build_info_packet(game, i, &serverpkt);
            send(game->sockets[i], &serverpkt, sizeof(serverpkt), 0);
        }
    }
}

int server_bet(game_state_t *game) {
    //This was our function to determine if everyone has called or folded
    (void) game;
    return 0;
}

// Returns 1 if all bets are the same among active players
int check_betting_end(game_state_t *game) {
    int active_players = 0;
    int matched_players = 0;

    for(int i = 0; i < MAX_PLAYERS; i++){
        if(game->player_status[i] == PLAYER_ACTIVE || game->player_status[i] == PLAYER_ALLIN){
            active_players++;
        }

        if(game->current_bets[i] == game->highest_bet || game->player_status[i] == PLAYER_ALLIN){
            matched_players++;
        }
    }

    return (active_players > 0 && (active_players == matched_players));
}

void server_community(game_state_t *game) {
    printf("\n\nSERVER_COMMUNITY IS CALLED\n\n");
    printf("\nThe round stage is %d\n", game->round_stage);
    //This function checked the game state and dealt new community cards if needed;
    switch(game->round_stage){
        case ROUND_FLOP:
        for(int i = 0; i < 3; i++){
            game->community_cards[i] = game->deck[game->next_card++];
        }
        printf("\nAssigned community cards\n");
        for(int i = 0; i < 3; i++){
            printf("\nthe community cards are %d\n", game->community_cards[i]);
        }
            break;

        case ROUND_TURN:
        game->community_cards[3] = game->deck[game->next_card++];
            break;

        case ROUND_RIVER:
        game->community_cards[4] = game->deck[game->next_card++];
            break;
        default:
        break;
    }

    for(int i = 0; i < MAX_PLAYERS; i++){
        printf("\nPLAYER %d IS BEING SENT\n", i);
        if(game->player_status[i] != PLAYER_LEFT){
            server_packet_t serverpkt;
            build_info_packet(game, i, &serverpkt);
            send(game->sockets[i], &serverpkt, sizeof(serverpkt), 0);
        }
    }
}

void server_end(game_state_t *game) {
    int winner = 0;
    winner = find_winner(game);
    
    for(int i = 0; i < game->num_players; i++){
        if(game->player_status[i] != PLAYER_LEFT){
            server_packet_t serverpkt;
            build_end_packet(game, winner, &serverpkt);
            send(game->sockets[i], &serverpkt, sizeof(end_packet_t), 0);
        }
    }
}

typedef enum {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8
} hands_t;


int evaluate_hand(game_state_t *game, player_id_t pid) {
    //We wrote a function to compare a "value" for each players hand (to make comparison easier)
    //Feel free to not do this.
    int testvalue = game->player_status[pid];
    int test = testvalue-testvalue + 8;

    return test;
}

int find_winner(game_state_t *game) {
    //MAKE SURE TO ADD POT SIZE TO WINNER AND SUBTRACT THE OTHER PLAYER STACKS

    int winner = 0;

    return winner;
}