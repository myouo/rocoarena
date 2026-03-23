// tests/session/session_state_test.cpp
// Strategy tests #23-25: Session state machine tests
// DEFERRED: rocoarena_app cannot be linked yet (static init crash)

#include <gtest/gtest.h>

// =============================================================================
// These tests are placeholders for the session state machine tests.
// They will be implemented after the Application Service Layer (AppController)
// is extracted from the startup code.
// =============================================================================

TEST(SessionState, CannotStartBattleIfPlayersNotReady) {
    GTEST_SKIP() << "Deferred: requires rocoarena_app refactoring (AppController)";
}

TEST(SessionState, PlayerLeaveDuringBattleTriggersForfeit) {
    GTEST_SKIP() << "Deferred: requires rocoarena_app refactoring (AppController)";
}

TEST(SessionState, ValidJoinAddsPlayerToLobby) {
    GTEST_SKIP() << "Deferred: requires rocoarena_app refactoring (AppController)";
}
