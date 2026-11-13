#pragma once

#include <QAbstractItemModel>

#include <functional>
#include <memory>
#include <type_traits>

#include "nullguard.hpp"

/*  Shared plumbing for QML singleton proxies over a model-shaped interface.
    A CRTP mixin, NOT a QIdentityProxyModel: moc can't do templates and the
    registrar can't resolve a template as a QML prototype. Derived lists
    QIdentityProxyModel FIRST and this SECOND, owns its Q_OBJECT, signals,
    properties and invokables, and forwards to the helpers below.

    The base type QIdentityProxyModel must come before this.

    CountFn : member pointer on Iface returning the count. nullptr -> rowCount().
    GetFn   : member pointer `T (Iface::*)(const QModelIndex &)`.
              nullptr -> Iface::pointed_to(index, callback). Only used if T != void. */
template <class Derived,
          class Iface = QAbstractItemModel,
          class T = void,
          auto CountFn = nullptr,
          auto GetFn = nullptr>
class ListProxyBase
{
    // Derived is incomplete here; only members that call these instantiate them
    Derived       &self()       { return static_cast<Derived &>(*this); }
    const Derived &self() const { return static_cast<const Derived &>(*this); }

public:
    // runs after Derived's QIdentityProxyModel base, which must be declared first
    ListProxyBase()
        : m_source(s_injected)
        , m_iface(qobject_cast<Iface *>(m_source.get()))
    {
        if (m_source) self().setSourceModel(m_source.get());
    }

    static void inject(const std::shared_ptr<QAbstractItemModel> &model) {
        s_injected = model;
    }

    qsizetype item_count() const {
        if constexpr (CountFn == nullptr)
            return self().rowCount();
        else
            return m_iface ? static_cast<qsizetype>(std::invoke(CountFn, *m_iface)) : 0;
    }

    // row arrives in this proxy's space
    T item_at(int row) const requires (!std::is_void_v<T>) {
        T result{};
        if (!m_iface) return result;

        const QModelIndex src = self().mapToSource(self().index(row, 0));
        if constexpr (GetFn == nullptr)
            m_iface->pointed_to(src, [&result](const T &v) { result = v; return true; });
        else
            result = std::invoke(GetFn, *m_iface, src);
        return result;
    }

protected:
    Iface *iface() const { return m_iface; }

    // Null-guarded call; returns R{} when no interface is injected.
    // Needs a non-overloaded member pointer (or a lambda taking Iface &).
    template <class Fn, class... A>
    std::invoke_result_t<Fn, Iface &, A...> call(Fn &&fn, A &&...a) const {
        return proxy_detail::call(m_iface, std::forward<Fn>(fn), std::forward<A>(a)...);
    }

    // Same, with an explicit fallback for when no interface is injected.
    template <class Fn, class... A>
    std::invoke_result_t<Fn, Iface &, A...>
    call_or(std::invoke_result_t<Fn, Iface &, A...> fallback, Fn &&fn, A &&...a) const {
        return proxy_detail::call_or(m_iface, std::move(fallback),
                                     std::forward<Fn>(fn), std::forward<A>(a)...);
    }

    // Interfaces can't declare signals, so bind by name.
    // Call from the DERIVED constructor body: during base construction the
    // dynamic metaobject is still QIdentityProxyModel's and `to` wouldn't resolve.
    void forward(const char *from, const char *to) {
        if (!m_source) return;
        [[maybe_unused]] const bool ok = QObject::connect(m_source.get(), from, &self(), to);
        Q_ASSERT_X(ok, "ListProxyBase::forward", from);
    }

private:
    std::shared_ptr<QAbstractItemModel> m_source;
    inline static std::shared_ptr<QAbstractItemModel> s_injected = nullptr;

    Iface *m_iface = nullptr; // same lifetime as m_source
};
