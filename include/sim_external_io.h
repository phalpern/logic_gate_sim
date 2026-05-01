/* sim_external_io.h                                                  -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * @brief Special gates simulating external inputs and outputs
 */

#ifndef INCLUDED_SIM_EXTERNAL_IO
#define INCLUDED_SIM_EXTERNAL_IO

#include <sim_gate_base.h>

namespace sim {

/// Array of `SZ` Boolean ports representing external input values (i.e. the
/// inputs to the simulation).
template <std::size_t SZ>
class external_inputs : public gate_with_leads<0, SZ>
{
public:
  static constexpr std::size_t size = SZ;

  external_inputs() = default;

  /// Set the value of the input port specified by `index`. The value is
  /// visible at the start of the next clock cycle.
  void set_value(std::size_t index, bool value)
  {
    assert(index < SZ);  // precondition check
    this->m_outputs[index].set(value);
  }

  /// Set the value of the output port specified by `index`. The value is
  /// visible immediately, without waiting for the next clock cycle.
  void set_value_immediate(std::size_t index, bool value)
  {
    assert(index < SZ);  // precondition check
    this->m_outputs[index].set_immediate(value);
  }

  /// Set the value of input output leads at once from an single array. The
  /// values are visible at the start of the next clock cycle.
  void set_all_values(const std::array<bool, SZ>& values)
  {
    for (std::size_t i = 0; i < SZ; ++i)
      this->m_outputs[i].set(values[i]);
  }

  /// Set the value of input output leads at once from an single array. The
  /// values are visible immediately, without waiting for the next clock cycle.
  void set_all_values_immediate(const std::array<bool, SZ>& values)
  {
    for (std::size_t i = 0; i < SZ; ++i)
      this->m_outputs[i].set_immediate(values[i]);
  }

  /// Do nothing on execute.
  void execute() override;
};

/// Array of `SZ` data sinks representing external output ports (i.e. the
/// outputs from the simulation).
template <std::size_t SZ>
class external_outputs : public gate_with_leads<SZ, 0>
{
public:
  external_outputs() = default;

  /// Get the value of the output lead to which input port specified by `index`
  /// is connected.
  bool get_value(std::size_t index) const
  {
    assert(index < SZ);  // precondition check
    return this->m_inputs[index].get();
  }

  /// Retrieve the value of all output leads in a single array.
  std::array<bool, SZ> get_all_values() const
  {
    std::array<bool, SZ> ret;
    for (std::size_t i = 0; i < SZ; ++i)
      ret[i] = this->m_inputs[i].get();
    return ret;
  }

  /// Do nothing on execute.
  void execute() override;
};

/// Output-only gate yielding a pulse every 2 ticks. The single output lead has
/// a value of `false` when `sim::clock::ticks()` is even and `true` when it's
/// odd.
class external_pulse : public gate_with_leads<0, 1>
{
public:
  /// Update the output on the next clock cycle.
  void execute() override;
};

///////////////////////////////////////////////////////////////////////////////
//           Inline and template implementations below this line             //
///////////////////////////////////////////////////////////////////////////////

template <std::size_t SZ>
void external_inputs<SZ>::execute()
{
}

template <std::size_t SZ>
void external_outputs<SZ>::execute()
{
}

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_EXTERNAL_IO)

// Local Variables:
// c-basic-offset: 2
// End:
