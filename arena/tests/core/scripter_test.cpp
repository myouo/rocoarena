// tests/core/scripter_test.cpp
// Strategy tests #14-18: Lua scripter contract & abnormal path tests

#include <gtest/gtest.h>
#include <core/scripter/scripter.h>

// =============================================================================
// #14: LuaContract_Loads_Valid_Script_Successfully
// =============================================================================

TEST(Scripter, LoadsValidScriptSuccessfully) {
    Scripter s;
    bool ok = s.runString("x = 1 + 2");
    EXPECT_TRUE(ok);
    EXPECT_EQ(s.get<int>("x"), 3);
}

// =============================================================================
// #15: LuaContract_Fails_Gracefully_On_SyntaxError
// =============================================================================

TEST(Scripter, FailsGracefullyOnSyntaxError) {
    Scripter s;
    // Malformed Lua: missing 'end'
    bool ok = s.runString("function foo( return 1");
    EXPECT_FALSE(ok); // Must return false, not crash
}

// =============================================================================
// #16: LuaContract_Fails_Gracefully_On_Missing_Execute_Function
// =============================================================================

TEST(Scripter, FailsOnMissingFunction) {
    Scripter s;
    s.runString("function greet() return 'hello' end");

    // Calling a function that doesn't exist should throw
    EXPECT_THROW(
        s.call<int>("nonexistent_function"),
        std::runtime_error
    );
}

// =============================================================================
// #17: LuaContract_API_DealDamage_Mutates_Target_HP
// Register a C++ function into Lua that modifies a C++ variable
// =============================================================================

TEST(Scripter, API_DealDamage_MutatesCppSide) {
    Scripter s;

    int hp = 100;
    s.registerFunction("deal_damage", [&hp](int dmg) {
        hp -= dmg;
        if (hp < 0) hp = 0;
    });

    bool ok = s.runString("deal_damage(30)");
    EXPECT_TRUE(ok);
    EXPECT_EQ(hp, 70);

    ok = s.runString("deal_damage(200)");
    EXPECT_TRUE(ok);
    EXPECT_EQ(hp, 0); // Clamped to 0
}

// =============================================================================
// #18: LuaContract_API_ApplyBuff_Adds_Buff_To_Target
// Register a buff application function, verify from C++ side
// =============================================================================

TEST(Scripter, API_ApplyBuff_ModifiesCppState) {
    Scripter s;

    int atkStage = 0;
    s.registerFunction("apply_buff", [&atkStage](int delta) {
        atkStage += delta;
        if (atkStage > 6) atkStage = 6;
        if (atkStage < -6) atkStage = -6;
    });

    bool ok = s.runString("apply_buff(2)");
    EXPECT_TRUE(ok);
    EXPECT_EQ(atkStage, 2);

    ok = s.runString("apply_buff(5)"); // Would be 7, capped at 6
    EXPECT_TRUE(ok);
    EXPECT_EQ(atkStage, 6);
}

// =============================================================================
// Additional contract tests
// =============================================================================

TEST(Scripter, RunString_EmptyString) {
    Scripter s;
    bool ok = s.runString("");
    EXPECT_TRUE(ok);
}

TEST(Scripter, RunScript_NonexistentFile) {
    Scripter s;
    bool ok = s.runScript("/nonexistent/path/fake_script.lua");
    EXPECT_FALSE(ok);
}

TEST(Scripter, SetAndGetVariable) {
    Scripter s;
    s.set("my_var", 42);
    EXPECT_EQ(s.get<int>("my_var"), 42);
}

TEST(Scripter, CallFunctionWithReturnValue) {
    Scripter s;
    s.runString("function add(a, b) return a + b end");
    int result = s.call<int>("add", 3, 7);
    EXPECT_EQ(result, 10);
}
