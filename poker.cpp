#include <algorithm>
#include <iostream>
#include <random>

#include "poker.hpp"

std::random_device rd;
std::default_random_engine rng(rd());
constexpr int RUNOUTS = 1;

Game::Game() {
    top_card_index = 51;
    small_blind = 1;
    main_character = 0;
    pot = 0;
    pre_raises = 0;
    post_raises = 0;
    int idx = 0;
    for (int j = Suit::HEART; j <= Suit::CLUB; j++) {
        for (int i = 2; i <= 14; i++) {
            deck[idx++] = {i, static_cast<Suit>(j)};
        }
    }

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        players[i] = Player(draw(), draw());
    }
}

Card Game::draw() {
    return deck[top_card_index--];
}

void Game::reset_and_deal() {
    top_card_index = 51;
    std::shuffle(deck, deck+52, rng);
    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i].hole_cards[0] = draw();
        players[i].hole_cards[1] = draw();
    }
}

void Game::run_game() {
    // Pre flop first take blinds
    players[0].chips -= small_blind;
    players[0].bet_made += small_blind;
    players[1].chips -= small_blind * 2;
    players[1].bet_made += small_blind * 2;
    pot += small_blind * 3;
    current_bet = small_blind * 2;
    
    for (int num_updates = 1; num_updates <= 60; ++num_updates) {
        for (int games_per_update = 1; games_per_update <= 10000; ++games_per_update) {
            dfs(-1, 0);
            reset_and_deal();
        }
        update_strategy();
        main_character = (main_character + 1) % NUM_PLAYERS;
    }
    players[0].save_strategy_to_file("p0_strat");
    players[1].save_strategy_to_file("p1_strat");
    

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
float Game::showdown() {
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
            return (players[main_character].chips - INITIAL_CHIPS);
        }
    }
    return (players[main_character].chips + (pot / number_of_winners) - INITIAL_CHIPS);
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
    int straight_draws = 0;
    bool flush_draw = false;

    int flush_possible = 0;
    int suit_count[4] = {};
    for (Card &c: community_cards) {
        if(++suit_count[c.suit] >= 3) flush_possible = 1;
    }
    int suit = -1;
    if (++suit_count[p.hole_cards[0].suit] >= 4) suit = p.hole_cards[0].suit;
    if (++suit_count[p.hole_cards[1].suit] >= 4) suit = p.hole_cards[1].suit;

    if (flush_possible) {
        if (score < 5000000) {
            if (community_cards.size() < 5 && suit != -1) flush_draw = true;
        }
    }
    if (score < 4000000) {
        // count straight draws
        int ranks[15];
        ranks[p.hole_cards[0].rank] = 1;
        ranks[p.hole_cards[1].rank] = 1;
        for (Card &c: community_cards) ranks[c.rank] = 1;
        if (ranks[14]) ranks[1] = 1;
        int l = 1; 
        int r = 1;
        int gap = -1;
        while (r <= 14) {
            if (ranks[r] == 0) {
                if (gap != -1) l = r;
                gap = r;
                r++;
            }
            if (r - l == 4) {
                straight_draws++;
                l = gap + 1;
            }
            if (ranks[r]) r++;
        }
    }
    return GameState(cards_that_beat_us, flush_possible, straight_draws, flush_draw, pre_raises, post_raises);
}

void Game::default_strategy(Player &p, const GameState &g) {
    if (pre_raises < 2 && post_raises < 2) {
        p.strategy[g][0] = 0.333333;
        p.strategy[g][1] = 0.333333;
        p.strategy[g][2] = 0.333333;
    }
    else {
        p.strategy[g][0] = 0.5f;
        p.strategy[g][1] = 0.5f;
        p.strategy[g][2] = 0.0f;
    }
    p.ev[g][0] = 0.0f;
    p.ev[g][1] = 0.0f;
    p.ev[g][2] = 0.0f;
}

