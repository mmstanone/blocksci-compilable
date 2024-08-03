//
//  dedup_address_info.hpp
//  blocksci
//
//  Created by Harry Kalodner on 3/7/18.
//

#ifndef dedup_address_info_hpp
#define dedup_address_info_hpp

#include <blocksci/core/address_types.hpp>
#include <blocksci/core/dedup_address_type.hpp>
#include <blocksci/core/meta.hpp>

#include <vector>

namespace blocksci {
    template <DedupAddressType::Enum>
    struct DedupAddressInfo;
    
    template <>
    struct DedupAddressInfo<DedupAddressType::PUBKEY> {
        static inline constexpr char name[] = "pubkey_script";
        static inline constexpr bool equived = true;
        static inline constexpr bool spendable = true;
        static inline constexpr bool indexed = false;
        static inline constexpr std::array<AddressType::Enum, 4> equivTypes = {{AddressType::PUBKEY, AddressType::PUBKEYHASH, AddressType::MULTISIG_PUBKEY, AddressType::WITNESS_PUBKEYHASH}};
        static inline constexpr AddressType::Enum reprType = AddressType::PUBKEYHASH;
    };
    
    template <>
    struct DedupAddressInfo<DedupAddressType::SCRIPTHASH> {
        static inline constexpr char name[] = "scripthash_script";
        static inline constexpr bool equived = true;
        static inline constexpr bool spendable = true;
        static inline constexpr bool indexed = false;
        static inline constexpr std::array<AddressType::Enum, 2> equivTypes = {{AddressType::SCRIPTHASH, AddressType::WITNESS_SCRIPTHASH}};
        static inline constexpr AddressType::Enum reprType = AddressType::SCRIPTHASH;
    };
    
    template <>
    struct DedupAddressInfo<DedupAddressType::MULTISIG> {
        static inline constexpr char name[] = "multisig_script";
        static inline constexpr bool equived = true;
        static inline constexpr bool spendable = true;
        static inline constexpr bool indexed = true;
        static inline constexpr std::array<AddressType::Enum, 1> equivTypes = {{AddressType::MULTISIG}};
        static inline constexpr AddressType::Enum reprType = AddressType::MULTISIG;
    };
    
    template <>
    struct DedupAddressInfo<DedupAddressType::NONSTANDARD> {
        static inline constexpr char name[] = "nonstandard_script";
        static inline constexpr bool equived = false;
        static inline constexpr bool spendable = true;
        static inline constexpr bool indexed = true;
        static inline constexpr std::array<AddressType::Enum, 1> equivTypes = {{AddressType::NONSTANDARD}};
        static inline constexpr AddressType::Enum reprType = AddressType::NONSTANDARD;
    };
    
    template <>
    struct DedupAddressInfo<DedupAddressType::NULL_DATA> {
        static inline constexpr char name[] = "null_data_script";
        static inline constexpr bool equived = false;
        static inline constexpr bool spendable = false;
        static inline constexpr bool indexed = true;
        static inline constexpr std::array<AddressType::Enum, 1> equivTypes = {{AddressType::NULL_DATA}};
        static inline constexpr AddressType::Enum reprType = AddressType::NULL_DATA;
    };
    
    template <>
    struct DedupAddressInfo<DedupAddressType::WITNESS_UNKNOWN> {
        static inline constexpr char name[] = "witness_unknown";
        static inline constexpr bool equived = false;
        static inline constexpr bool spendable = true;
        static inline constexpr bool indexed = true;
        static inline constexpr std::array<AddressType::Enum, 1> equivTypes = {{AddressType::WITNESS_UNKNOWN}};
        static inline constexpr AddressType::Enum reprType = AddressType::WITNESS_UNKNOWN;
    };
    
    template<DedupAddressType::Enum type>
    struct SpendableFunctor {
        static constexpr bool f() {
            return DedupAddressInfo<type>::spendable;
        }
    };
    
    constexpr void dedupTypeCheckThrow(size_t index) {
        index >= DedupAddressType::size ? throw std::invalid_argument("dedup address type enum value is not valid") : 0;
    }
    
    static constexpr auto spendableTable = blocksci::make_static_table<DedupAddressType, SpendableFunctor>();
    
    constexpr bool isSpendable(DedupAddressType::Enum t) {
        auto index = static_cast<size_t>(t);
        dedupTypeCheckThrow(index);
        return spendableTable[index];
    }
    
    template<DedupAddressType::Enum type>
    struct EquivedFunctor {
        static constexpr bool f() {
            return DedupAddressInfo<type>::equived;
        }
    };
    
    static constexpr auto equivedTable = blocksci::make_static_table<DedupAddressType, EquivedFunctor>();
    
    constexpr bool isEquived(DedupAddressType::Enum t) {
        auto index = static_cast<size_t>(t);
        dedupTypeCheckThrow(index);
        return equivedTable[index];
    }
    
    std::string dedupAddressName(DedupAddressType::Enum type);
    
    template<DedupAddressType::Enum type>
    struct DedupAddressTypesFunctor {
        static std::vector<AddressType::Enum> f() {
            auto &types = DedupAddressInfo<type>::equivTypes;
            return std::vector<AddressType::Enum>{types.begin(), types.end()};
        }
    };
    
    inline std::vector<AddressType::Enum> equivAddressTypes(DedupAddressType::Enum t) {
        static auto &dedupAddressTypesTable = *[]() {
            auto nameTable = make_static_table<DedupAddressType, DedupAddressTypesFunctor>();
            return new decltype(nameTable){nameTable};
        }();
        auto index = static_cast<size_t>(t);
        dedupTypeCheckThrow(index);
        return dedupAddressTypesTable[index];
    }
}

#endif /* dedup_address_info_hpp */
