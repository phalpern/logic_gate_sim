/* sim_external_io.cpp                                                -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_external_io.h>

namespace sim {

void external_pulse::execute()
{
  // Set to `true` if the *next* clock value is odd.
  m_outputs[0].set((clock::value() + 1) & 1);
}

} // close namespace sim

// Local Variables:
// c-basic-offset: 2
// End:
