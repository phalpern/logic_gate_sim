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

/// Designate a specific output lead of a specific gate.
struct source_lead_designator
{
  const gate* m_src_gate_p;
  std::size_t m_src_output_idx;

  /// Construct from a pointer-to-gate and index. Most gates have only a single
  /// output lead, so `out_idx` is optional. Without the index, this
  /// constructor can perform an implicit conversion from a gate pointer, which
  /// is convenient when constructing an array of designators from a list of
  /// gates. This object retains a pointer to the gate and passes it on to
  /// other gates, which in turn retain the pointer, so users must be careful
  /// of lifetimes, but this is not typically an issue because gates are
  /// non-movable and are live for the entire simulation.
  source_lead_designator(const gate* src, std::size_t out_idx = 0)
    : m_src_gate_p(src), m_src_output_idx(out_idx) { }
};

/// An encoding of the input connections for a single gate. The designators in
/// the `m_connect_to` vector are in order of input ports for `m_gate_p`. Note
/// that this class is used only when constructing a circuit; not when running
/// the simulation, so the allocaiton overhead for `std::vector` is not in the
/// hot path.
struct gate_connections {
  gate*                               m_dest_gate_p;
  std::vector<source_lead_designator> m_connect_to;
};

/// A description of a circuit, comprising its external inputs and outputs, its
/// gates, and the connections among the gates.
template <std::size_t NumIn, std::size_t NumOut>
class circuit {
  external_inputs<NumIn>&         m_in_ports;
  external_outputs<NumOut> const& m_out_ports;
  std::vector<gate*>              m_gates;

public:
  /// Construct a circuit from a set of input ports, output ports, gates and
  /// connections. This constructor works with CTAD so that the template
  /// parameters need not be explicitly specified.
  constexpr circuit(external_inputs<NumIn>&                 in_ports,
                    external_outputs<NumOut> const&         out_ports,
                    std::initializer_list<gate_connections> connections);

  constexpr external_inputs<NumIn>& in_ports() const { return m_in_ports; }
  constexpr external_outputs<NumOut> const& out_ports() const
    { return m_out_ports; }
  constexpr std::span<gate *const> gates() const { return m_gates; }
};

/// An `event` represents a change of state for a set of inputs or outputs (but
/// not both in one object). It consists of a timestamp and a set of values
/// that are represent the state of the I/O ports at that time.
template <std::size_t SZ>
struct event
{
  clock::value_type     m_timestamp;
  std::array<bool, SZ>  m_values;

  constexpr auto operator<=>(const event&) const = default;
};

/// Execute the main simulation loop. On each clock cycle, run `execute` on
/// each gate in `circuit` and write any output-port changes to the specified
/// output stream. The `in_events` range specifies a timestamp and set of
/// input-port values to be set when the clock reaches that timestamp; it must
/// be sorted in increasing order of timestamp.
template <std::size_t NumIn, std::size_t NumOut,
          std::ranges::forward_range InEventRange>
  requires std::same_as<event<NumIn>, std::ranges::range_value_t<InEventRange>>
std::vector<event<NumOut>>
main_loop(const circuit<NumIn, NumOut>& the_circuit,
          const InEventRange&           in_events,
          clock::value_type             extra_ticks);

///////////////////////////////////////////////////////////////////////////////
//           Inline and template implementations below this line             //
///////////////////////////////////////////////////////////////////////////////

template <std::size_t NumIn, std::size_t NumOut>
constexpr circuit<NumIn, NumOut>::circuit(
    external_inputs<NumIn>&                 in_ports,
    external_outputs<NumOut> const&         out_ports,
    std::initializer_list<gate_connections> connections)
  : m_in_ports(in_ports), m_out_ports(out_ports)
{
  m_gates.reserve(connections.size());
  for (const gate_connections& connect : connections) {
    gate* dest = connect.m_dest_gate_p;
    unsigned dest_input_idx = 0;
    if (dest != &out_ports)
      m_gates.push_back(dest);
    for (const source_lead_designator& source : connect.m_connect_to)
      dest->connect_input(dest_input_idx++, source.m_src_gate_p,
                          source.m_src_output_idx);
  }
}


template <std::size_t NumIn, std::size_t NumOut,
          std::ranges::forward_range InEventRange>
  requires std::same_as<event<NumIn>, std::ranges::range_value_t<InEventRange>>
std::vector<event<NumOut>>
main_loop(const circuit<NumIn, NumOut>& the_circuit,
          const InEventRange&           in_events,
          clock::value_type             extra_ticks)
{
  external_inputs<NumIn>&         in_ports  = the_circuit.in_ports();
  external_outputs<NumOut> const& out_ports = the_circuit.out_ports();
  std::vector<event<NumOut>>      out_events;

  auto last_out = out_ports.get_all_values();
  out_events.emplace_back(0, last_out);

  for (auto& event : in_events) {
    assert(clock::value() <= event.m_timestamp);
    while (clock::value() < event.m_timestamp) {
      for (gate* g : the_circuit.gates())
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
    for (gate* g : the_circuit.gates())
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
