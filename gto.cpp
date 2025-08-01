// map gamestates to Action probabilites
// players start with random maps
    // in genetic algo:
        // simulate games
        // keep top performers
        // introduce slightly different maps and 1 random map
        // repeat
    // in gto:
        // simulate game tree
        // calculate regret
        // update map
        // repeat with different players

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <random>
#include <array>
#include <fstream>
#include <sstream>

using namespace std;
auto rng = std::default_random_engine{ static_cast<unsigned>(chrono::system_clock::now().time_since_epoch().count()) };
const int NUM_PLAYERS = 4;

enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };
const string SUITS[] = { "H", "D", "C", "S" };
const string RANKS[] = { "2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A" };


struct Card {
    int rank; // 0 to 12 (2 to A)
    Suit suit;

    bool operator==(const Card &other) const {
        return (rank == other.rank && suit == other.suit);
    }

    bool operator< (const Card &other) const {
        if (rank != other.rank) return rank < other.rank;
        return suit < other.suit;
    }

    string toString() const {
        return RANKS[rank] + SUITS[suit];
    }
};

class Deck {
public:
    vector<Card> cards;
    Deck() { reset(); }

    void reset() {
        cards.clear();
        for (int r = 0; r < 13; r++) {
            for (int s = 0; s < 4; s++) {
                cards.push_back({ r, static_cast<Suit>(s) });
            }
        }
    }

    void shuffle() {
        
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    Card draw() {
        Card top = cards.back(); cards.pop_back();
        return top;
    }
};

int evaluateBest7CardHand(vector<Card> cards) {
    array<int, 13> rank_count = {};
    array<int, 4> suit_count = {};
    vector<vector<Card>> suit_cards(4);

    for (auto& c : cards) {
        rank_count[c.rank]++;
        suit_count[c.suit]++;
        suit_cards[c.suit].push_back(c);
    }

    // Sort cards by rank descending
    sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.rank > b.rank;
    });

    // --------- Check for Flush & Straight Flush ---------
    for (int s = 0; s < 4; ++s) {
        if (suit_count[s] >= 5) {
            auto& suited = suit_cards[s];
            sort(suited.begin(), suited.end(), [](const Card& a, const Card& b) {
                return a.rank > b.rank;
            });

            // Check for straight flush
            vector<int> vals;
            for (auto& c : suited)
                vals.push_back(c.rank);

            // Remove duplicates
            vals.erase(unique(vals.begin(), vals.end()), vals.end());

            for (int i = 0; i <= int(vals.size()) - 5; ++i) {
                bool isStraight = true;
                for (int j = 1; j < 5; ++j) {
                    if (vals[i + j - 1] != vals[i + j] + 1) {
                        isStraight = false;
                        break;
                    }
                }
                if (isStraight) {
                    return 8000000 + vals[i]; // Straight flush
                }
            }

            // Check for A-5 straight flush
            if (find(vals.begin(), vals.end(), 12) != vals.end() &&
                find(vals.begin(), vals.end(), 3) != vals.end() &&
                find(vals.begin(), vals.end(), 2) != vals.end() &&
                find(vals.begin(), vals.end(), 1) != vals.end() &&
                find(vals.begin(), vals.end(), 0) != vals.end()) {
                return 8000000 + 3;
            }

            // Regular flush
            return 5000000 + suited[0].rank;
        }
    }

    // --------- Rank histogram for 4/3/2-of-a-kinds ---------
    vector<int> ranks;
    for (int r = 12; r >= 0; --r)
        for (int i = 0; i < rank_count[r]; ++i)
            ranks.push_back(r);

    vector<pair<int, int>> count_rank;  // {count, rank}
    for (int r = 0; r < 13; ++r) {
        if (rank_count[r] > 0)
            count_rank.push_back({rank_count[r], r});
    }

    sort(count_rank.begin(), count_rank.end(), [](auto& a, auto& b) {
        if (a.first != b.first) return a.first > b.first;
        return a.second > b.second;
    });

    int first = count_rank[0].second;
    int first_count = count_rank[0].first;
    int second = count_rank.size() > 1 ? count_rank[1].second : -1;
    int second_count = count_rank.size() > 1 ? count_rank[1].first : 0;

    if (first_count == 4)
        return 7000000 + (first + 2) * 100 + (second + 2);
    if (first_count == 3 && second_count >= 2)
        return 6000000 + (first + 2) * 100 + (second + 2);
    if (first_count == 3)
        return 3000000 + (first + 2) * 10000 + ranks[0] * 100 + ranks[1];
    if (first_count == 2 && second_count == 2)
        return 2000000 + (first + 2) * 100 + (second + 2) * 10 + ranks[0];
    if (first_count == 2)
        return 1000000 + (first + 2) * 10000 + ranks[0] * 100 + ranks[1] * 10 + ranks[2];

    // --------- Check for Straight ---------
    vector<int> unique_ranks;
    for (int r = 12; r >= 0; --r)
        if (rank_count[r]) unique_ranks.push_back(r);

    for (int i = 0; i <= int(unique_ranks.size()) - 5; ++i) {
        bool straight = true;
        for (int j = 1; j < 5; ++j) {
            if (unique_ranks[i + j - 1] != unique_ranks[i + j] + 1) {
                straight = false;
                break;
            }
        }
        if (straight) return 4000000 + unique_ranks[i];
    }

    // A-5 low straight
    if (rank_count[12] && rank_count[3] && rank_count[2] &&
        rank_count[1] && rank_count[0])
        return 4000000 + 3;

    // --------- High Card ---------
    return (ranks[0] + 2) * 100000 + (ranks[1] + 2) * 10000 + (ranks[2] + 2) * 1000 + (ranks[3] + 2) * 100 + (ranks[4] + 2);
}

// takes in 7 card hand returns value of best 5 card hand
int bestHandValue(const vector<Card>& cards) {
    return evaluateBest7CardHand(cards);
}

enum Equity {
    DESTROYED,  // 0-19
    LOW,        // 20-39
    NEUTRAL,    // 40-59
    HIGH,       // 60-79
    DOMINATION  // 80-100
};

enum Action {
    FOLD,
    CALL, // call = check
    RAISE_50,
    RAISE_100,
    ALL_IN
};

struct Gamestate {
    Equity equity_vs_monster;
    Equity equity_vs_strong;
    Equity equity_vs_medium;
    Equity equity_vs_weak;
    int position; // number of people who act after you reach betting round
    int post_raises;
    int pre_raises;
    bool multiway;

    bool operator< (const Gamestate other) const {
        if (equity_vs_monster != other.equity_vs_monster) return equity_vs_monster < other.equity_vs_monster;
        if (equity_vs_strong != other.equity_vs_strong) return equity_vs_strong < other.equity_vs_strong;
        if (equity_vs_medium != other.equity_vs_medium) return equity_vs_medium < other.equity_vs_medium;
        if (equity_vs_weak != other.equity_vs_weak) return equity_vs_weak < other.equity_vs_weak;
        if (position != other.position) return position < other.position;
        if (post_raises != other.post_raises) return post_raises < other.post_raises;
        if (pre_raises != other.pre_raises) return pre_raises < other.pre_raises;
        return multiway < other.multiway;
    }
};

void normalize_probability(vector<float> &probabilities) {
    float sum = 0.0000001;
    for (float &prob: probabilities) {
        if (prob < 0) prob = 0;
        sum += prob;
    }
    for (float &prob: probabilities) prob /= sum;
}


vector<float> random_probabilities(Gamestate g) {
    if (g.pre_raises >= 4 || g.post_raises >= 4) return {(float) 1/2, (float) 1/2};
    vector<float> probabilities = {float (rand() % 100) / 100, float (rand() % 100) / 100, float (rand() % 100) / 100, float (rand() % 100) / 100, float (rand() % 100) / 100, float (rand() % 100) / 100};
    normalize_probability(probabilities);
    return probabilities;
}

class Player {
    public:
        vector<Card> hole_cards;
        map<Gamestate, vector<float>> strategy;
        int stack_size;
        int position;
        int bet_made;
        bool folded;
        Gamestate gamestate;

