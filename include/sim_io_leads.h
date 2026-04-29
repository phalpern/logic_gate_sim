/* sim_io_leads.h                                                     -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_SIM_IO_LEADS
#define INCLUDED_SIM_IO_LEADS

#include <sim_clock.h>

namespace sim {

/// Each gate has zero or more input leads and output leads. A gate's input
/// leads mush each be connected to one output lead of (the same or another
/// gate). Conversely each output lead from a gate can be connected to zero or
/// more input leads. There is no separate object representing the interconnect
/// (wire) between input and output leads; the connection is represented
/// entirely within the `input_lead` data structure.
class input_lead;  // Forward declaration

/// Model an output lead. Writing a value to the output is not seen by any
/// connected `input_lead` until the start of the next clock cycle (i.e., when
/// `clock::advance()` is called). An `output_lead` is neither copyable
/// nor movable, ensuring that connected `input_lead` objects do not become
/// disconnected. Only one writer is assumed, and it is not safe to modify an
/// object of this type concurrently, but it *is* safe to concurrently read
/// from a connected `input_lead` while modifying an `output_lead`; the
/// reader always sees the value as of the previous clock cycle.
class output_lead {

  friend class input_lead;

  /// `m_data` is a packed integral value whose high bit indicates the most
  /// recently set Boolean value for this output and the remaining bits
  /// timestamp (according to `clock::value()`) for the tick after which value
  /// was last toggled. A connected `input_lead` will not use the new value
  /// unless the stored timestamp no later than the current time.
  std::atomic<clock::value_type> m_data{clock::value()};

public:
  output_lead() = default;
  constexpr ~output_lead() = default;

  /// Initialize with specified `v` value.
  explicit output_lead(bool v)
    : m_data((v ? clock::high_bit : 0) | clock::value()) { }

  // Not copyable or movable
  output_lead(const output_lead&) = delete;
  output_lead& operator=(const output_lead&) = delete;

  /// Set a new value. This operation is thread safe with respect to concurrent
  /// reads via connected `input_lead` objects, but is not safe from concurrent
  /// `set` operations.
  void set(bool v);
};

/// Model an input lead. Reading a value from an input does not see changes to
/// any connected `output_lead` until the start of the next clock cycle (i.e.,
/// when `clock::advance()` is called). An `input_lead` is neither
/// copyable nor movable, ensuring that connected `output_lead` objects do not
/// become disconnected. It *is* safe to read the value of an `input_lead`
/// while the connected `output_lead` is beeing concurrently modified; the
/// reader always sees the value as of the previous clock cycle.
class input_lead
{
  const output_lead* m_source = nullptr;

public:
  constexpr input_lead() = default;
  constexpr ~input_lead() = default;

  // Not copyable or movable
  input_lead(const input_lead&) = delete;
  input_lead& operator=(const input_lead&) = delete;

  /// Connect this input lead to the specified `src` output lead (usually from
  /// a different gate), effectively creating a "wire" connecting the gates.
  /// Precondition: `src` is not null.
  /// Precondition: this lead is not already connected to another.
  void connect_to(const output_lead* src);

  /// Read the value on this lead. The returned Boolean is the value that was
  /// set on the connected `output_lead` during the previous clock cycle. It is
  /// safe to call this function while the connected `output_lead` is being
  /// modified concurrently; the changed value is not seen until the next clock
  /// cycle. The behavior is undefined if this object is not connected to a
  /// valid `output_lead`.
  bool get() const;
};

///////////////////////////////////////////////////////////////////////////////
//           Inline and template implementations below this line             //
///////////////////////////////////////////////////////////////////////////////

inline
void output_lead::set(bool v)
{
  // It is safe to use relaxed loads because there is a single writer, thus any
  // loads and stores within this function would be sequenced within the same
  // thread.
  auto data = m_data.load(std::memory_order::relaxed);
  auto value_bit = data & clock::high_bit;

  if (static_cast<bool>(value_bit) == v)
    return;  // Value did not change

  // Store the next clock value, toggling the previous value of the high bit.
  data = (value_bit ^ clock::high_bit) | (clock::value() + 1);

  // Store the new data. It is safe to use a relaxed store because calls to
  // `output_lead::set` and `input_lead::get`, though not sequenced with
  // respect to each other, *are strictly sequenced* with respect to calls to
  // `clock::advance()`. The algorithm is designed such that, if a set and
  // get occur within the same clock cycle, it doesn't matter whether or not
  // the new data is immediately visble. Conversely, sequencing ensures that
  // the new data is always available after an intervening call to
  // `clock::advance()`,
  m_data.store(data, std::memory_order::relaxed);
}

inline
void input_lead::connect_to(const output_lead* src)
{
  assert(src != nullptr);       // Precondition check
  assert(m_source == nullptr);  // Precondition check
  m_source = src;
}

inline
bool input_lead::get() const
{
  assert(m_source); // Precondition check

  // Load the current data from the connected output lead.  It is safe to use a
  // relaxed load because calls to `output_lead::set` and `input_lead::get`,
  // though not sequenced with respect to each other, *are strictly sequenced*
  // with respect to calls to `clock::advance()`. The algorithm is designed
  // such that, if a set and get occur within the same clock cycle, it doesn't
  // matter whether or not the new data is immediately visble. Conversely,
  // sequencing ensures that the new data is always available after an
  // intervening call to `clock::advance()`,
  auto data = m_source->m_data.load(std::memory_order::relaxed);

  // Extract the current value from the high bit
  bool current_value = (data & clock::high_bit) != 0;

  // Extract the timestamp from the lower bits
  clock::value_type timestamp = data & clock::mask;

  // If the timestamp is in the future, return the previous value (inverted
  // current value); otherwise return the current value.
  if (timestamp > clock::value()) {
    return !current_value;
  } else {
    return current_value;
  }
}

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_IO_LEADS)

// Local Variables:
// c-basic-offset: 2
// End:
