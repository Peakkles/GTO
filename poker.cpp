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
    dfs(1.0f, -1, 2);

}

bool Game::all_folded() {
    int active_players = 0;
    for (Player &p: players) {
        if (!p.folded) active_players++;
    }
    return active_players == 1;
}
int Game::evaluate_cards(int player) {
    Player &p = players[player];
    std::vector<Card> cards;
    cards.push_back(p.hole_cards[0]);
    cards.push_back(p.hole_cards[1]);
    assert(community_cards.size() >= 3);
    for (Card &c: community_cards) cards.push_back(c);

    int rank_count[15] = {}; // 2-14, 0 and 1 not used
    int suit_count[4] = {};

    for (auto& c : cards) {
        rank_count[c.rank]++;
        suit_count[c.suit]++;
    }

    // Sort cards by rank descending
    std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.rank > b.rank;
    });

    for (int suit = 0; suit < 4; ++suit) {
        if (suit_count[suit] >= 5) {
            int straight_count = -1;
            int last_rank = -1;
            for (Card &c: cards) {
                if (c.suit == suit) {
                    if (last_rank - 1 == c.rank) {
                        if(++straight_count == 5) return 8000000 + c.rank + 4;
                        last_rank--;
                    }
                    else {
                        straight_count = 1;
                        last_rank = c.rank;
                    }
                }
            }
            for (Card &c: cards) {
                if (c.suit == suit) return 5000000 + c.rank;
            }
        }
    }

    std::vector<std::pair<int, int>> freq_val_pairs;
    for (int rank = 2; rank <= 14; ++rank) {
        if (rank_count[rank] > 0) {
            freq_val_pairs.push_back({rank_count[rank], rank});
        }
    }
    std::sort(freq_val_pairs.begin(), freq_val_pairs.end(), [](const auto & a, const auto & b){
        if (a.first != b.first) return a.first > b.first;
        return a.second > b.second;
    });

    if (freq_val_pairs[0].first == 4) return 7000000 + freq_val_pairs[0].second;
    if (freq_val_pairs[0].first == 3 && freq_val_pairs[1].first == 2) {
        return 6000000 + freq_val_pairs[0].second * 10 + freq_val_pairs[1].second;
    }

    // check straight
    int straight_count = -1;
    int last_rank = -1;
    for (Card &c: cards) {
        if (c.rank == last_rank) continue;
        if (last_rank - 1 == c.rank) {
            if(++straight_count == 5) return 5000000 + c.rank + 4;
            last_rank--;
        }
        else {
            straight_count = 1;
            last_rank = c.rank;
        }
    }

    if (freq_val_pairs[0].first == 3) {
        return 3000000 + freq_val_pairs[0].second * 100 + freq_val_pairs[1].second * 10 + freq_val_pairs[2].second;
    }
    if (freq_val_pairs[0].first == 2 && freq_val_pairs[1].first == 2) {
        return 2000000 + freq_val_pairs[0].second * 100 + freq_val_pairs[1].second * 10 + freq_val_pairs[2].second;
    }
    if (freq_val_pairs[0].first == 2) {
        return 1000000 + freq_val_pairs[0].second * 1000 + freq_val_pairs[1].second * 100 + freq_val_pairs[2].second * 10 + freq_val_pairs[3].second;
    }
    return freq_val_pairs[0].second * 10000 + freq_val_pairs[1].second * 1000 + freq_val_pairs[2].second * 100 + freq_val_pairs[3].second * 10 + freq_val_pairs[4].second;
}
float Game::showdown(float probability) {
    int best_score = 0;
    int number_of_winners = 0;
    int main_player_score = 1e9;

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        Player &p = players[i];
        if (p.folded) continue;
        int score = evaluate_cards(i);
        if (i == main_character) main_player_score = score;
        if (score > best_score) {
            best_score = score;
            number_of_winners = 1;
        }
        else if (score == best_score) number_of_winners++;
        if (main_player_score < best_score) {
            return probability * (players[main_character].chips - INITIAL_CHIPS);
        }
    }

    return probability * (players[main_character].chips + (pot / number_of_winners) - INITIAL_CHIPS);
}

GameState Game::calc_gamestate(int player) {
    Player &p = players[player];
    if (community_cards.empty()) {
        int rank1 = p.hole_cards[0].rank;
        int rank2 = p.hole_cards[1].rank;
        if (rank1 < rank2) std::swap(rank1, rank2);
        bool suited = p.hole_cards[0].suit == p.hole_cards[1].suit;
        return GameState(rank1, rank2, suited, pre_raises);
    }
    int score = evaluate_cards(player);
    int cards_that_beat_us = 0;

    auto evaluate_suitless = [&](int a, int b) {
        std::vector<int> cards = {a, b};
        int rank_count[15] = {};
        rank_count[a]++;
        rank_count[b]++;
        for (Card &c: community_cards) {
            rank_count[c.rank]++;
            cards.push_back(c.rank);
        }
        std::sort(cards.begin(), cards.end(), [](const int &a, const int &b){return a > b;});
        std::vector<std::pair<int, int>> freq_val_pairs;
        for (int rank = 2; rank <= 14; ++rank) {
            if (rank_count[rank] > 0) {
                freq_val_pairs.push_back({rank_count[rank], rank});
            }
        }
        std::sort(freq_val_pairs.begin(), freq_val_pairs.end(), [](const auto & a, const auto & b){
            if (a.first != b.first) return a.first > b.first;
            return a.second > b.second;
        });

        if (freq_val_pairs[0].first == 4) return 7000000 + freq_val_pairs[0].second;
        if (freq_val_pairs[0].first == 3 && freq_val_pairs[1].first == 2) {
            return 6000000 + freq_val_pairs[0].second * 10 + freq_val_pairs[1].second;
        }

        // check straight
        int straight_count = -1;
        int last_rank = -1;
        for (int &c: cards) {
            if (c == last_rank) continue;
            if (last_rank - 1 == c) {
                if(++straight_count == 5) return 5000000 + c + 4;
                last_rank--;
            }
            else {
                straight_count = 1;
                last_rank = c;
            }
        }

        if (freq_val_pairs[0].first == 3) {
            return 3000000 + freq_val_pairs[0].second * 100 + freq_val_pairs[1].second * 10 + freq_val_pairs[2].second;
        }
        if (freq_val_pairs[0].first == 2 && freq_val_pairs[1].first == 2) {
            return 2000000 + freq_val_pairs[0].second * 100 + freq_val_pairs[1].second * 10 + freq_val_pairs[2].second;
        }
        if (freq_val_pairs[0].first == 2) {
            return 1000000 + freq_val_pairs[0].second * 1000 + freq_val_pairs[1].second * 100 + freq_val_pairs[2].second * 10 + freq_val_pairs[3].second;
        }
        return freq_val_pairs[0].second * 10000 + freq_val_pairs[1].second * 1000 + freq_val_pairs[2].second * 100 + freq_val_pairs[3].second * 10 + freq_val_pairs[4].second;

    };
    for (int high = 2; high <= 14; ++high) {
        for (int low = 2; low <= high; ++low) {
            if (evaluate_suitless(high, low) > score) cards_that_beat_us++;
        }
    }
    int flushes_beat_us = 0;
    int straight_draws = 0;
    bool flush_draw = false;

    bool flush_possible = false;
    int suit_count[4] = {};
    for (Card &c: community_cards) {
        if(++suit_count[c.suit] >= 3) flush_possible = true;
    }
    std::vector<Card> suited;
    if (flush_possible) {
        if (score < 5000000) {
            flushes_beat_us = 10;
            if (community_cards.size() < 5 && suited.size() == 4) flush_draw = true;
        }
        else if (score < 6000000){
            int high_suit = score - 5000000;
            int community_suits = 0;
            for (Card &c: suited) {
                if (c.rank > high_suit) community_suits++;
            }
            flushes_beat_us = 14 - high_suit - community_suits;
        }
    }
    if (score < 4000000) {
        // count straight draws
    }
    return GameState(cards_that_beat_us, flushes_beat_us, straight_draws, flush_draw, pre_raises, post_raises);
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
            return showdown(probability);
        }
        player_turn = 0;
        last_aggressor = 0;
    }
    if (last_aggressor == -1) last_aggressor = player_turn;
    Player &curr_player = players[player_turn];
    int nxt_player = (player_turn + 1) % NUM_PLAYERS;
    if (curr_player.folded) return dfs(probability, last_aggressor, player_turn + 1);
    
    auto state = calc_gamestate(player_turn); // work on this

    float total_ev = 0;
    for (int i = 0; i < NUM_ACTIONS; i++) {
        int to_call = current_bet - curr_player.bet_made;
        if (i == FOLD && to_call) {
            // if main character, don't dfs, calculate ev right here
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
        else if (i == RAISE && pre_raises < 3 && post_raises < 2) {
            community_cards.empty() ? pre_raises++ : post_raises++;
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
            community_cards.empty() ? pre_raises-- : post_raises--;
        }
    }
    

    while (undo_community_cards--) {
        community_cards.pop_back();
        top_card_index++;
    }

    return total_ev;
}



int main() {
    Game game;
}
