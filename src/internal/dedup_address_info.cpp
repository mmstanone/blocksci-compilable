//
//  dedup_address_info.cpp
//  blocksci
//
//  Created by Harry Kalodner on 3/7/18.
//

#include "dedup_address_info.hpp"

#include <string>

namespace blocksci {
    template<DedupAddressType::Enum type>
    struct DedupAddressNameFunctor {
        static std::string f() {
            return DedupAddressInfo<type>::name;
        }
    };
    
    std::string dedupAddressName(DedupAddressType::Enum type) {
        static auto &table = *[]() {
            auto nameTable = make_static_table<DedupAddressType, DedupAddressNameFunctor>();
            return new decltype(nameTable){nameTable};
        }();
        auto index = static_cast<size_t>(type);
        return table.at(index);
    }
}
