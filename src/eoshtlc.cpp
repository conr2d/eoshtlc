#include <eoshtlc/eoshtlc.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

void eoshtlc::on_transfer(name from, name to, asset quantity, string memo) {
   if (from == _self) return;

   htlcs idx(_self, from.value);
   const auto& it = idx.get(htlc::hash(memo));

   check(!it.activated, "contract is already activated");
   check(it.value == extended_asset{quantity, get_first_receiver()}, "token amount not match: " + it.value.quantity.to_string() + "@" + it.value.contract.to_string());

   idx.modify(it, same_payer, [&](auto& lck) {
      lck.activated = true;
   });
}

void eoshtlc::newcontract(name owner, string contract_name, name recipient, extended_asset value, checksum256 hashlock, time_point_sec timelock) {
   require_auth(owner);

   htlcs idx(_self, owner.value);
   check(idx.find(htlc::hash(contract_name)) == idx.end(), "existing contract name");
   check(timelock > current_time_point(), "the expiration time should be in the future");

   idx.emplace(owner, [&](auto& lck) {
      lck.contract_name = contract_name;
      lck.recipient = recipient;
      lck.value = value;
      lck.hashlock = hashlock;
      lck.timelock = timelock;
   });
}

void eoshtlc::withdraw(name owner, string contract_name, checksum256 preimage) {
   htlcs idx(_self, owner.value);
   const auto& it = idx.get(htlc::hash(contract_name));
   check(it.activated, "contract not activated");
   check(it.timelock >= current_time_point(), "contract is expired");

   // `preimage` works as a key here.
   //require_auth(it.recipient);

   auto data = preimage.extract_as_byte_array();
   auto hash = eosio::sha256(reinterpret_cast<const char*>(data.data()), data.size());
   check(memcmp((const void*)it.hashlock.data(), (const void*)hash.data(), 32) == 0, "invalid preimage");

   transfer_action(it.value.contract, {{_self, "active"_n}}).send(_self, it.recipient, it.value.quantity, "FROM " + owner.to_string() + (contract_name.size() ? ", " : "") + contract_name);

   idx.erase(it);
}

void eoshtlc::cancel(name owner, string contract_name) {
   require_auth(owner);

   htlcs idx(_self, owner.value);
   const auto& it = idx.get(htlc::hash(contract_name));

   check(it.timelock < current_time_point(), "contract not expired");

   if (it.activated)
      transfer_action(it.value.contract, {{_self, "active"_n}}).send(_self, owner, it.value.quantity, "CANCELED TO " + it.recipient.to_string() + (contract_name.size() ? ", " : "") + contract_name);

   idx.erase(it);
}
