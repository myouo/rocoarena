#include "Player.h"

#include <logger/logger.h>

#include <Pet.h>

Player::Player(Roster pets, std::size_t activeIndex) : pets_(pets) {
    if (isValidIndex(activeIndex) && pets_[activeIndex]) {
        activeIndex_ = activeIndex;
    } else if (auto idx = findFirstUsableIndex(); idx < kMaxPets) {
        LOG_WARN("Player", "Initial active index invalid; auto-switching to first usable slot ", idx);
        activeIndex_ = idx;
    } else {
        LOG_ERROR("Player", "No usable pets provided; active slot remains empty.");
        activeIndex_ = 0;
    }
}

Pet& Player::activePet() {
    if (!pets_[activeIndex_]) {
        throw std::runtime_error("Active pet is null");
    }
    return *pets_[activeIndex_];
}

const Pet& Player::activePet() const {
    if (!pets_[activeIndex_]) {
        throw std::runtime_error("Active pet is null");
    }
    return *pets_[activeIndex_];
}

bool Player::ensureActiveUsable() {
    if (isValidIndex(activeIndex_) && pets_[activeIndex_] && !pets_[activeIndex_]->isFainted()) {
        return true;
    }
    auto idx = findFirstUsableIndex();
    if (idx < kMaxPets) {
        activeIndex_ = idx;
        LOG_INFO("Player", "Auto-switched active pet to slot ", idx);
        return true;
    }
    return false;
}

bool Player::switchTo(std::size_t index) {
    if (!isValidIndex(index)) {
        LOG_WARN("Player", "Switch failed: index out of range ", index);
        return false;
    }
    Pet* target = pets_[index];
    if (!target) {
        LOG_WARN("Player", "Switch failed: empty slot at index ", index);
        return false;
    }
    if (target->isFainted()) {
        LOG_WARN("Player", "Switch failed: target pet fainted at index ", index);
        return false;
    }
    if (index == activeIndex_) {
        LOG_INFO("Player", "Switch skipped: already active at index ", index);
        return false;
    }
    activeIndex_ = index;
    LOG_INFO("Player", "Switched active pet to index ", index);
    return true;
}

bool Player::hasUsablePets() const {
    for (auto* pet : pets_) {
        if (pet && !pet->isFainted()) return true;
    }
    return false;
}

bool Player::isValidIndex(std::size_t index) const {
    return index < kMaxPets;
}

std::size_t Player::findFirstUsableIndex() const {
    for (std::size_t i = 0; i < kMaxPets; ++i) {
        if (pets_[i] && !pets_[i]->isFainted()) {
            return i;
        }
    }
    return kMaxPets;
}
