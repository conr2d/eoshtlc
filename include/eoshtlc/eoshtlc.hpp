#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eostd/crypto/xxhash.hpp>

using namespace eosio;
using std::string;

class [[eosio::contract]] eoshtlc : public contract {
public:
   using contract::contract;

   struct [[eosio::table]] htlc {
      string contract_name;
      name recipient;
      extended_asset value;
      checksum256 hashlock;
      time_point_sec timelock;
      bool activated = false;

      static uint64_t hash(string s) { return eostd::xxh64(s.data(), s.size()); }
      uint64_t primary_key()const { return hash(contract_name); }

      EOSLIB_SERIALIZE(htlc, (contract_name)(recipient)(value)(hashlock)(timelock)(activated))
   };

   typedef multi_index<"htlc"_n, htlc> htlcs;

   // dummy action to resolve cdt v1.6.1 issue on notify handler
   [[eosio::on_notify("eosio.token::transfer")]]
   void on_eos_transfer(name from, name to, asset quantity, string memo) {
      on_transfer(from, to, quantity, memo);
   }

   [[eosio::on_notify("*::transfer")]]
   void on_transfer(name from, name to, asset quantity, string memo);
   typedef action_wrapper<"transfer"_n, &eoshtlc::on_transfer> transfer_action;

   [[eosio::action]]
   void newcontract(name owner, string contract_name, name recipient, extended_asset value, checksum256 hashlock, time_point_sec timelock);

   [[eosio::action]]
   void withdraw(name owner, string contract_name, checksum256 preimage);

   [[eosio::action]]
   void cancel(name owner, string contract_name);
};