void Game::update_strategy() {
    Player &p = players[main_character];

    for (auto &[g, evs]: p.ev) {
        double avg_ev = 0;
        int n = (g.preflop_raises >= 2 || g.post_raises >= 2) ? 2 : 3; // hmmmm
        for (int i = 0; i < n; ++i) {
            avg_ev += p.strategy[g][i] * evs[i];
        }
        double sum_pos_regret = 0;
        for (int i = 0; i < n; ++i) {
            auto &e_value = evs[i];
            if (e_value - avg_ev > 0) sum_pos_regret += e_value - avg_ev;
        }
        if (sum_pos_regret == 0) {
            default_strategy(p, g);
        }
        else {
            for (int i = 0; i < n; ++i) {
                float e_value = evs[i];
                if (e_value - avg_ev <= 0) p.strategy[g][i] = 0.0f;
                else p.strategy[g][i] = (e_value - avg_ev) / sum_pos_regret;
            }
        }
        // for (auto &e: evs) e = 0.0f;
    }
    p.ev.clear();
}

float Game::dfs(int last_aggressor, int player_turn) {
    if (all_folded()) {
        float ev = (
            players[main_character].folded
            ? (players[main_character].chips - INITIAL_CHIPS)
            : (players[main_character].chips + pot - INITIAL_CHIPS)
        );
        return ev;
    }
    int undo_community_cards = 0;

    if (last_aggressor == player_turn) {
        if (community_cards.empty()) {
            for (int i = 0; i < 3; ++i)
                community_cards.push_back(draw());
            undo_community_cards = 3;
        }
        else if (community_cards.size() < 5) {
            community_cards.push_back(draw());
            undo_community_cards = 1;
        }
        else {
            return showdown();
        }

        player_turn = 0;
        while (players[player_turn].folded) {
            if(++player_turn >= NUM_PLAYERS) {
                player_turn -= NUM_PLAYERS;
            }
        }
            // player_turn = (player_turn + 1) % NUM_PLAYERS;
        last_aggressor = player_turn;
    }

    // --- If no aggressor yet, set it now ---
    if (last_aggressor == -1)
        last_aggressor = player_turn;

    Player &curr_player = players[player_turn];
    int nxt_player = player_turn + 1 >= NUM_PLAYERS ? 0 : player_turn + 1;

    // --- Skip folded players ---
    if (curr_player.folded)
        return dfs(last_aggressor, nxt_player);

    auto state = calc_gamestate(player_turn);
    if (curr_player.strategy.find(state) == curr_player.strategy.end())
        default_strategy(curr_player, state);

    float total_ev = 0.0f;
    int to_call = current_bet - curr_player.bet_made;

    // --- Loop over actions ---
    for (int rep = 0; rep < RUNOUTS; ++rep) {
    for (int i = 0; i < NUM_ACTIONS; i++) {
        if (i == FOLD && to_call > 0) {
            if (main_character == player_turn) {
                float ev = curr_player.chips - INITIAL_CHIPS;
                curr_player.ev[state][i] += ev;
                total_ev += ev;
                continue;
            }

            curr_player.folded = true;
            float ev = dfs(last_aggressor, nxt_player) * curr_player.strategy[state][i];
            total_ev += ev;
            curr_player.folded = false;
        }
        else if (i == CALL) {
            pot += to_call;
            curr_player.bet_made += to_call;
            curr_player.chips -= to_call;

            float ev = dfs(last_aggressor, nxt_player) * curr_player.strategy[state][i];
            if (main_character == player_turn)
                curr_player.ev[state][i] += ev;
            total_ev += ev;

            pot -= to_call;
            curr_player.bet_made -= to_call;
            curr_player.chips += to_call;
        }
        else if (i == RAISE && pre_raises < 2 && post_raises < 2) {
            bool preflop = community_cards.empty();
            if (preflop) pre_raises++;
            else post_raises++;

            int new_aggressor = player_turn;
            int bet = std::min(int((pot + to_call) * 0.5 + to_call), curr_player.chips);

            pot += bet;
            curr_player.bet_made += bet;
            current_bet += bet - to_call;
            curr_player.chips -= bet;

            float ev = dfs(new_aggressor, nxt_player) * curr_player.strategy[state][i];
            if (main_character == player_turn)
                curr_player.ev[state][i] += ev;
            total_ev += ev;

            pot -= bet;
            curr_player.bet_made -= bet;
            current_bet -= bet - to_call;
            curr_player.chips += bet;

            if (preflop) pre_raises--;
            else post_raises--;
        }
    }
    // std::shuffle(deck, deck + top_card_index + 1, rng);
    }

    // --- Undo dealt community cards ---
    while (undo_community_cards--) {
        community_cards.pop_back();
        top_card_index++;
    }

    return total_ev;
}


int main() {
    Game game;

    game.run_game();
}
