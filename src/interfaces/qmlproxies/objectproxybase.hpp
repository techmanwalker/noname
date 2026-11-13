#pragma once

#include <QObject>

#include <functional>
#include <memory>
#include <type_traits>

#include "nullguard.hpp"

/*  Shared plumbing for QML singleton proxies over a plain (non-model) interface.
    A CRTP mixin, NOT a QObject: moc can't do templates and the registrar can't
    resolve a template as a QML prototype. Derived lists QObject FIRST and this
    SECOND, owns its Q_OBJECT, signals, properties and invokables, and forwards
    to the helpers below. */
template <class Derived, class Iface>
class ObjectProxyBase
{
    Derived &self() { return static_cast<Derived &>(*this); }

public:
    ObjectProxyBase() : m_iface(s_injected) {}

    static void inject(const std::shared_ptr<Iface> &iface) {
        s_injected = iface;
    }

protected:
    Iface *iface() const { return m_iface.get(); }

    // Null-guarded call; returns R{} when no interface is injected.
    // Needs a non-overloaded member pointer (or a lambda taking Iface &).
    template <class Fn, class... A>
    std::invoke_result_t<Fn, Iface &, A...> call(Fn &&fn, A &&...a) const {
        return proxy_detail::call(m_iface.get(), std::forward<Fn>(fn), std::forward<A>(a)...);
    }

    // Same, with an explicit fallback for when no interface is injected.
    template <class Fn, class... A>
    std::invoke_result_t<Fn, Iface &, A...>
    call_or(std::invoke_result_t<Fn, Iface &, A...> fallback, Fn &&fn, A &&...a) const {
        return proxy_detail::call_or(m_iface.get(), std::move(fallback),
                                     std::forward<Fn>(fn), std::forward<A>(a)...);
    }

    // The interface can't declare signals and isn't a QObject itself: cross-cast
    // to the implementation and bind by name. Call from the DERIVED constructor
    // body, or `to` wouldn't resolve against the derived metaobject yet.
    void forward(const char *from, const char *to) {
        auto *src = dynamic_cast<QObject *>(m_iface.get());
        if (!src) return;
        [[maybe_unused]] const bool ok = QObject::connect(src, from, &self(), to);
        Q_ASSERT_X(ok, "ObjectProxyBase::forward", from);
    }

private:
    std::shared_ptr<Iface> m_iface;
    inline static std::shared_ptr<Iface> s_injected = nullptr;
};
