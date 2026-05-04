/* sim_mainloop.t.cpp                                                 -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_mainloop.h>
#include <sim_external_io.h>
#include <simple_gates.h>
#include <gtest/gtest.h>

TEST(sim_mainloop, AndNot) {

  // construct a simple `circuit` for (!A && B), recording the output of the
  // NOT gate and of the AND gate.
  sim::external_inputs<2>  in;
  sim::external_outputs<2> out;
  simple_gates::NOT        not_gate;
  simple_gates::AND        and_gate;

  const sim::circuit test_circuit{
    in, out,
    {
      { &not_gate, { &in } },
      { &and_gate, { &not_gate, { &in, 1 } } },
      { &out,      { &not_gate, &and_gate } }
    }
  };

  // Create an array of input events
  const sim::event<2> in_events[] = {
    { 0,  { false, false } },
    { 16, { true,  false } },
    { 32, { false, true  } },
    { 64, { true,  true  } }
  };

  // Run the main loop
  auto result = sim::main_loop(test_circuit, in_events, 16);

  // Verify results.
  EXPECT_EQ(result[ 0], (sim::event<2>{ 0, { false, false }}));
  EXPECT_EQ(result[ 1], (sim::event<2>{ 1, { true,  false }}));
  EXPECT_EQ(result[ 2], (sim::event<2>{17, { false, false }}));
  EXPECT_EQ(result[ 3], (sim::event<2>{33, { true,  false }}));
  EXPECT_EQ(result[ 4], (sim::event<2>{34, { true,  true  }}));
  EXPECT_EQ(result[ 5], (sim::event<2>{65, { false, true  }}));
  EXPECT_EQ(result[ 6], (sim::event<2>{66, { false, false }}));
}

TEST(sim_mainloop, Cyclic) {

  // Construct a simple `circuit` that contains a cycle, whereby a `NOT` gate
  // feeds back to its own input through an `OR` gate, resulting in an
  // oscillation. The other lead of the `OR` gate starts out biased `true`, the
  // output is a stable `false` until that lead is set `false`.
  sim::clock::reset();
  sim::external_inputs<1>  in;
  sim::external_outputs<1> out;
  simple_gates::NOT        not_gate;
  simple_gates::OR         or_gate;

  const sim::circuit cyclic_circuit{
    in, out,
    {
      { &not_gate, { &or_gate } },
      { &or_gate,  { &not_gate, &in } },
      { &out,      { &not_gate } }
    }
  };

  const sim::event<1> in_events[] = {
    {  0, { true  } },
    {  5, { false } },
    { 11, { true  } }
  };

  // Run the main loop.
  auto result = sim::main_loop(cyclic_circuit, in_events, 5);

  // Verify results.
  EXPECT_EQ(7, result.size());
  EXPECT_EQ(result[0], (sim::event<1>{ 0, { false }}));
  EXPECT_EQ(result[1], (sim::event<1>{ 1, { true  }}));
  EXPECT_EQ(result[2], (sim::event<1>{ 2, { false }})); // stable until cycle 7

  // Oscillate every other cycle from cycle 7 to cycle 13
  for (int c = 7, i = 3; c <= 13; c += 2, ++i)
    EXPECT_EQ(result[i],
              (sim::event<1>{ sim::clock::value_type(c), { bool(i & 1) } }));
}

// Local Variables:
// c-basic-offset: 2
// End:
