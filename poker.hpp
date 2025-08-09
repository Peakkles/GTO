#ifndef _POKER_HPP
#define _POKER_HPP

#include <map>
#include <vector>

#define NUM_PLAYERS 4
#define INITIAL_CHIPS 200

enum Suit : char {
    HEART,
    SPADE,
    DIAMOND,
    CLUB
};
const std::string SUIT_NAME[] = { "H", "S", "D", "C" };

struct Card {
    char rank;
    Suit suit;

    std::string to_string() {
        return "[" + std::to_string(rank) + SUIT_NAME[suit] + "]";
    }
};

struct GameState {
    bool is_hand_good;
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
    Player(Card c1, Card c2) : chips(INITIAL_CHIPS), hole_cards{c1, c2} {}

    std::string to_string() {
        return (
            "<Chips: " + std::to_string(chips) + " | Hole cards: " +
            hole_cards[0].to_string() + hole_cards[1].to_string() + ">"
        );
    }

    GameState calc_gamestate() { return GameState{ false };}
};

struct Game {
    Player players[NUM_PLAYERS];
    Card deck[52];
    int top_card_index;
    std::vector<Card> community_cards;

    int small_blind;
    int current_bet;
    int pot;

    int main_character;

    Game();

    Card draw();
    bool all_folded();
    void run_game();

private:
    float dfs(float p, int last_aggressor, int player_turn);
};

#endif
