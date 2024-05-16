//
//  blockchain_py.cpp
//  blocksci
//
//  Created by Harry Kalodner on 7/4/17.
//
//

#include "blockchain_py.hpp"
#include "caster_py.hpp"
#include "sequence.hpp"

#include <blocksci/address/address.hpp>
#include <blocksci/chain/blockchain.hpp>
#include <blocksci/chain/access.hpp>
#include <blocksci/scripts/script_range.hpp>
#include <blocksci/cluster/cluster.hpp>
#include <blocksci/heuristics/tx_identification.hpp>
#include "../external/json/single_include/nlohmann/json.hpp"

namespace py = pybind11;

using namespace blocksci;

using json = nlohmann::json;
template <AddressType::Enum type>
using PythonScriptRange = Range<ScriptAddress<type>>;
using PythonScriptRangeVariant = to_variadic_t<to_address_tuple_t<PythonScriptRange>, mpark::variant>;

namespace {
    template<blocksci::AddressType::Enum type>
    struct PythonScriptRangeFunctor {
        static PythonScriptRangeVariant f(blocksci::DataAccess &access) {
            auto scriptCount = getScriptCount(type, access);
            return PythonScriptRange<type>{ranges::views::ints(uint32_t{1}, scriptCount + 1) | ranges::views::transform([&](uint32_t scriptNum) {
                return ScriptAddress<type>(scriptNum, access);
            })};
        }
    };
}

void init_blockchain(py::class_<Blockchain> &cl) {
    cl
    .def("__len__", [](Blockchain &chain) { return chain.size(); })
    .def("__bool__", [](Blockchain &range) { return !ranges::empty(range); })
    .def("__iter__", [](Blockchain &chain) { return pybind11::make_iterator(chain.begin(), chain.end()); },
         pybind11::keep_alive<0, 1>())
    .def("__getitem__", [](Blockchain &chain, int64_t posIndex) {
        auto chainSize = static_cast<int64_t>(chain.size());
        if (posIndex < 0) {
            posIndex += chainSize;
        }
        if (posIndex < 0 || posIndex >= chainSize) {
            throw pybind11::index_error();
        }
        return chain[posIndex];
    }, py::arg("index"))
    .def("__getitem__", [](Blockchain &chain, pybind11::slice slice) -> Range<Block> {
        size_t start, stop, step, slicelength;
        if (!slice.compute(chain.size(), &start, &stop, &step, &slicelength))
            throw pybind11::error_already_set();

        if (step != 1) {
            throw std::runtime_error{"Cannot slice blockchain with step size not equal to one"};
        }

        return ranges::any_view<Block, random_access_sized>{chain[{static_cast<BlockHeight>(start), static_cast<BlockHeight>(stop)}]};
    }, py::arg("slice"))
    ;

    cl
    .def(py::init<std::string>())
    .def(py::init<std::string, BlockHeight>())
    .def_property_readonly("data_location", &Blockchain::dataLocation, "Returns the location of the data directory that this Blockchain object represents.")
    .def_property_readonly("config_location", &Blockchain::configLocation, "Returns the location of the configuration file that this Blockchain object represents.")
    .def("reload", &Blockchain::reload, "Reload the blockchain to make new blocks visible (Invalidates current BlockSci objects).")
    .def("is_parser_running", &Blockchain::isParserRunning, "Returns whether the parser is currently operating on this chain's data directory.")
    .def("addresses", [](Blockchain &chain, AddressType::Enum type) {
        static constexpr auto table = make_dynamic_table<AddressType, PythonScriptRangeFunctor>();
        auto index = static_cast<size_t>(type);
        return table.at(index)(chain.getAccess());
    }, py::arg("address_type"), "Return a range of all addresses of the given type.")
    
    .def("address_count",
        &Blockchain::addressCount,
        "Get an upper bound of the number of address of a given type (This reflects the number of type equivlant addresses of that type).",
        pybind11::arg("address_type")
    )
    .def_property_readonly("blocks",
        +[](Blockchain &chain) -> Range<Block> {
        return ranges::any_view<Block, random_access_sized>{chain};
    }, "Returns a range of all the blocks in the chain")
    .def("tx_with_index", [](Blockchain &chain, uint32_t index) {
        return Transaction{index, chain.getAccess()};
    }, "This functions gets the transaction with given index.", pybind11::arg("index"))
    .def("tx_with_hash", [](Blockchain &chain, const std::string &hash) {
        return Transaction{hash, chain.getAccess()};
    },"This functions gets the transaction with given hash.", pybind11::arg("tx_hash"))
    .def("address_from_index", [](Blockchain &chain, uint32_t index, AddressType::Enum type) {
        return Address{index, type, chain.getAccess()};
    }, "Construct an address object from an address num and type", pybind11::arg("index"), pybind11::arg("type"))
    .def("address_from_string", [](Blockchain &chain, const std::string &addressString) -> ranges::optional<Address> {
        auto address = getAddressFromString(addressString, chain.getAccess());
        if (address) {
            return address;
        } else {
            return ranges::nullopt;
        }
    }, "Construct an address object from an address string", pybind11::arg("address_string"))
    .def("addresses_with_prefix", [](Blockchain &chain, const std::string &addressPrefix) {
        pybind11::list pyAddresses;
        auto addresses = getAddressesWithPrefix(addressPrefix, chain.getAccess());
        for (auto &address : addresses) {
            pyAddresses.append(address.getScript().wrapped);
        }
        return pyAddresses;
    }, "Find all addresses beginning with the given prefix", pybind11::arg("prefix"))
    .def("find_friends_who_dont_pay", [](Blockchain &chain, const pybind11::dict &keys, BlockHeight start, BlockHeight stop) {
        std::unordered_set<std::string> umap;
        for (auto item : keys) {
            std::string key = py::str(item.first).cast<std::string>();
            umap.insert(key);
        };

        using MapType = std::vector<std::string>;
        auto reduce_func = [](MapType &vec1, MapType &vec2) -> MapType& {
                vec1.reserve(vec1.size() + vec2.size());
                vec1.insert(vec1.end(), std::make_move_iterator(vec2.begin()), std::make_move_iterator(vec2.end()));
                return vec1;
        };

        auto map_func = [&umap](const Transaction &tx) -> MapType {
            MapType result;
            // if it is a ww2 coinjoin, then
            if (umap.find(tx.getHash().GetHex()) == umap.end()) {
                return {};
            }
            
            // go through all the outputs
            for (const auto& output : tx.outputs()) {
                if (!output.isSpent()) continue;
                // ignore direct coinjoin remixes
                if (umap.find(output.getSpendingTx().value().getHash().GetHex()) != umap.end()) {
                    continue;
                }

                // find an output that has all inputs from a coinjoin
                bool all_inputs_from_cj = true;
                for (auto input : output.getSpendingTx().value().inputs()) {
                    if (umap.find(input.getSpentTx().getHash().GetHex()) == umap.end()) {
                        all_inputs_from_cj = false;
                        break;
                    }
                }
                if (!all_inputs_from_cj) {
                    continue;
                }

                // and check whether at least one of the outputs is mixed again
                for (auto output2: output.getSpendingTx().value().outputs()) {
                    if (!output2.isSpent()) continue;
                    if (umap.find(output2.getSpendingTx().value().getHash().GetHex()) != umap.end()) {
                        result.push_back(output.getSpendingTx().value().getHash().GetHex());
                        break;
                    }
                }
            }
            return result;

        };

        return chain[{start, stop}].mapReduce<MapType, decltype(map_func), decltype(reduce_func)>(map_func, reduce_func);
    }, "Filter the blockchain to only include 'friends don't pay' transactions.", pybind11::arg("keys"), pybind11::arg("start"), pybind11::arg("stop"))

    .def("filter_ww2_coinjoin_txes", [](Blockchain &chain, BlockHeight start, BlockHeight stop) {
        return chain[{start, stop}].filter([](const Transaction &tx) {
            return blocksci::heuristics::isWasabi2CoinJoin(tx);
        });
    }, "Filter ww2 coinjoin transactions", pybind11::arg("start"), pybind11::arg("stop"))

    .def("filter_in_keys", [](Blockchain &chain, const pybind11::dict &keys, BlockHeight start, BlockHeight stop) {
        std::unordered_set<std::string> umap;
        for (auto item : keys) {
            std::string key = py::str(item.first).cast<std::string>();
            umap.insert(key);
        };
        std::cout << "Size of keys: " << umap.size() << std::endl;

        return chain[{start, stop}].filter([&umap](const Transaction &tx) {
            return umap.find(tx.getHash().GetHex()) != umap.end();
        });
    }, "Filter the blockchain to only include txes with the given keys", pybind11::arg("keys"), pybind11::arg("start"), pybind11::arg("stop"))


    .def("find_consolidation_3_hops", [](Blockchain &chain, const pybind11::dict &keys, BlockHeight start, BlockHeight stop) {
        std::unordered_set<std::string> umap;
        for (auto item : keys) {
            std::string key = py::str(item.first).cast<std::string>();
            umap.insert(key);
        };

        using MapType = std::pair<std::string, std::unordered_map<std::string, int>>;
        auto reduce_func = [](std::vector<MapType> &vec1, std::vector<MapType> &vec2) -> std::vector<MapType> & {
                vec1.reserve(vec1.size() + vec2.size());
                vec1.insert(vec1.end(), std::make_move_iterator(vec2.begin()), std::make_move_iterator(vec2.end()));
                return vec1;
            };

        auto map_func = [&umap](const Transaction &tx) -> std::vector<MapType> {
            if (umap.find(tx.getHash().GetHex()) == umap.end()) {
                return {};
            }

            std::unordered_map<std::string, int> found;
            for (const auto& output : tx.outputs()) {
                if (!output.isSpent()) continue;

                bool found_match = false;

                if (output.getSpendingTx().value().outputCount() < 2) {
                    auto output_spent_in = output.getSpendingTx().value().getHash().GetHex();
                    if (found.find(output_spent_in) == found.end()) {
                        found[output_spent_in] = 0;
                    }
                    found[output_spent_in]++;
                    continue;
                }

                for (const auto& output2 : output.getSpendingTx().value().outputs()) {
                    if (found_match) break;
                    if (!output2.isSpent()) continue;

                    if (output2.getSpendingTx().value().outputCount() < 2) {
                        auto output_spent_in = output2.getSpendingTx().value().getHash().GetHex();
                        if (found.find(output_spent_in) == found.end()) {
                            found[output_spent_in] = 0;
                        }
                        found[output_spent_in]++;
                        found_match = true;
                        break;
                    }

                    for (const auto& output3 : output2.getSpendingTx().value().outputs()) {
                        if (!output3.isSpent()) continue;

                        if (output3.getSpendingTx().value().outputCount() < 2) {
                            auto output_spent_in = output3.getSpendingTx().value().getHash().GetHex();
                            if (found.find(output_spent_in) == found.end()) {
                                found[output_spent_in] = 0;
                            }
                            found[output_spent_in]++;
                            found_match = true;
                            break;
                        }
                    }
                }
            }
            // remove entries from found where value is less than 2
            for (auto it = found.begin(); it != found.end(); ) {
                if (it->second < 2) {
                    it = found.erase(it);
                } else {
                    ++it;
                }
            }

            return {{tx.getHash().GetHex(), found}};
        };

        return chain[{start, stop}].mapReduce<std::vector<MapType>, decltype(map_func), decltype(reduce_func)>(map_func, reduce_func);

    }, "Map blockchain to get txes consolidated within 3 hops", pybind11::arg("keys"), pybind11::arg("start"), pybind11::arg("stop"))

    .def("find_hw_sw_coinjoins", [](Blockchain &chain, BlockHeight start, BlockHeight stop) {
        return chain[{start, stop}].filter([](const Transaction &tx) {
            auto result = blocksci::heuristics::isLongDormantInRemixes(tx);

            if (result == blocksci::heuristics::HWWalletRemixResult::False) {
                return false;
            }

            if (result == blocksci::heuristics::HWWalletRemixResult::Trezor) {
                // std::cout << "Trezor coinjoin: " << tx.getHash().GetHex() << std::endl;
                return true;
            }

            // std::cout << "SW coinjoin: " << tx.getHash().GetHex() << std::endl;
            return true;
        });
    }, "Filter hw_sw coinjoin transactions", pybind11::arg("start"), pybind11::arg("stop"))

    .def("find_traverses_in_coinjoin_flows", [](Blockchain& chain, BlockHeight start, BlockHeight stop, const pybind11::dict &from_keys, const pybind11::dict &to_keys, bool strict = false) {
        std::unordered_set<std::string> from_umap, to_umap;
        for (auto item : from_keys) {
            std::string key = py::str(item.first).cast<std::string>();
            from_umap.insert(key);
        }

        for (auto item : to_keys) {
            std::string key = py::str(item.first).cast<std::string>();
            to_umap.insert(key);
        }

        using MapType = std::tuple<std::string, std::unordered_set<std::string>, std::unordered_set<std::string>, uint64_t, bool>;

        auto reduce_func = [](std::vector<MapType> &vec1, std::vector<MapType> &vec2) -> std::vector<MapType> & {
                vec1.reserve(vec1.size() + vec2.size());
                vec1.insert(vec1.end(), std::make_move_iterator(vec2.begin()), std::make_move_iterator(vec2.end()));
                return vec1;
        };

        auto map_func = [&from_umap, &to_umap, strict](const Transaction &tx) -> std::vector<MapType> {
            // will run on each tx.
            // First, check if there is input to tx from a given coinjoin - from_umap
            // Get sum of all inputs from the coinjoin

            std::unordered_set<std::string> in_coinjoins = {}, out_coinjoins = {};
            uint64_t out_liquidity = 0;
            for (const auto& input : tx.inputs()) {
                if (from_umap.find(input.getSpentTx().getHash().GetHex()) != from_umap.end()) {
                    out_liquidity += input.getValue();
                    in_coinjoins.insert(input.getSpentTx().getHash().GetHex());
                }
                else if (strict) {
                    return {};
                }
            }

            // If sum is zero, then there is no input from the coinjoin
            if (out_liquidity == 0) {
                return {};
            }

            // Now, 2 cases:
            // case 1:
            //   there is a direct output to other coinjoin type - to_umap
            //   we have to sum all the outputs that are spent in the coinjoin
            // case 2:
            //   there is one more hop to switch to a different wallet
            //   we have to do that one more hop
            // if strict, if there is an output anywhere else, then we ignore the tx

            bool case_1 = false;

            for (const auto& output : tx.outputs()) {
                if (!output.isSpent()) continue;

                if (to_umap.find(output.getSpendingTx().value().getHash().GetHex()) != to_umap.end()) {
                    case_1 = true;
                    break;
                }
            }
            
            uint64_t actual_liquidity = 0;

            if (case_1) {
                for (const auto& output : tx.outputs()) {
                    if (!output.isSpent()) continue;

                    if (to_umap.find(output.getSpendingTx().value().getHash().GetHex()) != to_umap.end()) {
                        actual_liquidity += output.getValue();
                        out_coinjoins.insert(output.getSpendingTx().value().getHash().GetHex());
                    }
                    else if (strict) {
                        return {};
                    }
                }
            }
            else {
                for (const auto& output : tx.outputs()) {
                    if (!output.isSpent()) continue;
   
                    for (const auto& output2 : output.getSpendingTx().value().outputs()) {
                        if (!output2.isSpent()) continue;

                        if (to_umap.find(output2.getSpendingTx().value().getHash().GetHex()) != to_umap.end()) {
                            actual_liquidity += output2.getValue();
                            out_coinjoins.insert(output2.getSpendingTx().value().getHash().GetHex());
                        }
                        else if (strict) {
                            return {};
                        }
                    }
                }
            }

            if (out_liquidity == 0 || actual_liquidity == 0) {
                return {};
            }

            return {{tx.getHash().GetHex(), in_coinjoins, out_coinjoins, actual_liquidity, case_1}};
        };
        
        return chain[{start, stop}].mapReduce<std::vector<MapType>, decltype(map_func), decltype(reduce_func)>(map_func, reduce_func);

        
    }, "Filter transactions that traverse in coinjoin flows", pybind11::arg("start"), pybind11::arg("stop"), pybind11::arg("from_keys"), pybind11::arg("to_keys"), pybind11::arg("strict") = false)


    .def("_segment_indexes", [](Blockchain &chain, BlockHeight start, BlockHeight stop, unsigned int cpuCount) {
        auto segments = chain[{start, stop}].segment(cpuCount);
        std::vector<std::pair<BlockHeight, BlockHeight>> ret;
        ret.reserve(segments.size());
        for (auto segment : segments) {
            ret.emplace_back(segment.sl.start, segment.sl.stop);
        }
        return ret;
    })
    ;
}

void init_data_access(py::module &m) {
    py::class_<Access> (m, "_DataAccess", "Private class for accessing blockchain data")
    .def("tx_with_index", &Access::txWithIndex, "This functions gets the transaction with given index.")
    .def("tx_with_hash", &Access::txWithHash, "This functions gets the transaction with given hash.")
    .def("address_from_index", &Access::addressFromIndex, "Construct an address object from an address num and type")
    .def("address_from_string", [](Access &access, const std::string &addressString) -> ranges::optional<AnyScript> {
        auto address = access.addressFromString(addressString);
        if (address) {
            return address->getScript().wrapped;
        } else {
            return ranges::nullopt;
        }
    }, "Construct an address object from an address string")
    .def("addresses_with_prefix", [](Access &access, const std::string &addressPrefix) {
        py::list pyAddresses;
        auto addresses = access.addressesWithPrefix(addressPrefix);
        for (auto &address : addresses) {
            pyAddresses.append(address.getScript().wrapped);
        }
        return pyAddresses;
    }, "Find all addresses beginning with the given prefix")
    ;
}
