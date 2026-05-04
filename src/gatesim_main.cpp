/* gatesim_main.cpp                                                   -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_clock.h>
#include <sim_external_io.h>
#include <simple_gates.h>
#include <sim_mainloop.h>
#include <iostream>
#include <iomanip>
#include <span>

// Circuit to add two 2-bit unsigned binary numbers to yield 2-bits + carry
// (i.e., a 3-bit result). It consists of a half adder (one XOR gate and one
// AND gate) for the lower bits and one full adder (two XOR gates, two AND
// gates, and one OR gate) for the upper bits plus the carry from the lower
// bits.
simple_gates::XOR xor0;  // XOR for bit 0
simple_gates::AND and0;  // AND for bit 0
simple_gates::XOR xor1;  // XOR for bit 1
simple_gates::XOR xor1c; // XOR for bit 1 and the carry bit from bit 0
simple_gates::AND and1;  // AND for bit 1
simple_gates::AND and1c; // AND for bit 1 and the carry bit from bit 0
simple_gates::OR  or1cc; // OR  to yield carry bit from bit 1

// Inputs and outputs
sim::external_inputs<4>  in;  // bits 0 & 1 of addends A and B
sim::external_outputs<3> out; // bits 0, 1, and carry from result

// Name the inputs for clarity (Addend A bit 0, Addend A bit 1, etc.)
enum { A0, A1, B0, B1 };

// Wire up the circuit
const sim::circuit add2by2_circuit {
  in, out,
  {
    // Half adder sum of low bits
    { &xor0, { {&in, A0}, {&in, B0} } },  // low bit of sum S0 = A0 + A1
    { &and0, { {&in, A0}, {&in, B0} } },  // carry from low bits (C0)

    // Full adder sum of high bits without carry
    { &xor1, { {&in, A1}, {&in, B1} } },  // high bit of sum sans carry (S1)
    { &and1, { {&in, A1}, {&in, B1} } },  // carry from high bits (C1)

    // Full adder add in carry from low bits
    { &xor1c, { &xor1, &and0 } },  // S1 + C0
    { &and1c, { &xor1, &and0 } },  // Carry from S1 + C0

    // Generate the carry bit from bit 1
    { &or1cc,  { &and1, &and1c } },  // Final carry

    // Assign bits to output
    { &out,  { &xor0, &xor1c, &or1cc } }
  }
};

// Try a new set of addends every 4 cycles. Note that the bits in the addends
// are (counterintuitively) arranged low-bit first
const sim::event<4> in_events[] = {
  {  0, { 0, 0, 0, 0 } },
  {  4, { 1, 0, 0, 0 } },
  {  8, { 0, 1, 0, 0 } },
  { 12, { 1, 1, 0, 0 } },
  { 16, { 0, 0, 1, 0 } },
  { 20, { 1, 0, 1, 0 } },
  { 24, { 0, 1, 1, 0 } },
  { 28, { 1, 1, 1, 0 } },
  { 32, { 0, 0, 0, 1 } },
  { 36, { 1, 0, 0, 1 } },
  { 40, { 0, 1, 0, 1 } },
  { 44, { 1, 1, 0, 1 } },
  { 48, { 0, 0, 1, 1 } },
  { 52, { 1, 0, 1, 1 } },
  { 56, { 0, 1, 1, 1 } },
  { 60, { 1, 1, 1, 1 } }
};

int main()
{
  auto out_events = sim::main_loop(add2by2_circuit, std::span(in_events), 4);

  const auto in_size    = sizeof(in_events) / sizeof(in_events[0]);
  const auto out_size   = out_events.size();
  constexpr auto max_ts = sim::clock::mask; // Max timestamp

  // Pretty-print input and output events.
  std::size_t in_idx = 0, out_idx = 0;
  while (in_idx < in_size || out_idx < out_size)
  {
    auto in_ts  = (in_idx < in_size) ? in_events[in_idx].m_timestamp   : max_ts;
    auto out_ts = (out_idx<out_size) ? out_events[out_idx].m_timestamp : max_ts;
    if (in_ts < out_ts) {
      // Print input event only
      const auto& in_vals = in_events[in_idx].m_values;
      std::cout << std::setw(2) << in_ts << ": in["
                << int(in_vals[A1]) << int(in_vals[A0]) << " + "
                << int(in_vals[B1]) << int(in_vals[B0]) << "]\n";
      ++in_idx;
    }
    else if (out_ts < in_ts) {
      // Print output event only
      const auto& out_vals = out_events[out_idx].m_values;
      std::cout << std::setw(2) << out_ts << ":             out["
                << int(out_vals[2]) << int(out_vals[1]) << int(out_vals[0])
                << "]\n";
      ++out_idx;
    }
    else {
      // Both input and output events have changed in the same clock cycle
      const auto& in_vals  = in_events[in_idx].m_values;
      const auto& out_vals = out_events[out_idx].m_values;
      std::cout << std::setw(2) << in_ts << ": in["
                << int(in_vals[A1]) << int(in_vals[A0]) << " + "
                << int(in_vals[B1]) << int(in_vals[B0]) << "] out["
                << int(out_vals[2]) << int(out_vals[1]) << int(out_vals[0])
                << "]\n";
      ++in_idx;
      ++out_idx;
    }
  }
}

// Local Variables:
// c-basic-offset: 2
// End:
