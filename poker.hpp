#ifndef _POKER_HPP
#define _POKER_HPP

#include <map>

#define NUM_PLAYERS 4

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
    std::map<GameState, float[NUM_ACTIONS]> strategy;
    std::map<GameState, float[NUM_ACTIONS]> ev;

    Player() {}
    Player(Card c1, Card c2) : chips(200), hole_cards{c1, c2} {}

    std::string to_string() {
        return (
            "<Chips: " + std::to_string(chips) + " | Hole cards: " +
            hole_cards[0].to_string() + hole_cards[1].to_string() + ">"
        );
    }

    GameState calc_gamestate() { return GameState{ false }}
};

struct Game {
    Player players[NUM_PLAYERS];
    Card deck[52];
    int top_card_index;
    int player_turn;
    Card community_cards[5];

    int small_blind;

    Game();

    void run_game();

private:
    void dfs();
};

#endif
