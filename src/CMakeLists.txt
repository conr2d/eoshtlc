project(eoshtlc)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract(eoshtlc eoshtlc eoshtlc.cpp)
target_include_directories(eoshtlc PUBLIC ${CMAKE_SOURCE_DIR}/../include)
#target_ricardian_directory(eoshtlc ${CMAKE_SOURCE_DIR}/../ricardian)
