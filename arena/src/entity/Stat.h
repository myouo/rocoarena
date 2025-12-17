#pragma once
#include <cstddef>

// Battle-relevant stats that can be staged; Ene kept for base HP indexing.
enum Stat {
    Ene = 0,
    Atk,
    Def,
    SpA,
    SpD,
    Spe,
    Acc, // 命中
    Eva, // 闪避
    Cri  // 暴击
};

constexpr std::size_t kStatCount = static_cast<std::size_t>(Cri) + 1;
