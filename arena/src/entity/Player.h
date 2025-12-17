#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>

#include <forward.h>

class Player {
  public:
    static constexpr std::size_t kMaxPets = 6;

    using Roster = std::array<Pet*, kMaxPets>;

    Player(Roster pets, std::size_t activeIndex = 0);

    Pet& activePet();
    const Pet& activePet() const;

    bool ensureActiveUsable();
    std::size_t activeIndex() const { return activeIndex_; }

    bool switchTo(std::size_t index);
    bool hasUsablePets() const;

  private:
    bool isValidIndex(std::size_t index) const;
    std::size_t findFirstUsableIndex() const;

    Roster pets_{};
    std::size_t activeIndex_ = 0;
};
