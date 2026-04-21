#pragma once
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace Webview
{
    namespace Helpers
    {
        template <std::size_t I, typename Tuple, typename Function, std::enable_if_t<(I >= 0)> * = nullptr>
        void setTupleImpl(Tuple &tuple, Function func)
        {
            if constexpr (I >= 0)
            {
                func(I, std::get<I>(tuple));
                if constexpr (I > 0)
                {
                    setTupleImpl<I - 1>(tuple, func);
                }
            }
        }
        template <typename Tuple, typename Function> void setTuple(Tuple &tuple, Function func)
        {
            constexpr int size = std::tuple_size_v<Tuple> - 1;
            if constexpr (size >= 0)
            {
                setTupleImpl<size>(tuple, func);
            }
        }
    } // namespace Helpers
} // namespace Webview