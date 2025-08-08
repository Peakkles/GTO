#include <algorithm>
#include <iostream>
#include <random>

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
        players[i] = Player(deck[top_card_index--], deck[top_card_index--]);
    }

    player_turn = 0;
}

void Game::run_game() {
    // Pre flop first take blinds

    players[0].chips -= small_blind;
    players[1].chips -= small_blind * 2;

    player_turn = 2;
}

void Game::dfs(float probability) {
    auto &curr_player = players[player_turn];
    auto state = curr_player.calc_gamestate();
    for (int i = 0; i < NUM_ACTIONS; i++) {
        if (i == FOLD) {
            curr_player.folded = true;
            player_turn = (player_turn + 1) % NUM_PLAYERS;
            float new_prob = curr_player.strategy[state][i] * probability;
            dfs(new_prob);
            curr_player.folded = false;
        }
    }
}



int main() {
    Game game;
}
