#pragma once

// Lightweight stand-in for robin_hood hashing to avoid external dependency.
// If you later vendor the real robin_hood.h, replace this file.
#include <unordered_map>
#include <unordered_set>

namespace robin_hood {
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, T>>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Alloc>;

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Alloc = std::allocator<Key>>
using unordered_set = std::unordered_set<Key, Hash, KeyEqual, Alloc>;
}  // namespace robin_hood
