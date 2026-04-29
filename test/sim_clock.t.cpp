/* sim_clock.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_clock.h>
#include <gtest/gtest.h>

/// Test the `sim::clock` class
TEST(SimClockTest, Clock) {
  // Clock starts at zero and stays until we advance it.
  EXPECT_EQ(0,  sim::clock::value());
  EXPECT_EQ(0,  sim::clock::value());

  // After each advance (and only when advanced), clock is incremented
  sim::clock::advance();
  EXPECT_EQ(1,  sim::clock::value());
  EXPECT_EQ(1,  sim::clock::value());
  sim::clock::advance();
  EXPECT_EQ(2,  sim::clock::value());
  EXPECT_EQ(2,  sim::clock::value());

  // Resetting clock sets it to zero again
  sim::clock::reset();
  EXPECT_EQ(0,  sim::clock::value());
  EXPECT_EQ(0,  sim::clock::value());
}

// Local Variables:
// c-basic-offset: 2
// End:
