/* sim_clock.h                                                        -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_SIM_CLOCK
#define INCLUDED_SIM_CLOCK

#include <atomic>
#include <array>
#include <cassert>

namespace sim {

/// Maintain a global simulated clock, measured in arbitrary "ticks". The clock
/// must be advanced manually by the simulator machinery; system time is not
/// consulted. All methods of this class are static.
class clock {
public:
  // TYPES
  using value_type = unsigned long;

private:
  // CLASS DATA
  static value_type m_ticks;  ///< Number of ticks since program started

public:
  // CONSTANTS

  /// The high bit of the tick count is reserved for the user.  The following
  /// constants allow the user to mask off the high bit or isolate the high
  /// bit, respectively.
  static constexpr value_type mask     = ~static_cast<value_type>(0) >> 1;
  static constexpr value_type high_bit = mask + 1;

  /// Return number of ticks since program start.
  static value_type value() { return m_ticks; }

  /// Advance the clock by one clock tick.
  static void advance() { ++m_ticks; }

  /// Reset the clock back to zero (e.g., for restarting simulations)
  static void reset() { m_ticks = 0; }
};

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_CLOCK)

// Local Variables:
// c-basic-offset: 2
// End:
