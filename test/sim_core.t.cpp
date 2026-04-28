/* sim_core.t.cpp                                                     -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_core.h>
#include <gtest/gtest.h>

/// Test the `sim::time` class
TEST(SimCoreTest, Time) {
  // Clock starts at zero and stays until we advance it.
  EXPECT_EQ(0,  sim::time::clock());
  EXPECT_EQ(0,  sim::time::clock());

  // After each advance (and only when advanced), clock is incremented
  sim::time::advance_clock();
  EXPECT_EQ(1,  sim::time::clock());
  EXPECT_EQ(1,  sim::time::clock());
  sim::time::advance_clock();
  EXPECT_EQ(2,  sim::time::clock());
  EXPECT_EQ(2,  sim::time::clock());

  // Resetting clock sets it to zero again
  sim::time::reset_clock();
  EXPECT_EQ(0,  sim::time::clock());
  EXPECT_EQ(0,  sim::time::clock());
}

/// Single-thread test of the `sim::input_lead` and `sim::output_lead` classes.
TEST(SimCoreTest, InputOutputLead_ST) {
  {
    sim::output_lead ol;
    sim::input_lead  il; const sim::input_lead& ilC = il;
    il.connect_to(&ol);

    EXPECT_FALSE(ilC.get());

    sim::time::advance_clock();
    EXPECT_FALSE(ilC.get());  // Was not changed

    ol.set(true);
    EXPECT_FALSE(ilC.get());  // Does not see change

    sim::time::advance_clock();
    EXPECT_TRUE(ilC.get());  // Change is now visible

    ol.set(true); // No change
    EXPECT_TRUE(ilC.get());

    ol.set(false);
    EXPECT_TRUE(ilC.get());  // Does not see change

    ol.set(false);           // idempotent
    EXPECT_TRUE(ilC.get());  // Does not see change

    sim::time::advance_clock();
    EXPECT_FALSE(ilC.get());  // Change is now visible
  }

  {
    sim::output_lead ol(true);  // With initial value
    sim::input_lead  il; const sim::input_lead& ilC = il;
    il.connect_to(&ol);

    EXPECT_TRUE(ilC.get());  // See initial value

    sim::time::advance_clock();
    EXPECT_TRUE(ilC.get());  // No change

    ol.set(false);
    EXPECT_TRUE(ilC.get());  // Does not see change

    sim::time::advance_clock();
    EXPECT_FALSE(ilC.get());  // Change is now visible
  }
}

// Local Variables:
// c-basic-offset: 2
// End:
