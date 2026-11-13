#pragma once

#include <functional>
#include <type_traits>

namespace proxy_detail {

// Null-guarded member call shared by the proxy bases.
template <class Iface, class Fn, class... A>
std::invoke_result_t<Fn, Iface &, A...> call(Iface *iface, Fn &&fn, A &&...a)
{
    using R = std::invoke_result_t<Fn, Iface &, A...>;
    if (!iface) {
        if constexpr (std::is_void_v<R>) return;
        else if constexpr (std::is_reference_v<R>) {
            // keep reference semantics: a null interface yields a shared empty value
            static_assert(std::is_lvalue_reference_v<R> && std::is_const_v<std::remove_reference_t<R>>,
                          "only const lvalue-reference returns have a safe fallback");
            static const std::remove_cvref_t<R> empty{};
            return empty;
        }
        else return R{};
    }
    return std::invoke(std::forward<Fn>(fn), *iface, std::forward<A>(a)...);
}

template <class Iface, class Fn, class... A>
std::invoke_result_t<Fn, Iface &, A...>
call_or(Iface *iface, std::invoke_result_t<Fn, Iface &, A...> fallback, Fn &&fn, A &&...a)
{
    static_assert(!std::is_reference_v<std::invoke_result_t<Fn, Iface &, A...>>,
                  "call_or with a reference return would risk returning a dangling fallback");
    if (!iface) return fallback;
    return std::invoke(std::forward<Fn>(fn), *iface, std::forward<A>(a)...);
}

} // namespace proxy_detail
