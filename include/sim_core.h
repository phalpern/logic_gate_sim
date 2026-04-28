/* sim_core.h                                                         -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_SIM_CORE
#define INCLUDED_SIM_CORE

#include <atomic>

namespace sim {

/// Maintain a global simulated clock, measured in arbitrary "ticks". The clock
/// must be advanced manually by the simulator machinery; system time is not
/// consulted. All methods of this class are static.
class time {
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
  static value_type clock() { return m_ticks; }

  /// Advance the clock by one clock tick.
  static void advance_clock() { ++m_ticks; }

  /// Reset the clock back to zero (e.g., for restarting simulations)
  static void reset_clock() { m_ticks = 0; }
};

/// Each gate has zero or more input leads and output leads. A gate's input
/// leads mush each be connected to one output lead of (the same or another
/// gate). Conversely each output lead from a gate can be connected to zero or
/// more input leads. There is no separate object representing the interconnect
/// (wire) between input and output leads; the connection is represented
/// entirely within the `input_lead` data structure.
class input_lead;  // Forward declaration

/// Model an output lead. Writing a value to the output is not seen by any
/// connected `input_lead` until the start of the next clock cycle (i.e., when
/// `time::advance_clock()` is called). An `output_lead` is neither copyable
/// nor movable, ensuring that connected `input_lead` objects do not become
/// disconnected. Only one writer is assumed, and it is not safe to modify an
/// object of this type concurrently, but it *is* safe to concurrently read
/// from a corresponding `input_lead` while modifying an `output_lead`; the
/// reader always sees the value as of the previous clock cycle.
class output_lead {
  friend class input_lead;

  /// `m_data` is a packed integral value whose high bit indicates the most
  /// recently set Boolean value for this output and the remaining bits
  /// indicate time stamp (according to `time::clock()` at which value was last
  /// toggled. The corresponding `input_lead` will not use the new value unless
  /// the timestamp is at least one cycle old.
  std::atomic<time::value_type> m_data{};

public:
  constexpr output_lead() = default;
  constexpr ~output_lead() = default;

  // Not copyable or movable
  output_lead(const output_lead&) = delete;
  output_lead& operator=(const output_lead&) = delete;

  /// Set a new value. This operation is thread safe with respect to concurrent
  /// reads via connected `input_lead` objects, but is not safe from concurrent
  /// `set` operations.
  void set(bool v)
  {
    // It is safe to use relaxed loads because there is a single writer, thus
    // any store would be sequenced within the same thread.
    auto data = m_data.load(std::memory_order::relaxed);
    auto value_bit = data & time::high_bit;

    if (static_cast<bool>(value_bit) == v)
      return;  // Value did not change

    // Store the current clock value, toggling the previous value of the high
    // bit.
    data = (value_bit ^ time::high_bit) | time::clock();

    // It is safe to use relaxed stores because a change to the clock is always
    // synchronized with this operation. A load from an `input_lead` within the
    // clock cycle will see either the old value or the new value, but the
    // result will always be the same as reading the old value. A load in a
    // subsequent clock cycle is synchronized with the `time::advance` call,
    // and will thus always see the new value.
    m_data.store(data, std::memory_order::relaxed);
  }
};

#if 0
class GateBase
{
protected:

public:

  // Connect
  void connectInput(unsigned        input_idx,
                    const GateBase* sourceGate,
                    unsigned        output_idx);
};
#endif

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_CORE)

// Local Variables:
// c-basic-offset: 2
// End:
