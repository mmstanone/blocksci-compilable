//
//  tx_identification.hpp
//  blocksci
//
//  Created by Harry Kalodner on 12/1/17.
//

#ifndef tx_identification_hpp
#define tx_identification_hpp

#include <blocksci/blocksci_export.h>
#include <blocksci/chain/chain_fwd.hpp>
#include <blocksci/scripts/scripts_fwd.hpp>

namespace blocksci {
    class DataAccess;
    namespace heuristics {
    
    enum class BLOCKSCI_EXPORT CoinJoinResult {
        True, False, Timeout
    };

    enum class BLOCKSCI_EXPORT HWWalletRemixResult {
        Trezor, SW, False
    };

    bool BLOCKSCI_EXPORT isPeelingChain(const Transaction &tx);
    bool BLOCKSCI_EXPORT isCoinjoin(const Transaction &tx);
    CoinJoinResult BLOCKSCI_EXPORT isPossibleCoinjoin(const Transaction &tx, int64_t minBaseFee, double percentageFee, size_t maxDepth);
    CoinJoinResult BLOCKSCI_EXPORT isCoinjoinExtra(const Transaction &tx, int64_t minBaseFee, double percentageFee, size_t maxDepth);
    bool BLOCKSCI_EXPORT isDeanonTx(const Transaction &tx);
    bool BLOCKSCI_EXPORT containsKeysetChange(const Transaction &tx);
    bool BLOCKSCI_EXPORT isChangeOverTx(const Transaction &tx);
    bool BLOCKSCI_EXPORT isWasabi2CoinJoin(const Transaction &tx, std::optional<uint64_t> minInputCount = std::nullopt);
    bool BLOCKSCI_EXPORT isWasabi1CoinJoin(const Transaction &tx);
    bool BLOCKSCI_EXPORT isWhirlpoolCoinJoin(const Transaction &tx);

    HWWalletRemixResult BLOCKSCI_EXPORT isLongDormantInRemixes(const Transaction &tx);
}}


#endif /* tx_identification_hpp */
