#ifndef blocksci_coinjoin_utils_hpp
#define blocksci_coinjoin_utils_hpp

#include <blocksci/blocksci_export.h>
#include <unordered_set>
#include <limits.h>
#include <algorithm>

namespace blocksci {
    class BLOCKSCI_EXPORT CoinjoinUtils {
        // port from dumplings
        static std::unordered_set<long> compute_ww2_denominations() {
            long maxSatoshis = 134375000000;
            long minSatoshis = 5000;
            long denom = 1l;

            std::unordered_set<long> denominations;
            denominations.insert(minSatoshis);
            denominations.insert(maxSatoshis);

            // Powers of 2
            for (int i = 0; i < maxSatoshis; i++)
            {
                denom *= 2;
                if (denom < minSatoshis)
                {
                    continue;
                }

                if (denom > maxSatoshis)
                {
                    break;
                }

                denominations.insert(denom);
            }

            denom = 3;
            // Powers of 3
            for (int i = 0; i < INT_MAX; i++)
            {
                denom *= 3;

                if (denom < minSatoshis)
                {
                    continue;
                }

                if (denom > maxSatoshis)
                {
                    break;
                }

                denominations.insert(denom);
            }

            // Powers of 3 * 2
            denom = 3;
            for (int i = 0; i < INT_MAX; i++)
            {
                denom *= 3;
                auto next = denom * 2;
                if (denom < minSatoshis)
                {
                    continue;
                }

                if (denom > maxSatoshis)
                {
                    break;
                }

                denominations.insert(next);
            }

            denom = 10;
            // Powers of 10 (1-2-5 series)
            for (int i = 0; i < INT_MAX; i++)
            {
                denom *= 10;

                if (denom < minSatoshis)
                {
                    continue;
                }

                if (denom > maxSatoshis)
                {
                    break;
                }

                denominations.insert(denom);
            }

            denom = 10;
            // Powers of 10 * 2 (1-2-5 series)
            for (int i = 0; i < INT_MAX; i++)
            {
                denom *= 10;
                auto a = denom * 2;
                if (denom < minSatoshis)
                {
                    continue;
                }

                if (denom > maxSatoshis)
                {
                    break;
                }

                denominations.insert(a);
            }

            denom = 10;
            // Powers of 10 * 5 (1-2-5 series)
            for (int i = 0; i < INT_MAX; i++)
            {
                denom *= 10;
                auto a = denom * 2;

                if (denom < minSatoshis)
                {
                    continue;
                }

                if (denom > maxSatoshis)
                {
                    break;
                }

                denominations.insert(a);
            }

            return denominations; 
        }
    public:
        static inline std::unordered_set<long> ww2_denominations = CoinjoinUtils::compute_ww2_denominations();

        CoinjoinUtils() = default;
        CoinjoinUtils(const CoinjoinUtils &other) = default;
        CoinjoinUtils(CoinjoinUtils &&other) = default;
        CoinjoinUtils &operator=(const CoinjoinUtils &other) = default;
        CoinjoinUtils &operator=(CoinjoinUtils &&other) = default;
        ~CoinjoinUtils() = default;


    };
}

#endif