        int total_profit;

        Action decideAction () {
            if (strategy.find(gamestate) == strategy.end()) strategy[gamestate] = random_probabilities(gamestate);

            float rng = float (rand() % 100) / 100;
            float cumulative = 0;
            for (int i = 0; i < strategy[gamestate].size(); ++i) {
                float probability = strategy[gamestate][i];
                Action action = static_cast<Action>(i);
                cumulative += probability;
                if (rng < cumulative) {
                    return action;
                }
            }
            return ALL_IN;
        }
};


vector<pair<Card, Card>> MONSTERS;
vector<pair<Card, Card>> STRONG;
vector<pair<Card, Card>> MEDIUM;
vector<pair<Card, Card>> WEAK;

void bucket_hands() {
    Deck deck;
    for (int i = 0; i < deck.cards.size()-1; ++i) {
        Card c1 = deck.cards[i];
        for (int j = i+1; j < deck.cards.size(); ++j) {
            Card c2 = deck.cards[j];
            if (c1.rank == c2.rank && c1.rank >= 9) {
                MONSTERS.push_back({c2, c1});
            }
            else if (c1.rank >= 10 && c2.rank >= 12) {
                MONSTERS.push_back({c2, c1});
            }
            else if (c1.rank >= 8 && c2.rank >= 8) {
                STRONG.push_back({c2, c1});
            }
            else if (c1.rank == c2.rank && c1.rank >= 5) {
                STRONG.push_back({c2, c1});
            }
            else if (c2.rank == 12 && c1.suit == c2.suit) {
                STRONG.push_back({c2, c1});
            }
            else if (c2.rank - c1.rank == 1 && c1.suit == c2.suit && c1.rank >= 3) {
                STRONG.push_back({c2, c1});
            }
            else if (c2.rank - c1.rank <= 1 || c1.suit == c2.suit ) {
                MEDIUM.push_back({c2, c1});
            }
            else {
                WEAK.push_back({c2, c1});
            }

        }    
    }
}



tuple<Equity, Equity, Equity, Equity> get_equities(vector<Card> &hole_cards, vector<Card> &community_cards) {
    auto calculate_equity = [&](vector<Card> &hero, vector<Card> &villain, vector<Card> &community_cards) {
        int score = 0;
        int total = 20;
        for (int i = 0; i < 10; ++i) {
            vector<Card> community = community_cards;
            while (community.size() < 5) {
                int rng = rand() % 52;
                Card draw = {(rng / 4), static_cast<Suit>(rng % 4)};
                if (find(hero.begin(), hero.end(), draw) != hero.end() || find(villain.begin(), villain.end(), draw) != villain.end() || find(community.begin(), community.end(), draw) != community.end()) {
                    continue;
                }
                community.push_back(draw);
            }
            vector<Card> v = villain;
            v.insert(v.end(), community.begin(), community.end());
            int v_score = bestHandValue(v);

            vector<Card> h = hero;
            h.insert(h.end(), community.begin(), community.end());
            int h_score = bestHandValue(h);
            if (h_score > v_score) {
                score += 2;
            }
            else if (h_score == v_score) {
                score += 1;
            }
        }
        return (float) score / total;
    };
    set<Card> set = {community_cards.begin(), community_cards.end()};
    set.insert(hole_cards[0]);
    set.insert(hole_cards[1]);
    float equity = 0;
    shuffle(MONSTERS.begin(), MONSTERS.end(), rng);
    shuffle(STRONG.begin(), STRONG.end(), rng);
    shuffle(MEDIUM.begin(), MEDIUM.end(), rng);
    shuffle(WEAK.begin(), WEAK.end(), rng);
    int samples = 0;
    for (int i = 0; i <= 25; ++i) {
        auto& [c1, c2] = MONSTERS[i];
        if (set.count(c1) || set.count(c2)) continue;
        samples++;
        vector<Card> villain = {c1, c2};
        equity += calculate_equity(hole_cards, villain, community_cards);
    }
    equity /= samples;
    Equity monster = static_cast<Equity> (int (equity / 0.2));

    equity = 0;
    samples = 0;
    for (int i = 0; i <= 25; ++i) {
        auto& [c1, c2] = STRONG[i];
        if (set.count(c1) || set.count(c2)) continue;
        samples++;
        vector<Card> villain = {c1, c2};
        equity += calculate_equity(hole_cards, villain, community_cards);
    }
    equity /= samples;
    Equity strong = static_cast<Equity> (int (equity / 0.2));

    equity = 0;
    samples = 0;
    for (int i = 0; i <= 25; ++i) {
        auto& [c1, c2] = MEDIUM[i];
        if (set.count(c1) || set.count(c2)) continue;
        samples++;
        vector<Card> villain = {c1, c2};
        equity += calculate_equity(hole_cards, villain, community_cards);
    }
    equity /= samples;
    Equity medium = static_cast<Equity> (int (equity / 0.2));

    equity = 0;
    samples = 0;
    for (int i = 0; i <= 25; ++i) {
        auto& [c1, c2] = WEAK[i];
        if (set.count(c1) || set.count(c2)) continue;
        samples++;
        vector<Card> villain = {c1, c2};
        equity += calculate_equity(hole_cards, villain, community_cards);
    }
    equity /= samples;
    Equity weak = static_cast<Equity> (int (equity / 0.2));

    return {monster, strong, medium, weak};
}

Gamestate get_gamestate(vector<Player> &players, int player, vector<Card> &community_cards, int pre_raises, int post_raises) {
    Gamestate g;
    int position = 0;
    int in_pot = 0;
    for (int i = 0; i < players.size(); ++i) {
        if (i > player && !players[i].folded) position++;
        if (!players[i].folded) in_pot++;
    }
    bool multiway = in_pot > 2;
    auto [monster, strong, medium, weak] =  get_equities(players[player].hole_cards, community_cards);

    g.equity_vs_monster = monster;
    g.equity_vs_strong = strong;
    g.equity_vs_medium = medium;
    g.equity_vs_weak = weak;

    g.multiway = multiway;
    g.position = position;
    g.post_raises = post_raises;
    g.pre_raises = pre_raises;

    return g;
}

void bettingRound(vector<Player>& players, vector<Card> community_cards, int& pre_r, int& post_r, int& pot, int& currentBet, int startingIndex = 0) {
    const int numPlayers = players.size();
    int current = startingIndex;
    int lastAggressor = -1;

    // Count active players & assign gamestates
    int active = 0;
    for (int i = 0; i < numPlayers; ++i) {
        if (!players[i].folded && players[i].stack_size > 0) {
            active++;
            players[i].gamestate = get_gamestate(players, i, community_cards, pre_r, post_r);
        }
    }
    if (active <= 1) return;

    bool bettingComplete = false;
    int consecutiveCalls = 0;

    while (!bettingComplete) {
        Player& p = players[current];

        if (!p.folded && p.stack_size > 0) {
            int toCall = currentBet - p.bet_made;
            Action action = p.decideAction();

            if (action == FOLD && toCall > 0) {
                p.folded = true;
                active--;
                if (active <= 1) break;
                consecutiveCalls++; // counts as a "non-raise" action
            }
            else if (action == CALL || (action == FOLD && toCall == 0)) {
                int callAmt = min(toCall, p.stack_size);
                p.stack_size -= callAmt;
                p.bet_made += callAmt;
                pot += callAmt;
                consecutiveCalls++;
            }
            else if (action == ALL_IN) {
                int allInAmt = p.stack_size;
                p.bet_made += allInAmt;
                p.stack_size = 0;
                pot += allInAmt;
                currentBet = max(currentBet, p.bet_made);
                lastAggressor = current;
                consecutiveCalls = 1;

                if (community_cards.size() == 0) pre_r = 4;
                else post_r = 4;
            }
            else { // raise
                float pct = action == RAISE_50 ? 0.5 : 1.0;
                int raiseAmt = static_cast<int>((pot + toCall) * pct);
                int totalBet = toCall + raiseAmt;
                totalBet = min(totalBet, p.stack_size);

                p.stack_size -= totalBet;
                p.bet_made += totalBet;
                pot += totalBet;
                currentBet = max(currentBet, p.bet_made);

                lastAggressor = current;
                consecutiveCalls = 1;

                if (community_cards.size() == 0) pre_r++;
                else post_r++;
            }
        } else {
            consecutiveCalls++; // folded or all-in
        }

        // Betting ends if we've looped through all players with no new raise
        if (consecutiveCalls >= numPlayers) break;
        if (lastAggressor == current) break;

        current = (current + 1) % numPlayers;
    }
}

void showdown(vector<Player> &players, vector<Card> &community, int &pot, Deck &deck) {
    vector<int> winners;
    int best_score = -1;
    for (int i = 0; i < players.size(); ++i) {
        if (players[i].folded) continue;
        community.push_back(players[i].hole_cards[0]);
        community.push_back(players[i].hole_cards[1]);
        int score = bestHandValue(community);
        if (score > best_score) {
            winners.clear();
            winners.push_back(i);
            best_score = score;
        }
        else if (score == best_score) {
            winners.push_back(i);
        }
        community.pop_back(); community.pop_back();
    }
    
    for (int winner: winners) {
        players[winner].stack_size += pot/winners.size();
    }

    for (auto &p: players) {
        p.total_profit += p.stack_size - 100;
        p.stack_size = 100;
        p.bet_made = 0;
        p.folded = false;
        deck.cards.push_back(p.hole_cards.back()); p.hole_cards.pop_back();
        deck.cards.push_back(p.hole_cards.back()); p.hole_cards.pop_back();
    }
    deck.cards.push_back(community.back()); community.pop_back();
    deck.cards.push_back(community.back()); community.pop_back();
    deck.cards.push_back(community.back()); community.pop_back();
    deck.cards.push_back(community.back()); community.pop_back();
    deck.cards.push_back(community.back()); community.pop_back();

    pot = 0;
}

map<Gamestate, vector<float>> get_modified_strategy(map<Gamestate, vector<float>> &strategy) {
    map<Gamestate, vector<float>> new_strategy;
    for (auto [gamestate, probabilities]: strategy) {

        if (gamestate.pre_raises >= 4 || gamestate.post_raises >= 4) {
            new_strategy[gamestate] = {(float) 1/2, (float) 1/2};
        }
        else {
            for (float & prob: probabilities) {
                prob += (float) (rand() % 11) / 100 - 0.05;
            }
            normalize_probability(probabilities);

            new_strategy[gamestate] = probabilities;
        }
    }
    return new_strategy;
}

void save_strategy_to_file(const map<Gamestate, vector<float>>& strategy, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file for writing: " << filename << endl;
        return;
    }

    for (const auto& [g, probs] : strategy) {
        file << g.equity_vs_monster << " "
             << g.equity_vs_strong << " "
             << g.equity_vs_medium << " "
             << g.equity_vs_weak << " "
             << g.position << " "
             << g.post_raises << " "
             << g.pre_raises << " "
             << g.multiway << " ";

        for (float p : probs) file << p << " ";
        file << '\n';
    }

    file.close();
}

map<Gamestate, vector<float>> load_strategy_from_file(const string& filename) {
    map<Gamestate, vector<float>> strategy;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return strategy;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        int eq_monster, eq_strong, eq_medium, eq_weak;
        int position, post_raises, pre_raises, multiway_int;

        iss >> eq_monster >> eq_strong >> eq_medium >> eq_weak
            >> position >> post_raises >> pre_raises >> multiway_int;

        vector<float> probs;
        float prob;
        while (iss >> prob) {
            probs.push_back(prob);
        }

        Gamestate g = {
            static_cast<Equity>(eq_monster),
            static_cast<Equity>(eq_strong),
            static_cast<Equity>(eq_medium),
            static_cast<Equity>(eq_weak),
            position,
            post_raises,
            pre_raises,
            static_cast<bool>(multiway_int)
        };

        strategy[g] = probs;
    }

    return strategy;
}

map<Gamestate, vector<float>> initial_strategy() {
    map<Gamestate, vector<float>> strat;
    for (int a = DESTROYED; a <= DOMINATION; ++a) {
        for (int b = DESTROYED; b <= DOMINATION; ++b) {
            for (int c = DESTROYED; c <= DOMINATION; ++c) {
                for (int d = DESTROYED; d <= DOMINATION; ++d) {
                    for (int pos = 0; pos < NUM_PLAYERS; ++pos) {
                        for (int post_r = 0; post_r < 5; ++post_r) {
                            for (int pre_r = 0; pre_r < 5; ++pre_r) {
                                for (bool multiway: {false, true}) {
                                    Gamestate g = {
                                        static_cast<Equity>(a),
                                        static_cast<Equity>(b),
                                        static_cast<Equity>(c),
                                        static_cast<Equity>(d),
                                        pos,
                                        post_r,
                                        pre_r,
                                        multiway
                                    };
                                    vector<float> p;
                                    if (a + b + c <= 6 && (multiway || post_r >= 2 || pre_r >= 2)) p = {1.0};
                                    else if (a + b + c <= 6) p = {0.5, 0, 0.5};
                                    else if (a + b + c <= 7 && (!multiway && post_r <= 2 && pre_r <= 2)) p = {0, 0.5, 0.5};
                                    else if (a + b + c <= 7 && pre_r <= 2 && post_r <= 2) p = {0.5, 0.5}; 
                                    else if (a + b + c <= 8 && pre_r <= 3 && post_r <= 3) p = {0.5, 0.5};
                                    else if (a + b + c <= 8) p = {0.9, 0.1};
                                    else if (a + b + c <= 10) p = {0, 0.30, 0.30, 0.40};
                                    else p = {0, 0.2, 0.2, 0.6};

                                    strat[g] = p;

                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return strat;
}

int main() {
    string filename = "strategy.txt";
    map<Gamestate, vector<float>> strat = load_strategy_from_file(filename);
    bucket_hands();
    Deck deck;
    deck.shuffle();
    vector<Card> community;
    vector<Player> players(NUM_PLAYERS);
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        players[i].stack_size = 100;
        players[i].position = NUM_PLAYERS - 1 - i;
        players[i].hole_cards.push_back(deck.draw());
        players[i].hole_cards.push_back(deck.draw());
        players[i].strategy = strat;
    }
    // players[0].strategy = initial_strategy();
    
    
    for (int gen = 0; gen < 100; ++gen) {
        for (int round = 0; round < 50; ++round) {
            // blinds
            players[0].bet_made = 1;
            players[0].stack_size -= 1;
            players[1].bet_made = 2;
            players[1].stack_size -= 2;
            int current_bet = 2;
            int pot = 3;
            int pre_raises = 0;
            int post_raises = 0;

            bettingRound(players, community, pre_raises, post_raises, pot, current_bet, 2);

            community.push_back(deck.draw());
            community.push_back(deck.draw());
            community.push_back(deck.draw());

            bettingRound(players, community, pre_raises, post_raises, pot, current_bet, 0);
            community.push_back(deck.draw());
            bettingRound(players, community, pre_raises, post_raises, pot,  current_bet, 0);
            community.push_back(deck.draw());
            bettingRound(players, community, pre_raises, post_raises, pot,  current_bet, 0);

            showdown(players, community, pot, deck);
            deck.shuffle();
            
            for (int i = 0; i < NUM_PLAYERS; ++i) {
                players[i].hole_cards.push_back(deck.draw());
                players[i].hole_cards.push_back(deck.draw());
            }
            shuffle(players.begin(), players.end(), rng);

        }
        sort(players.begin(), players.end(), [](const Player a, const Player b){ return a.total_profit > b.total_profit;});
        if (gen % 5 == 4) save_strategy_to_file(players[0].strategy, filename);
        cout << gen << '\n';

        players[3].strategy = get_modified_strategy(players[0].strategy);
        players[2].strategy = get_modified_strategy(players[1].strategy);
        for (Player &p: players) {
            p.total_profit = 0;
        }
        shuffle(players.begin(), players.end(), rng);

    }
}
