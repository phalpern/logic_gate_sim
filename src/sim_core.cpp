/* sim_core.cpp                                                       -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_core.h>

namespace sim {

time::value_type time::m_ticks = 0;

template <std::size_t NI, std::size_t NO>
const output_lead* gate_with_IO<NI,NO>::get_output_lead(unsigned idx) const
{
  assert(idx < NO);  // Precondition check
  return m_outputs[idx];
}

template <std::size_t NI, std::size_t NO>
void gate_with_IO<NI,NO>::connect_input(unsigned    input_idx,
                                        const gate* src_gate,
                                        unsigned    src_output_idx)
{
  assert(input_idx < NI);        // Precondition check
  assert(src_gate != nullptr);  // Precondition check

  m_inputs[input_idx].connect_to(src_gate->get_output_lead(src_output_idx));
}

gate::~gate() { }

} // close namespace sim

// Local Variables:
// c-basic-offset: 2
// End:
