#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/crypto.h>
#include <string>

using namespace eosio;
using namespace std;

#define EOS_SYMBOL S(4, EOS)
#define TOKEN_CONTRACT N(eosio.token)
#define LOG_CONTRACT N(eosslotloger)

class slot_machine : public contract {
public:
        slot_machine(account_name self)
                : contract(self),
        global(_self, _self),
        items(_self, _self) {}
    void init();
    void transfer(account_name from, account_name to, asset quantity, std::string memo);
    void reveal(uint64_t id, public_key& pub_key, signature& sig, std::string block_id);

private:
    // @abi table global i64
    struct global {
        uint64_t    id = 0;
        uint64_t    total_count; // 总共玩过多少次
        bool        started;
        asset       jackPot;
        uint64_t    finished_count; // 总共玩过多少次
        uint64_t primary_key() const { return id; }
        EOSLIB_SERIALIZE(global, (id)(total_count)(started)(jackPot)(finished_count))
    };

    // @abi table item i64
    struct item {
        uint64_t id;
        account_name player;
        checksum256 seed;
        asset bet_amount;
        uint64_t primary_key() const { return id; }
        account_name get_player() const { return player;}
        EOSLIB_SERIALIZE(item, (id)(player)(seed)(bet_amount))
    };

    typedef eosio::multi_index<N(item), item, indexed_by<N(player), const_mem_fun<item, account_name, &item::get_player>>> item_index;
    item_index items;

    const int Reelstrips[50][3]  = {
         {2,4,7},{4,7,7},{7,6,7},
         {7,7,0},{7,7,6},{4,0,6},
         {8,6,5},{6,6,7},{4,7,7},
         {7,7,7},{7,0,0},{7,2,6},
         {6,3,5},{5,7,0},{5,0,0},
         {4,7,7},{5,7,7},{7,5,4},
         {7,5,3},{4,5,0},{3,0,6},
         {7,6,0},{5,6,4},{0,7,8},
         {7,7,5},{7,6,3},{7,0,7},
         {7,0,0},{5,5,5},{5,0,6},
         {6,5,7},{3,7,7},{7,6,7},
         {6,6,0},{6,7,7},{6,4,5},
         {7,0,6},{6,0,6},{3,7,7},
         {1,0,0},{5,7,6},{7,7,2},
         {7,6,1},{7,8,6},{7,3,4},
         {7,1,2},{2,4,5},{6,7,7},
         {6,6,8},{7,5,6}
    };

    const int Jackpot[130] = {
        69, 99, 40, 33, 39, 118, 126, 73, 112, 46, 67, 83,
        35, 66, 64, 76, 95, 129, 120, 8, 96, 2, 21, 14, 29,
        18, 123, 15, 3, 55, 125, 52, 102, 13, 68, 63, 22, 5,
        20, 81, 37, 101, 30, 58, 16, 45, 89, 78, 47, 24, 54,
        85, 32, 36, 71, 92, 50, 11, 57, 1, 124, 79, 114, 53,
        93, 72, 25, 113, 108, 51, 26, 90, 10, 49, 75, 100, 28,
        12, 84, 107, 7, 4, 104, 127, 41, 70, 87, 17, 116, 98, 94,
        6, 80, 59, 44, 38, 31, 19, 103, 27, 65, 106, 105, 110,
        115, 48, 109, 61, 91, 23, 117, 77, 43, 86, 88, 60, 121, 119,
        128, 0, 111, 74, 82, 9, 34, 97, 42, 62, 56, 122
    };

    const uint64_t BonusJackpot[3] =  { 100000 , 40000  , 10000 };

    const uint64_t Bonus[9] = { 1200 , 1000, 800 , 500 , 200 ,50, 20, 5 , 0 };

    const char* fmtReel = "\"reel%d\":{\"pos\": %d , \"number\": %d }";

    const char* fmtJackPot = "\"jackpot\":{ \"point\":%d , \"pos\":%d , \"number\": %d }";

    typedef eosio::multi_index<N(global), global> global_index;
    global_index global;


    uint8_t char2int(char input);

    void string2seed(const string& str, checksum256& seed);

    void split(std::string str, std::string splitBy, std::vector<std::string>& tokens)
    {
        /* Store the original string in the array, so we can loop the rest
         * of the algorithm. */
        tokens.push_back(str);

        // Store the split index in a 'size_t' (unsigned integer) type.
        size_t splitAt;
        // Store the size of what we're splicing out.
        size_t splitLen = splitBy.size();
        // Create a string for temporarily storing the fragment we're processing.
        std::string frag;
        // Loop infinitely - break is internal.
        while(true)
        {
            /* Store the last string in the vector, which is the only logical
             * candidate for processing. */
            frag = tokens.back();
            /* The index where the split is. */
            splitAt = frag.find(splitBy);
            // If we didn't find a new split point...
            if(splitAt == string::npos)
            {
                // Break the loop and (implicitly) return.
                break;
            }
            /* Put everything from the left side of the split where the string
             * being processed used to be. */
            tokens.back() = frag.substr(0, splitAt);
            /* Push everything from the right side of the split to the next empty
             * index in the vector. */
            tokens.push_back(frag.substr(splitAt+splitLen, frag.size()-(splitAt+splitLen)));
        }
    }
};
