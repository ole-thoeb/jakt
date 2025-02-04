/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Jakt/HashMap.h>
#include <Jakt/NonnullRefPtr.h>
#include <Jakt/RefCounted.h>
#include <Jakt/Tuple.h>

namespace JaktInternal {
using namespace Jakt;

template<typename K, typename V>
struct DictionaryStorage : public RefCounted<DictionaryStorage<K, V>> {
    HashMap<K, V> map;
};

template<typename K, typename V>
class DictionaryIterator {
    using Storage = DictionaryStorage<K, V>;
    using Iterator = typename HashMap<K, V>::IteratorType;

public:
    DictionaryIterator(NonnullRefPtr<Storage> storage)
        : m_storage(move(storage))
        , m_iterator(m_storage->map.begin())
    {
    }

    Optional<Tuple<K, V>> next()
    {
        if (m_iterator == m_storage->map.end())
            return {};
        auto res = *m_iterator;
        ++m_iterator;
        return Tuple<K, V>(res.key, res.value);
    }

private:
    NonnullRefPtr<Storage> m_storage;
    Iterator m_iterator;
};

template<typename K, typename V>
class Dictionary {
    using Storage = DictionaryStorage<K, V>;

public:
    bool is_empty() const { return m_storage->map.is_empty(); }
    size_t size() const { return m_storage->map.size(); }
    void clear() { m_storage->map.clear(); }

    ErrorOr<void> set(K const& key, V value)
    {
        TRY(m_storage->map.set(key, move(value)));
        return {};
    }

    bool remove(K const& key) { return m_storage->map.remove(key); }
    bool contains(K const& key) const { return m_storage->map.contains(key); }

    Optional<V> get(K const& key) const { return m_storage->map.get(key); }
    V& operator[](K const& key) { return m_storage->map.get(key).value(); }
    V const& operator[](K const& key) const { return m_storage->map.get(key).value(); }

    ErrorOr<Array<K>> keys() const
    {
        Array<K> keys;
        TRY(keys.ensure_capacity(m_storage->map.size()));
        for (auto& it : m_storage->map) {
            MUST(keys.push(it.key));
        }
        return keys;
    }

    ErrorOr<void> ensure_capacity(size_t capacity)
    {
        TRY(m_storage->map.ensure_capacity(capacity));
        return {};
    }

    // FIXME: Remove this constructor once Jakt knows how to call Dictionary::create_empty()
    Dictionary()
        : m_storage(MUST(adopt_nonnull_ref_or_enomem(new (nothrow) Storage)))
    {
    }

    static ErrorOr<Dictionary> create_empty()
    {
        auto storage = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) Storage));
        return Dictionary { move(storage) };
    }

    struct Entry {
        K key;
        V value;
    };
    static ErrorOr<Dictionary> create_with_entries(std::initializer_list<Entry> list)
    {
        auto dictionary = TRY(create_empty());
        TRY(dictionary.ensure_capacity(list.size()));
        for (auto& item : list)
            TRY(dictionary.set(item.key, item.value));
        return dictionary;
    }

    DictionaryIterator<K, V> iterator() const { return DictionaryIterator<K, V> { m_storage }; }

private:
    explicit Dictionary(NonnullRefPtr<Storage> storage)
        : m_storage(move(storage))
    {
    }

    NonnullRefPtr<Storage> m_storage;
};

}

namespace Jakt {
using JaktInternal::Dictionary;
using JaktInternal::DictionaryIterator;
}
