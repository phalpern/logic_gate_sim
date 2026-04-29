/* sim_core.h                                                         -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_SIM_CORE
#define INCLUDED_SIM_CORE

#include <atomic>
#include <array>
#include <cassert>

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
/// from a connected `input_lead` while modifying an `output_lead`; the
/// reader always sees the value as of the previous clock cycle.
class output_lead {

  friend class input_lead;

  /// `m_data` is a packed integral value whose high bit indicates the most
  /// recently set Boolean value for this output and the remaining bits
  /// timestamp (according to `time::clock()`) for the tick after which value
  /// was last toggled. A connected `input_lead` will not use the new value
  /// unless the stored timestamp no later than the current time.
  std::atomic<time::value_type> m_data{time::clock()};

public:
  output_lead() = default;
  constexpr ~output_lead() = default;

  /// Initialize with specified `v` value.
  explicit output_lead(bool v)
    : m_data((v ? time::high_bit : 0) | time::clock()) { }

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
/// when `time::advance_clock()` is called). An `input_lead` is neither
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
  void connect_to(const output_lead* src)
  {
    assert(src != nullptr);       // Precondition check
    assert(m_source == nullptr);  // Precondition check
    m_source = src;
  }

  /// Read the value on this lead. The returned Boolean is the value that was
  /// set on the connected `output_lead` during the previous clock cycle. It is
  /// safe to call this function while the connected `output_lead` is being
  /// modified concurrently; the changed value is not seen until the next clock
  /// cycle. The behavior is undefined if this object is not connected to a
  /// valid `output_lead`.
  bool get() const;
};

/// Abstract Base class for logic gates. Each gate has some number of input
/// leads, some number of output leads (each having an integral index), and an
/// `execute` method for performing one tick worth of the logic of the
/// gate. There is also a method for connecting the input leads to the output
/// leads of another gate, i.e., to create a (possibly cyclic) directed graph
/// of gates representing the entire logic circuit.
class gate
{
public:
  constexpr gate() = default;

  /// To avoid slicing, this (and most polymorphic classes) should not be
  /// copyable or movable.
  gate(const gate&) = delete;
  gate& operator=(const gate&) = delete;

  /// (Virtual) destructor does nothing and it is expected that derived-class
  /// desstructors will do nothing, too.
  virtual ~gate();

  /// Return a pointer to the output lead specified by `idx`.
  /// Precondition: `idx` is in range for this gate.
  virtual const output_lead* get_output_lead(unsigned idx) const = 0;

  /// Connect this gete's `input_lead`, as specified by `input_idx` to the
  /// `output_lead` specified by `src_gate` and `src_output_idx`.
  /// Preconditions: `src_gate` is non-null and `input_idx` and
  /// `src_output_idx` are in range for their respective gates.
  virtual void connect_input(unsigned    input_idx,
                             const gate* src_gate,
                             unsigned    src_output_idx) = 0;

  /// Do the actual work of the gate. The postcondition is that one tick worth
  /// of work was done, which might or might not result in a change to one
  /// or more output leads.
  virtual void execute() = 0;
};

/// A partial implemention (still abstract) of the `gate` protocol, handling
/// all of the interconnection logic. A class derived from `gate_with_IO<In,
/// Out>` has `In` input leads and `Out` output leads. A derived class that is
/// made concrete simply by defining the `execute` method.
template <std::size_t NumInputs, std::size_t NumOutputs>
class gate_with_IO : public gate
{
protected:
  std::array<input_lead,  NumInputs>  m_inputs;
  std::array<output_lead, NumOutputs> m_outputs;

public:
  // Rule of 0. Implicitly non-copyable

  const output_lead* get_output_lead(unsigned idx) const override;

  void connect_input(unsigned    input_idx,
                     const gate* src_gate,
                     unsigned    src_output_idx) override;
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
  auto value_bit = data & time::high_bit;

  if (static_cast<bool>(value_bit) == v)
    return;  // Value did not change

  // Store the next clock value, toggling the previous value of the high bit.
  data = (value_bit ^ time::high_bit) | (time::clock() + 1);

  // Store the new data. It is safe to use a relaxed store because calls to
  // `output_lead::set` and `input_lead::get`, though not sequenced with
  // respect to each other, *are strictly sequenced* with respect to calls to
  // `time::advance_clock`. The algorithm is designed such that, if a set and
  // get occur within the same clock cycle, it doesn't matter whether or not
  // the new data is immediately visble. Conversely, sequencing ensures that
  // the new data is always available after an intervening call to
  // `time::advance_clock`,
  m_data.store(data, std::memory_order::relaxed);
}

inline
bool input_lead::get() const
{
  assert(m_source); // Precondition check

  // Load the current data from the connected output lead.  It is safe to use a
  // relaxed load because calls to `output_lead::set` and `input_lead::get`,
  // though not sequenced with respect to each other, *are strictly sequenced*
  // with respect to calls to `time::advance_clock`. The algorithm is designed
  // such that, if a set and get occur within the same clock cycle, it doesn't
  // matter whether or not the new data is immediately visble. Conversely,
  // sequencing ensures that the new data is always available after an
  // intervening call to `time::advance_clock`,
  auto data = m_source->m_data.load(std::memory_order::relaxed);

  // Extract the current value from the high bit
  bool current_value = (data & time::high_bit) != 0;

  // Extract the timestamp from the lower bits
  time::value_type timestamp = data & time::mask;

  // If the timestamp is in the future, return the previous value (inverted
  // current value); otherwise return the current value.
  if (timestamp > time::clock()) {
    return !current_value;
  } else {
    return current_value;
  }
}

template <std::size_t NI, std::size_t NO>
const output_lead* gate_with_IO<NI,NO>::get_output_lead(unsigned idx) const
{
  assert(idx < NO);  // Precondition check
  return &m_outputs[idx];
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

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_CORE)

// Local Variables:
// c-basic-offset: 2
// End:
