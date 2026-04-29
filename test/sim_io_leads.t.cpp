/* sim_io_leads.t.cpp                                                 -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_io_leads.h>
#include <gtest/gtest.h>

/// Single-thread test of the main functionality of `sim::input_lead::get` and
/// `sim::output_lead` classes.
TEST(SimIOLeads, InputOutputLead_ST) {
  sim::output_lead ol;
  sim::input_lead  il; const sim::input_lead& ilC = il;
  il.connect_to(&ol);

  EXPECT_FALSE(ilC.get());  // initial value

  sim::clock::advance();
  EXPECT_FALSE(ilC.get());  // no change

  ol.set(true);
  EXPECT_FALSE(ilC.get());  // Does not see change before clock update

  sim::clock::advance();
  EXPECT_TRUE(ilC.get());   // Change is now visible

  ol.set(true);             // No change
  EXPECT_TRUE(ilC.get());

  ol.set(false);
  EXPECT_TRUE(ilC.get());   // Does not see change before clock update

  ol.set(false);            // idempotent
  EXPECT_TRUE(ilC.get());   // Does not see change before clock update

  sim::clock::advance();
  EXPECT_FALSE(ilC.get());  // Change is now visible
}

/// Single-thread test of the of `sim::input_lead::get` and
/// `sim::output_lead` classes `sim::output_lead` initialized to `true`.
TEST(SimIOLeads, InitializedInputOutputLead_ST) {
  sim::output_lead ol(true);  // With initial value
  sim::input_lead  il; const sim::input_lead& ilC = il;
  il.connect_to(&ol);

  EXPECT_TRUE(ilC.get());  // Sees initial value

  sim::clock::advance();
  EXPECT_TRUE(ilC.get());  // No change

  ol.set(false);
  EXPECT_TRUE(ilC.get());  // Does not see change before click update

  sim::clock::advance();
  EXPECT_FALSE(ilC.get());  // Change is now visible
}

// Local Variables:
// c-basic-offset: 2
// End:
