/* sim_mainloop.h                                                     -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * @brief Implementation of main simulation loop
 *
 * The main loop takes two inputs:
 * 1. The input circuit in the form of an array of pointers to gates,
 * 2. A sequence of `input_event` objects, storing the value of the input
 *    ports over time.
 *
 * The loop executes until some number of cycles after all inputs are connsumed
 * (which number is specified as a parameter). Each iteration of the loop
 * detects whether the output has changed and, if so writes the output values
 * to the specified output stream.
 */

#ifndef INCLUDED_SIM_MAINLOOP
#define INCLUDED_SIM_MAINLOOP

#include <sim_gate_base.h>
#include <sim_external_io.h>

#include <array>
#include <span>
#include <ranges>
#include <vector>

namespace sim {

template <std::size_t SZ>
struct input_event
{
  clock::value_type     m_timestamp;
  std::array<bool, SZ>  m_values;

  auto operator<=>(const input_event&) const = default;
};

template <std::size_t SZ>
struct output_event
{
  clock::value_type    m_timestamp;
  std::array<bool, SZ> m_values;

  auto operator<=>(const output_event&) const = default;
};

/// Executes the main simulation loop. On each clock cycle, run `execute` on
/// each gate in `circuit` and write any output-port changes to the specified
/// output stream. The `in_events` range specifies a timestamp and set of
/// input-port values to be set when the clock reaches that timestamp; it must
/// be sorted in increasing order of timestamp.
template <std::size_t NumIn, std::size_t NumOut,
          std::ranges::forward_range EventRange>
std::vector<output_event<NumOut>>
main_loop(std::span<gate*>                circuit,
          external_inputs<NumIn>&         in_ports,
          external_outputs<NumOut> const& out_ports,
          EventRange                      in_events,
          clock::value_type               extra_ticks)
{
  std::vector<output_event<NumOut>> out_events;

  auto last_out = out_ports.get_all_values();
  out_events.emplace_back(0, last_out);

  for (auto& event : in_events) {
    assert(clock::value() <= event.m_timestamp);
    while (clock::value() < event.m_timestamp) {
      for (gate* g : circuit)
        g->execute();
      clock::advance();
      if (auto new_out = out_ports.get_all_values(); last_out != new_out) {
        out_events.emplace_back(clock::value(), new_out);
        last_out = new_out;
      }
    }
    in_ports.set_all_values_immediate(event.m_values);
  }

  // Run `extra_ticks` more loops
  for (clock::value_type t = 0; t < extra_ticks; ++t) {
    for (gate* g : circuit)
      g->execute();
    clock::advance();
    if (auto new_out = out_ports.get_all_values(); last_out != new_out) {
      out_events.emplace_back(clock::value(), new_out);
      last_out = new_out;
    }
  }

  return out_events;
}

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_MAINLOOP)

// Local Variables:
// c-basic-offset: 2
// End:
