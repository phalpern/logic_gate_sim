/* sim_core.t.cpp                                                     -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_core.h>
#include <gtest/gtest.h>

#include <iostream>

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

// Local Variables:
// c-basic-offset: 2
// End:
