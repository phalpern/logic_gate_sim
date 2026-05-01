/* sim_external_io.t.cpp                                              -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_external_io.h>
#include <gtest/gtest.h>

/// Test both `external_inputs` and `external_outputs` using
/// `set_value_immediate`. Though not best practice, it is easiest to test both
/// classes at once, as each is an observer for the other.
TEST(sim_external_io, SetValueImmediate) {
  // Create a 3-port `external_outputs` connected to a 3-port
  // `external_inputs`, where output port 0, 1, and 2 are connected to input
  // ports 1, 0, 2.
  sim::external_inputs<3>  ei;
  sim::external_outputs<3> eo;
  eo.connect_input(0, &ei, 1);
  eo.connect_input(1, &ei, 0);
  eo.connect_input(2, &ei, 2);

  // Set new values on 2 of the ports.
  ei.set_value_immediate(1, true);
  ei.set_value_immediate(2, true);

  // Changes to `ei` are reflected immediately in `eo`.
  EXPECT_TRUE( eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_TRUE( eo.get_value(2));

  // `execute` is invocable, but changes nothing.
  ei.execute();
  eo.execute();
  EXPECT_TRUE( eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_TRUE( eo.get_value(2));

  // Advancing the clock changes nothing.
  sim::clock::advance();
  EXPECT_TRUE( eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_TRUE( eo.get_value(2));

  // Invert the values and check again
  ei.set_value_immediate(0, true);
  ei.set_value_immediate(1, false);
  ei.set_value_immediate(2, false);
  EXPECT_FALSE(eo.get_value(0));
  EXPECT_TRUE( eo.get_value(1));
  EXPECT_FALSE(eo.get_value(2));

  // `execute` changes nothing
  ei.execute();
  eo.execute();
  EXPECT_FALSE(eo.get_value(0));
  EXPECT_TRUE( eo.get_value(1));
  EXPECT_FALSE(eo.get_value(2));

  // Advancing the clock changes nothing.
  sim::clock::advance();
  EXPECT_FALSE(eo.get_value(0));
  EXPECT_TRUE( eo.get_value(1));
  EXPECT_FALSE(eo.get_value(2));
}

/// Test both `external_inputs` and `external_outputs` using `set_value`.
TEST(sim_external_io, SetValue) {
  // Create a 3-port `external_outputs` connected to a 3-port
  // `external_inputs`, where output port 0, 1, and 2 are connected to input
  // ports 1, 0, 2.
  sim::external_inputs<3>  ei;
  sim::external_outputs<3> eo;
  eo.connect_input(0, &ei, 1);
  eo.connect_input(1, &ei, 0);
  eo.connect_input(2, &ei, 2);

  // Set new values on 2 of the ports.
  ei.set_value(1, true);
  ei.set_value(2, true);

  // Changes to `ei` are not reflected immediately in `eo`.
  EXPECT_FALSE(eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_FALSE(eo.get_value(2));

  // `execute` is invocable, but changes nothing.
  ei.execute();
  eo.execute();
  EXPECT_FALSE(eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_FALSE(eo.get_value(2));

  // Advancing the clock makes changes to `ei` visible through `eo`.
  sim::clock::advance();
  EXPECT_TRUE( eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_TRUE( eo.get_value(2));

  // Invert the values and check again
  ei.set_value(0, true);
  ei.set_value(1, false);
  ei.set_value(2, false);
  EXPECT_TRUE( eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_TRUE( eo.get_value(2));

  // `execute` changes nothing
  ei.execute();
  eo.execute();
  EXPECT_TRUE( eo.get_value(0));
  EXPECT_FALSE(eo.get_value(1));
  EXPECT_TRUE( eo.get_value(2));

  // Advancing the clock makes the latest changes visible.
  sim::clock::advance();
  EXPECT_FALSE(eo.get_value(0));
  EXPECT_TRUE( eo.get_value(1));
  EXPECT_FALSE(eo.get_value(2));
}

/// Test both `external_inputs` and `external_outputs` using
/// `set_all_values_immediate`. Though not best practice, it is easiest to test
/// both classes at once, as each is an observer for the other.
TEST(sim_external_io, SetAllValuesImmediate) {
  // Create a 3-port `external_outputs` connected to a 3-port
  // `external_inputs`, where output port 0, 1, and 2 are connected to input
  // ports 1, 0, 2.
  sim::external_inputs<3>  ei;
  sim::external_outputs<3> eo;
  eo.connect_input(0, &ei, 1);
  eo.connect_input(1, &ei, 0);
  eo.connect_input(2, &ei, 2);

  // Set new values on 2 of the ports.
  ei.set_all_values_immediate({false, true, true});

  // Changes to `ei` are reflected immediately in `eo`.
  EXPECT_EQ(eo.get_all_values(), (std::array{true, false, true}));

  // `execute` is invocable, but changes nothing.
  ei.execute();
  eo.execute();
  EXPECT_EQ(eo.get_all_values(), (std::array{true, false, true}));

  // Advancing the clock changes nothing.
  sim::clock::advance();
  EXPECT_EQ(eo.get_all_values(), (std::array{true, false, true}));

  // Invert the values and check again
  ei.set_all_values_immediate({true, false, false});
  EXPECT_EQ(eo.get_all_values(), (std::array{false, true, false}));

  // `execute` changes nothing
  ei.execute();
  eo.execute();
  EXPECT_EQ(eo.get_all_values(), (std::array{false, true, false}));

  // Advancing the clock changes nothing.
  sim::clock::advance();
  EXPECT_EQ(eo.get_all_values(), (std::array{false, true, false}));
}

/// Test both `external_inputs` and `external_outputs` using
/// `set_all_values`.
TEST(sim_external_io, SetAllValues) {
  // Create a 3-port `external_outputs` connected to a 3-port
  // `external_inputs`, where output port 0, 1, and 2 are connected to input
  // ports 1, 0, 2.
  sim::external_inputs<3>  ei;
  sim::external_outputs<3> eo;
  eo.connect_input(0, &ei, 1);
  eo.connect_input(1, &ei, 0);
  eo.connect_input(2, &ei, 2);

  // Set new values on 2 of the ports.
  ei.set_all_values({false, true, true});

  // Changes to `ei` are not reflected immediately in `eo`.
  EXPECT_EQ(eo.get_all_values(), (std::array{false, false, false}));

  // `execute` is invocable, but changes nothing.
  ei.execute();
  eo.execute();
  EXPECT_EQ(eo.get_all_values(), (std::array{false, false, false}));

  // Advancing the clock makes changes to `ei` visible through `eo`.
  sim::clock::advance();
  EXPECT_EQ(eo.get_all_values(), (std::array{true, false, true}));

  // Invert the values and check again
  ei.set_all_values({true, false, false});
  EXPECT_EQ(eo.get_all_values(), (std::array{true, false, true}));

  // `execute` changes nothing
  ei.execute();
  eo.execute();
  EXPECT_EQ(eo.get_all_values(), (std::array{true, false, true}));

  // Advancing the clock makes the latest changes visible.
  sim::clock::advance();
  EXPECT_EQ(eo.get_all_values(), (std::array{false, true, false}));
}

/// Test `external_pulse`
TEST(sim_external_io, ExternalPulse) {
  // Use `external_outputs` to read the result.
  sim::external_outputs<1> out;
  sim::external_pulse      ep;
  out.connect_input(0, &ep);

  // Run the equivalent of a simulation cycle (call execute on all gates, then
  // advance the clock) multiple times and verify that the pulse is correct.
  bool exp = false;
  while (sim::clock::value() < 10) {
    EXPECT_EQ(exp, out.get_value(0));
    ep.execute();
    EXPECT_EQ(exp, out.get_value(0));  // No change during cycle
    sim::clock::advance();
    exp = ! exp;
  }
}


// Local Variables:
// c-basic-offset: 2
// End:
