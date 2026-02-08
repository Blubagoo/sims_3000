/**
 * @file test_simulation_pipeline.cpp
 * @brief Tests for SimulationPipeline (Ticket 4-048)
 *
 * Verifies:
 * - Empty pipeline ticks without crash
 * - Register single system, verify tick called
 * - Multiple systems execute in priority order
 * - ZoneSystem priority is 30
 * - BuildingSystem priority is 40
 * - Full ordering: 5, 10, 20, 30, 40
 * - Duplicate priority handled (both called)
 * - System count correct
 * - Get execution order returns sorted names
 * - Delta time passed correctly to all systems
 */

#include <gtest/gtest.h>
#include <sims3000/sim/SimulationPipeline.h>
#include <vector>
#include <string>

using namespace sims3000::sim;

namespace {

class SimulationPipelineTest : public ::testing::Test {
protected:
    SimulationPipeline pipeline;
};

// =========================================================================
// Empty Pipeline Ticks Without Crash
// =========================================================================

TEST_F(SimulationPipelineTest, EmptyPipelineTicksWithoutCrash) {
    // Should not crash
    pipeline.tick(0.05f);
    EXPECT_EQ(pipeline.system_count(), 0u);
}

// =========================================================================
// Register Single System, Verify Tick Called
// =========================================================================

TEST_F(SimulationPipelineTest, RegisterSingleSystemVerifyTickCalled) {
    bool called = false;
    pipeline.register_system(10, [&called](float) { called = true; }, "TestSystem");

    pipeline.tick(0.05f);
    EXPECT_TRUE(called);
}

// =========================================================================
// Multiple Systems Execute In Priority Order
// =========================================================================

TEST_F(SimulationPipelineTest, MultipleSystemsExecuteInPriorityOrder) {
    std::vector<int> execution_order;

    pipeline.register_system(30, [&execution_order](float) { execution_order.push_back(30); }, "Zone");
    pipeline.register_system(10, [&execution_order](float) { execution_order.push_back(10); }, "Energy");
    pipeline.register_system(40, [&execution_order](float) { execution_order.push_back(40); }, "Building");
    pipeline.register_system(5,  [&execution_order](float) { execution_order.push_back(5); },  "Terrain");
    pipeline.register_system(20, [&execution_order](float) { execution_order.push_back(20); }, "Fluid");

    pipeline.tick(0.05f);

    ASSERT_EQ(execution_order.size(), 5u);
    EXPECT_EQ(execution_order[0], 5);
    EXPECT_EQ(execution_order[1], 10);
    EXPECT_EQ(execution_order[2], 20);
    EXPECT_EQ(execution_order[3], 30);
    EXPECT_EQ(execution_order[4], 40);
}

// =========================================================================
// ZoneSystem Priority Is 30
// =========================================================================

TEST_F(SimulationPipelineTest, ZoneSystemPriorityIs30) {
    std::vector<std::string> execution_order;

    pipeline.register_system(40, [&execution_order](float) { execution_order.push_back("Building"); }, "Building");
    pipeline.register_system(30, [&execution_order](float) { execution_order.push_back("Zone"); }, "Zone");

    pipeline.tick(0.05f);

    ASSERT_EQ(execution_order.size(), 2u);
    EXPECT_EQ(execution_order[0], "Zone");
    EXPECT_EQ(execution_order[1], "Building");
}

// =========================================================================
// BuildingSystem Priority Is 40
// =========================================================================

TEST_F(SimulationPipelineTest, BuildingSystemPriorityIs40) {
    std::vector<std::string> execution_order;

    pipeline.register_system(40, [&execution_order](float) { execution_order.push_back("Building"); }, "Building");
    pipeline.register_system(5,  [&execution_order](float) { execution_order.push_back("Terrain"); }, "Terrain");

    pipeline.tick(0.05f);

    ASSERT_EQ(execution_order.size(), 2u);
    EXPECT_EQ(execution_order[0], "Terrain");
    EXPECT_EQ(execution_order[1], "Building");
}

// =========================================================================
// Full Ordering: 5, 10, 20, 30, 40
// =========================================================================

TEST_F(SimulationPipelineTest, FullOrderingVerification) {
    std::vector<std::string> execution_order;

    // Register in scrambled order
    pipeline.register_system(40, [&execution_order](float) { execution_order.push_back("BuildingSystem"); }, "BuildingSystem");
    pipeline.register_system(5,  [&execution_order](float) { execution_order.push_back("TerrainSystem"); }, "TerrainSystem");
    pipeline.register_system(20, [&execution_order](float) { execution_order.push_back("FluidStub"); }, "FluidStub");
    pipeline.register_system(30, [&execution_order](float) { execution_order.push_back("ZoneSystem"); }, "ZoneSystem");
    pipeline.register_system(10, [&execution_order](float) { execution_order.push_back("EnergyStub"); }, "EnergyStub");

    pipeline.tick(0.05f);

    ASSERT_EQ(execution_order.size(), 5u);
    EXPECT_EQ(execution_order[0], "TerrainSystem");
    EXPECT_EQ(execution_order[1], "EnergyStub");
    EXPECT_EQ(execution_order[2], "FluidStub");
    EXPECT_EQ(execution_order[3], "ZoneSystem");
    EXPECT_EQ(execution_order[4], "BuildingSystem");
}

// =========================================================================
// Duplicate Priority Handled (Both Called)
// =========================================================================

TEST_F(SimulationPipelineTest, DuplicatePriorityHandledBothCalled) {
    int call_count = 0;

    pipeline.register_system(10, [&call_count](float) { call_count++; }, "SystemA");
    pipeline.register_system(10, [&call_count](float) { call_count++; }, "SystemB");

    pipeline.tick(0.05f);

    EXPECT_EQ(call_count, 2);
}

// =========================================================================
// System Count Correct
// =========================================================================

TEST_F(SimulationPipelineTest, SystemCountCorrect) {
    EXPECT_EQ(pipeline.system_count(), 0u);

    pipeline.register_system(10, [](float) {}, "A");
    EXPECT_EQ(pipeline.system_count(), 1u);

    pipeline.register_system(20, [](float) {}, "B");
    EXPECT_EQ(pipeline.system_count(), 2u);

    pipeline.register_system(30, [](float) {}, "C");
    EXPECT_EQ(pipeline.system_count(), 3u);
}

// =========================================================================
// Get Execution Order Returns Sorted Names
// =========================================================================

TEST_F(SimulationPipelineTest, GetExecutionOrderReturnsSortedNames) {
    pipeline.register_system(40, [](float) {}, "BuildingSystem");
    pipeline.register_system(5,  [](float) {}, "TerrainSystem");
    pipeline.register_system(30, [](float) {}, "ZoneSystem");
    pipeline.register_system(10, [](float) {}, "EnergyStub");
    pipeline.register_system(20, [](float) {}, "FluidStub");

    auto order = pipeline.get_execution_order();

    ASSERT_EQ(order.size(), 5u);
    EXPECT_STREQ(order[0], "TerrainSystem");
    EXPECT_STREQ(order[1], "EnergyStub");
    EXPECT_STREQ(order[2], "FluidStub");
    EXPECT_STREQ(order[3], "ZoneSystem");
    EXPECT_STREQ(order[4], "BuildingSystem");
}

// =========================================================================
// Delta Time Passed Correctly To All Systems
// =========================================================================

TEST_F(SimulationPipelineTest, DeltaTimePassedCorrectlyToAllSystems) {
    float received_dt_a = 0.0f;
    float received_dt_b = 0.0f;

    pipeline.register_system(10, [&received_dt_a](float dt) { received_dt_a = dt; }, "A");
    pipeline.register_system(20, [&received_dt_b](float dt) { received_dt_b = dt; }, "B");

    pipeline.tick(0.0167f);

    EXPECT_FLOAT_EQ(received_dt_a, 0.0167f);
    EXPECT_FLOAT_EQ(received_dt_b, 0.0167f);
}

// =========================================================================
// Multiple Ticks Work Correctly
// =========================================================================

TEST_F(SimulationPipelineTest, MultipleTicksWorkCorrectly) {
    int call_count = 0;

    pipeline.register_system(10, [&call_count](float) { call_count++; }, "Counter");

    pipeline.tick(0.05f);
    pipeline.tick(0.05f);
    pipeline.tick(0.05f);

    EXPECT_EQ(call_count, 3);
}

} // anonymous namespace
