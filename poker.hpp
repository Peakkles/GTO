#ifndef _POKER_HPP
#define _POKER_HPP

#include <map>
#include <vector>
#include <string>
#include <fstream>


#define NUM_PLAYERS 3
#define INITIAL_CHIPS 200

enum Suit : int {
    HEART,
    SPADE,
    DIAMOND,
    CLUB
};
const std::string SUIT_NAME[] = { "H", "S", "D", "C" };

struct Card {
    int rank;
    Suit suit;

    std::string to_string() {
        return "[" + std::to_string(rank) + SUIT_NAME[suit] + "]";
    }
};

struct GameState {
    int rank_combos_that_beat_you; // 0 to (14 choose 2 - 1)
    int flush_highs_that_beat_you; // 0 to 10
    int straight_draws; // 0,1,or 2 -> same as 0,4,or 8 outs
    bool flush_draw;
    int preflop_raises; // 0-3
    int post_raises; // 0-2

    /* if preflop, rank_combos is used for high card 2-14
       flush_highs is used for low card 2-14
       straight_draw set to -1
       flush_draw = hole_cards are suited */ 

    GameState(int r_combos, int flushes, int s_draws, bool f_draw, int pre_r, int post_r) 
    : rank_combos_that_beat_you(r_combos), flush_highs_that_beat_you(flushes)
    , straight_draws(s_draws), flush_draw(f_draw), preflop_raises(pre_r), post_raises(post_r) {}

    GameState(int high_card, int low_card, bool suited, int pre_r) 
    : rank_combos_that_beat_you(high_card), flush_highs_that_beat_you(low_card)
    , straight_draws(-1), flush_draw(suited) , preflop_raises(pre_r), post_raises(0) {}

    bool operator< (const GameState &other) const {
        if (rank_combos_that_beat_you != other.rank_combos_that_beat_you) return rank_combos_that_beat_you < other.rank_combos_that_beat_you;
        if (flush_highs_that_beat_you != other.flush_highs_that_beat_you) return flush_highs_that_beat_you < other.flush_highs_that_beat_you;
        if (straight_draws != other.straight_draws) return straight_draws < other.straight_draws;
        if (flush_draw != other.flush_draw) return flush_draw < other.flush_draw;
        if (preflop_raises != other.preflop_raises) return preflop_raises < other.preflop_raises;
        if (post_raises != other.post_raises) return post_raises < other.post_raises;
        return false;
    }
};

enum Action {
    FOLD,
    CALL,
    RAISE,
    NUM_ACTIONS
};

struct Player {
    int chips;
    Card hole_cards[2];
    bool folded;
    int bet_made;
    std::map<GameState, float[NUM_ACTIONS]> strategy;
    std::map<GameState, float[NUM_ACTIONS]> ev;

    Player() {}
    Player(Card c1, Card c2) : chips(INITIAL_CHIPS), hole_cards{c1, c2}
    , folded(false), bet_made(0) {}

    std::string to_string() {
        return (
            "<Chips: " + std::to_string(chips) + " | Hole cards: " +
            hole_cards[0].to_string() + hole_cards[1].to_string() + ">"
        );
    }

    void save_strategy_to_file(const std::string &filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }
        for (const auto &[g, probabilities]: strategy) {
            file << g.rank_combos_that_beat_you << " " 
                 << g.flush_highs_that_beat_you << " "
                 << g.straight_draws << " "
                 << g.flush_draw << " "
                 << g.preflop_raises << " "
                 << g.post_raises << " |";
            for (auto &p: probabilities) file << p << " ";
            file << "|";
            for (auto &p: ev[g]) file << p << " ";
            file << '\n';
        }
        file.close();
    }

};

class Game {
public:
    Player players[NUM_PLAYERS];
    Card deck[52];
    int top_card_index;
    std::vector<Card> community_cards;

    int small_blind;
    int current_bet;
    int pot;
    int pre_raises;
    int post_raises;

    int main_character;

    Game();

    Card draw();
    bool all_folded();
    void run_game();
    GameState calc_gamestate(int player);
    int evaluate_cards(int player);
    float showdown(float probability);
    void default_strategy(Player &p, const GameState &g);

    float dfs(float p, int last_aggressor, int player_turn);
};

#endif
