/* simple_gates.t.cpp                                                 -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <simple_gates.h>
#include <sim_external_io.h>
#include <gtest/gtest.h>

/// Test a gate of type `GateT`.
template <class GateT, std::size_t TT_Sz>
void TestGate(const bool (&truth_table)[TT_Sz])
{
  // A gate with 1 input has a truth table with 2 elements; a gate with 2
  // inputs has a truth table with 4 elements.
  constexpr std::size_t num_in = TT_Sz == 4 ? 2 : 1;

  GateT                        the_gate;
  sim::external_inputs<num_in> in;
  sim::external_outputs<1>     out;
  out.connect_input(0, &the_gate);
  the_gate.connect_input(0, &in, 0);
  if constexpr (num_in == 2) {
    the_gate.connect_input(1, &in, 1); // 2-input gate has 4-element TT
  }

  // Simulate the exectution loop. Note that the input deliberately wraps, so
  // the last input state is the state is the same as the initial state.
  for (unsigned i = 0; i <= TT_Sz; ++i) {
    bool in0 = i & 1;
    bool in1 = i & 2;
    in.set_value_immediate(0, in0);
    if constexpr (num_in == 2) {
      in.set_value_immediate(1, in1);
    }
    the_gate.execute();
    sim::clock::advance();

    bool exp = truth_table[i % TT_Sz];
    EXPECT_EQ(exp, out.get_value(0));
  }
}

TEST(simple_gates, NOTGate) {
  static constexpr bool truth_table[] = {
    //
    // in0
    /*  0 */ 1,
    /*  1 */ 0
  };

  TestGate<simple_gates::NOT>(truth_table);
}

TEST(simple_gates, ANDGate) {
  static constexpr bool truth_table[] = {
    //        in0
    // in1   -----
    /*  0 */ 0, 0,
    /*  1 */ 0, 1
  };

  TestGate<simple_gates::AND>(truth_table);
}

////////////// The remaining tests were implemented by AI /////////////////////

TEST(simple_gates, ORGate) {
  static constexpr bool truth_table[] = {
    //        in0
    // in1   -----
    /*  0 */ 0, 1,
    /*  1 */ 1, 1
  };

  TestGate<simple_gates::OR>(truth_table);
}

TEST(simple_gates, NANDGate) {
  static constexpr bool truth_table[] = {
    //        in0
    // in1   -----
    /*  0 */ 1, 1,
    /*  1 */ 1, 0
  };

  TestGate<simple_gates::NAND>(truth_table);
}

TEST(simple_gates, NORGate) {
  static constexpr bool truth_table[] = {
    //        in0
    // in1   -----
    /*  0 */ 1, 0,
    /*  1 */ 0, 0
  };

  TestGate<simple_gates::NOR>(truth_table);
}

TEST(simple_gates, XORGate) {
  static constexpr bool truth_table[] = {
    //        in0
    // in1   -----
    /*  0 */ 0, 1,
    /*  1 */ 1, 0
  };

  TestGate<simple_gates::XOR>(truth_table);
}

// Local Variables:
// c-basic-offset: 2
// End:
