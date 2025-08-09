#include <algorithm>
#include <iostream>
#include <random>
#include <cassert>

#include "poker.hpp"

Game::Game() {
    top_card_index = 51;
    small_blind = 1;

    int idx = 0;
    for (int j = Suit::HEART; j <= Suit::CLUB; j++) {
        for (char i = 2; i <= 14; i++) {
            deck[idx++] = {i, static_cast<Suit>(j)};
        }
    }

    unsigned seed = 73;
    std::shuffle(&deck[0], &deck[52], std::default_random_engine(seed));

    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i] = Player(draw(), draw());
    }
}

Card Game::draw() {
    assert(top_card_index >= 0);
    return deck[top_card_index--];
}

void Game::run_game() {
    // Pre flop first take blinds

    players[0].chips -= small_blind;
    players[1].chips -= small_blind * 2;

}

bool Game::all_folded() {
    int active_players = 0;
    for (Player &p: players) {
        if (!p.folded) active_players++;
    }
    return active_players == 1;
}

float Game::dfs(float probability, int last_aggressor, int player_turn) {
    if (all_folded()) {
        float ev;
        if (players[main_character].folded) {
            ev = probability * (players[main_character].chips - INITIAL_CHIPS);
        }
        else ev = probability * (players[main_character].chips + pot - INITIAL_CHIPS);
        return ev;    
    }
    int undo_community_cards = 0;
    if (last_aggressor == player_turn) {
        if (community_cards.empty()) {
            community_cards.push_back(draw());
            community_cards.push_back(draw());
            community_cards.push_back(draw());
            undo_community_cards = 3;
        }
        else if (community_cards.size() < 5) {
            community_cards.push_back(draw());
            undo_community_cards = 1;
        }
        else {
            // find winner, handle main player ev 
            // return ev;
        }
        player_turn = 0;
        last_aggressor = 0;
    }
    Player &curr_player = players[player_turn];
    int nxt_player = (player_turn + 1) % NUM_PLAYERS;
    if (curr_player.folded) {
        return dfs(probability, last_aggressor, player_turn + 1);
    }
    auto state = curr_player.calc_gamestate();

    float total_ev = 0;
    for (int i = 0; i < NUM_ACTIONS; i++) {
        int to_call = current_bet - curr_player.bet_made;
        if (i == FOLD && to_call) {
            curr_player.folded = true;
            float new_prob = curr_player.strategy[state][i] * probability;
            float ev = dfs(new_prob, last_aggressor, nxt_player);
            if (main_character == player_turn) curr_player.ev[state][i] += ev;
            total_ev += ev;
            curr_player.folded = false;
        }
        else if (i == CALL) {
            pot += to_call;
            curr_player.bet_made += to_call;
            curr_player.chips -= to_call;
            float new_prob = curr_player.strategy[state][i] * probability;
            float ev = dfs(new_prob, last_aggressor, nxt_player);
            if (main_character == player_turn) curr_player.ev[state][i] += ev;
            total_ev += ev;
            pot -= to_call;
            curr_player.bet_made -= to_call;
            curr_player.chips += to_call;
        }
        else if (i == RAISE) {
            int new_aggressor = player_turn;
            int bet = std::min(int((pot + to_call) * 0.5 + to_call), curr_player.chips);
            pot += bet;
            curr_player.bet_made += bet;
            current_bet += bet - to_call;
            curr_player.chips -= bet;
            float new_prob = curr_player.strategy[state][i] * probability;
            float ev = dfs(new_prob, new_aggressor, nxt_player);
            if (main_character == player_turn) curr_player.ev[state][i] += ev;
            total_ev += ev;
            pot -= bet;
            curr_player.bet_made -= bet;
            current_bet -= bet - to_call;
            curr_player.chips += bet;
        }
    }

    return total_ev;
}



int main() {
    Game game;
}
