/* sim_mainloop.t.cpp                                                 -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_mainloop.h>
#include <sim_external_io.h>
#include <simple_gates.h>
#include <gtest/gtest.h>

TEST(sim_mainloop, Test1) {

  // construct a simple circuit for (!A && B), recording the output of the NOT
  // gate and of the AND gate.
  sim::external_inputs<2>  in;
  sim::external_outputs<2> out;
  simple_gates::NOT        not_gate;
  simple_gates::AND        and_gate;
  not_gate.connect_input(0, &in, 0);
  and_gate.connect_input(0, &not_gate);
  and_gate.connect_input(1, &in, 1);
  out.connect_input(0, &not_gate);
  out.connect_input(1, &and_gate);
  std::array<sim::gate*, 2> circuit{&not_gate, &and_gate};

  // Create an array of events
  sim::input_event<in.num_outputs()> in_events[] = {
    { 0,  { false, false } },
    { 16, { true,  false } },
    { 32, { false, true  } },
    { 64, { true,  true  } }
  };

  auto result = sim::main_loop(circuit, in, out, std::span(in_events), 16);
  EXPECT_EQ(result[ 0], (sim::output_event<2>{ 0, { false, false }}));
  EXPECT_EQ(result[ 1], (sim::output_event<2>{ 1, { true,  false }}));
  EXPECT_EQ(result[ 2], (sim::output_event<2>{17, { false, false }}));
  EXPECT_EQ(result[ 3], (sim::output_event<2>{33, { true,  false }}));
  EXPECT_EQ(result[ 4], (sim::output_event<2>{34, { true,  true  }}));
  EXPECT_EQ(result[ 5], (sim::output_event<2>{65, { false, true  }}));
  EXPECT_EQ(result[ 6], (sim::output_event<2>{66, { false, false }}));
}

// Local Variables:
// c-basic-offset: 2
// End:
