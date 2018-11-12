#include <string>
#include <vector>
#include <eosiolib/transaction.hpp>
#include "eosslot.hpp"
#include "rand.hpp"
#include <stdio.h>

void slot_machine::init() {
    require_auth(_self);

    auto it = global.begin();
    if (it != global.end()) {
        global.modify( it, 0, [&]( auto& g ) {
            g.id = 0;
            g.started = true;
        });
    }else{
        global.emplace(_self, [&](auto& g){
            g.id = 0;
            g.total_count = 0;
            g.jackPot = asset( 0LL, EOS_SYMBOL );
            g.started = true;
        });
    }
}



void slot_machine::transfer(account_name from, account_name to, asset quantity, std::string memo) {
    require_auth(from);
    // 允许自己给别人转账已经
    if (from == _self || to != _self) {
      return;
    }

    if (quantity.symbol == EOS_SYMBOL) {
        std::vector<std::string> inputs;
        // seed:b2cd3ea401d963b6fa75dd776a6397b77aef6623f9fabd4aa5c56f695b309af7
        split(memo, ":", inputs);

        if (inputs.size() != 2 || inputs[0] != "seed" ){
            return;
        }

        // have seed in memo
        auto global_itr = global.begin();
        eosio_assert(global_itr->started== 1, "slot machine not start yet." );
        eosio_assert(quantity.symbol == EOS_SYMBOL, "only EOS allowed" );
        eosio_assert(quantity.is_valid(), "Invalid token transfer");
        eosio_assert(quantity.amount >= 1000, "Bet must large than 0.1 EOS");
        eosio_assert(inputs[1].size() == 64, "invalid seed" );

        checksum256 seed;
        string2seed(inputs[1], seed);

        //auto player_index = items.template get_index<N(player)>();
        //eosio_assert(player_index.find( from ) == player_index.end(), "ItemAlreadyStarted");

        auto id = global_itr->total_count+1;

        items.emplace(_self, [&](auto& item){
            item.id = id;
            item.player = from;
            item.seed = seed;
            item.bet_amount = quantity;
        });

        action(
               permission_level{_self, N(active)},
               LOG_CONTRACT, N(play),
               make_tuple(id, from, quantity, seed , current_time()))
        .send();

        global.modify(global_itr, 0, [&](auto &g) {
          g.total_count = g.total_count + 1;
        });
    }
}


// For fair, bet queue size should always small than hash queue size.
void slot_machine::reveal(uint64_t id, public_key& pub_key, signature& sig, std::string block_id){
    require_auth(_self);
    auto global_itr = global.begin();
    eosio_assert(global_itr->started== 1, "slot machine not start yet." );

    auto items_itr = items.find(id);
    eosio_assert(items_itr != items.end(), "game item not found.");

    account_name player = items_itr->player;
    asset bet_amount = items_itr->bet_amount;
    checksum256 seed = items_itr->seed;
    char bufDigest[512] = { 0 };
    //check sig
    uint32_t used = 0 ;
    string sid = to_string(id );
    std::memcpy( bufDigest  , sid.c_str(),sid.size () );
    used += sid.size () ;
    std::memcpy( bufDigest+ used , &seed, sizeof( checksum256 ) );
    used += sizeof( checksum256 );
    std::memcpy( bufDigest+ used  ,block_id.c_str(), block_id.size() );
    used += block_id.size() ;
    checksum256 digest = { 0 };
    sha256( bufDigest,  used , &digest);
    assert_recover_key(&digest , (const char*)sig.data , sizeof(sig), pub_key.data, sizeof(pub_key));
    //
    //mt19937::result_type seed = time(0);

    items.erase(items_itr);
    std::string logContract("{") ;
    char buf[128] = {0};
    int  shots[3] = {0};
    const char *p64 = reinterpret_cast<const char *>(&sig.data);
    int offset = 4;
    uint32_t r = *(uint32_t*) &p64[offset+ 1*8] ;
    uint32_t rJackpot = *(uint32_t*) &p64[offset+ 2*8] ;

    from_seed(r) ;

    //auto jackpot_rand = std::bind(std::uniform_int_distribution<int>(0,129),
    //                       mt19937(seedJackPot));
    for(int i = 0; i< 3; i++) {
        //uint64_t r = *(uint64_t*) &p64[offset+ i*8] ;//% (49 + 1 - 1)) + 1;
        int r = rand_u32()% 50 ;
        shots[i] = Reelstrips[r][i];
        sprintf(buf ,fmtReel,i,r,shots[i]  );
        logContract += std::string(buf);
        logContract +=",";
    }
    int judgerVal = judger ( shots );
    if ( judgerVal >= 0 ){
        auto bonus = uint64_t( Bonus [ judgerVal ] ) ;
        bool super = false;
        if ( bonus == 0 ){
            from_seed(rJackpot) ;
            int r = rand_u32()% 130;
            auto jackpot  = Jackpot[r];
            if ( jackpot < 5 ){
                bonus = BonusJackpot[ 0 ];
                super = true;
            }else if (jackpot < 50){
                bonus =  BonusJackpot[ 1 ];
            }else {
                bonus = BonusJackpot[2];
            }
            sprintf(buf ,fmtJackPot, 1 ,r , bonus );
            logContract += std::string(buf);

        }else{
            sprintf(buf ,fmtJackPot, 0 , -1, -1);
            logContract += std::string(buf);
        }
        //risk control
        auto reward = bonus * bet_amount.amount/10;

        logContract +="}";
        action(
            permission_level{_self, N(active)},
            LOG_CONTRACT, N(result),
               make_tuple(id, std::string("win"), player, bet_amount, asset(reward, EOS_SYMBOL), pub_key ,sig, seed, logContract,block_id, current_time()))
        .send();

        action(permission_level{_self, N(active)},
                TOKEN_CONTRACT,
                N(transfer),
                make_tuple(_self, player, asset(reward, EOS_SYMBOL), std::string("Congratulations!"))
        ).send();
        // transaction trx;
        // trx.actions.emplace_back(permission_level{_self, N(active)},
        //         TOKEN_CONTRACT,
        //         N(transfer),
        //         make_tuple(_self, player, asset(reward, EOS_SYMBOL), std::string("Congratulations!"))
        // );
        // trx.delay_sec = 1;
        // trx.send(player, _self, false);


    } else {
        sprintf(buf ,fmtJackPot, 0 , -1, -1);
        logContract += std::string(buf);
        logContract += "}";

        action(
            permission_level{_self, N(active)},
            LOG_CONTRACT, N(result),
               make_tuple(id, std::string("lost"), player, bet_amount, asset(0LL, EOS_SYMBOL), pub_key ,sig, seed ,logContract ,block_id, current_time()))
        .send();
    }
    asset ir(bet_amount.amount* 3LL/100LL , EOS_SYMBOL );
    global.modify(global_itr, 0, [&](auto &g) {
          g.jackPot = g.jackPot + ir;
          g.finished_count = g.finished_count + 1;
    });
}


/*
 * The Mersenne Twister pseudo-random number generator (PRNG)
 *
 * This is an implementation of fast PRNG called MT19937, meaning it has a
 * period of 2^19937-1, which is a Mersenne prime.
 *
 * This PRNG is fast and suitable for non-cryptographic code.  For instance, it
 * would be perfect for Monte Carlo simulations, etc.
 *
 * For all the details on this algorithm, see the original paper:
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/ARTICLES/mt.pdf
 *
 * Written by Christian Stigen Larsen
 * Distributed under the modified BSD license.
 * 2015-02-17, 2017-12-06
 */


// Better on older Intel Core i7, but worse on newer Intel Xeon CPUs (undefine
// it on those).
//#define MT_UNROLL_MORE

/*
 * We have an array of 624 32-bit values, and there are 31 unused bits, so we
 * have a seed value of 624*32-31 = 19937 bits.
 */
static const size_t SIZE   = 624;
static const size_t PERIOD = 397;
static const size_t DIFF   = SIZE - PERIOD;

static const uint32_t MAGIC = 0x9908b0df;

// State for a singleton Mersenne Twister. If you want to make this into a
// class, these are what you need to isolate.
struct MTState {
  uint32_t MT[SIZE];
  uint32_t MT_TEMPERED[SIZE];
  size_t index = SIZE;
};

static MTState state;

#define M32(x) (0x80000000 & x) // 32nd MSB
#define L31(x) (0x7FFFFFFF & x) // 31 LSBs

#define UNROLL(expr) \
  y = M32(state.MT[i]) | L31(state.MT[i+1]); \
  state.MT[i] = state.MT[expr] ^ (y >> 1) ^ (((int32_t(y) << 31) >> 31) & MAGIC); \
  ++i;

static void generate_numbers()
{
  /*
   * For performance reasons, we've unrolled the loop three times, thus
   * mitigating the need for any modulus operations. Anyway, it seems this
   * trick is old hat: http://www.quadibloc.com/crypto/co4814.htm
   */

  size_t i = 0;
  uint32_t y;

  // i = [0 ... 226]
  while ( i < DIFF ) {
    /*
     * We're doing 226 = 113*2, an even number of steps, so we can safely
     * unroll one more step here for speed:
     */
    UNROLL(i+PERIOD);

#ifdef MT_UNROLL_MORE
    UNROLL(i+PERIOD);
#endif
  }

  // i = [227 ... 622]
  while ( i < SIZE -1 ) {
    /*
     * 623-227 = 396 = 2*2*3*3*11, so we can unroll this loop in any number
     * that evenly divides 396 (2, 4, 6, etc). Here we'll unroll 11 times.
     */
    UNROLL(i-DIFF);

#ifdef MT_UNROLL_MORE
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
#endif
  }

  {
    // i = 623, last step rolls over
    y = M32(state.MT[SIZE-1]) | L31(state.MT[0]);
    state.MT[SIZE-1] = state.MT[PERIOD-1] ^ (y >> 1) ^ (((int32_t(y) << 31) >>
          31) & MAGIC);
  }

  // Temper all numbers in a batch
  for (size_t i = 0; i < SIZE; ++i) {
    y = state.MT[i];
    y ^= y >> 11;
    y ^= y << 7  & 0x9d2c5680;
    y ^= y << 15 & 0xefc60000;
    y ^= y >> 18;
    state.MT_TEMPERED[i] = y;
  }

  state.index = 0;
}

extern "C" void from_seed(uint32_t value)
{
  /*
   * The equation below is a linear congruential generator (LCG), one of the
   * oldest known pseudo-random number generator algorithms, in the form
   * X_(n+1) = = (a*X_n + c) (mod m).
   *
   * We've implicitly got m=32 (mask + word size of 32 bits), so there is no
   * need to explicitly use modulus.
   *
   * What is interesting is the multiplier a.  The one we have below is
   * 0x6c07865 --- 1812433253 in decimal, and is called the Borosh-Niederreiter
   * multiplier for modulus 2^32.
   *
   * It is mentioned in passing in Knuth's THE ART OF COMPUTER
   * PROGRAMMING, Volume 2, page 106, Table 1, line 13.  LCGs are
   * treated in the same book, pp. 10-26
   *
   * You can read the original paper by Borosh and Niederreiter as well.  It's
   * called OPTIMAL MULTIPLIERS FOR PSEUDO-RANDOM NUMBER GENERATION BY THE
   * LINEAR CONGRUENTIAL METHOD (1983) at
   * http://www.springerlink.com/content/n7765ku70w8857l7/
   *
   * You can read about LCGs at:
   * http://en.wikipedia.org/wiki/Linear_congruential_generator
   *
   * From that page, it says: "A common Mersenne twister implementation,
   * interestingly enough, uses an LCG to generate seed data.",
   *
   * Since we're using 32-bits data types for our MT array, we can skip the
   * masking with 0xFFFFFFFF below.
   */

  state.MT[0] = value;
  state.index = SIZE;

  for ( uint_fast32_t i=1; i<SIZE; ++i )
    state.MT[i] = 0x6c078965*(state.MT[i-1] ^ state.MT[i-1]>>30) + i;
}

extern "C" uint32_t rand_u32()
{
  if ( state.index == SIZE ) {
    generate_numbers();
    state.index = 0;
  }

  return state.MT_TEMPERED[state.index++];
}

#undef EOSIO_ABI
#define EOSIO_ABI(TYPE, MEMBERS)                                                                                                              \
  extern "C" {                                                                                                                                    \
  void apply(uint64_t receiver, uint64_t code, uint64_t action)                                                                                   \
  {                                                                                                                                               \
    auto self = receiver;                                                                                                                         \
    if (action == N(onerror))                                                                                                                     \
    {                                                                                                                                             \
      eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account");                                        \
    }                                                                                                                                             \
    if ((code == TOKEN_CONTRACT && action == N(transfer)) || (code == self && (action != N(transfer) ))) \
    {                                                                                                                                             \
      TYPE thiscontract(self);                                                                                                                    \
      switch (action)                                                                                                                             \
      {                                                                                                                                           \
        EOSIO_API(TYPE, MEMBERS)                                                                                                                  \
      }                                                                                                                                           \
    }                                                                                                                                             \
  }                                                                                                                                               \
}

// generate .wasm and .wast file
EOSIO_ABI(slot_machine, (transfer)(init)(reveal))
