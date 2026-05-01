/* sim_gate_base.h                                                    -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * @brief Abstract base classes for implementing simulated gates
 */

#ifndef INCLUDED_SIM_GATE_BASE
#define INCLUDED_SIM_GATE_BASE

#include <sim_clock.h>
#include <sim_io_leads.h>

namespace sim {

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
/// all of the interconnection logic. A class derived from `gate_with_leads<In,
/// Out>` has `In` input leads and `Out` output leads. A derived class that is
/// made concrete simply by defining the `execute` method.
template <std::size_t NumInputs, std::size_t NumOutputs>
class gate_with_leads : public gate
{
protected:
  std::array<input_lead,  NumInputs>  m_inputs;
  std::array<output_lead, NumOutputs> m_outputs;

public:
  // Rule of 0. Implicitly non-copyable

  /// Return the number of input leads
  static constexpr std::size_t num_inputs() { return NumInputs; };

  /// Return the number of output leads
  static constexpr std::size_t num_outputs() { return NumOutputs; };

  const output_lead* get_output_lead(unsigned idx) const override;

  void connect_input(unsigned    input_idx,
                     const gate* src_gate,
                     unsigned    src_output_idx = 0) override;
};

///////////////////////////////////////////////////////////////////////////////
//           Inline and template implementations below this line             //
///////////////////////////////////////////////////////////////////////////////

template <std::size_t NI, std::size_t NO>
const output_lead* gate_with_leads<NI,NO>::get_output_lead(unsigned idx) const
{
  assert(idx < NO);  // Precondition check
  return &m_outputs[idx];
}

template <std::size_t NI, std::size_t NO>
void gate_with_leads<NI,NO>::connect_input(unsigned    input_idx,
                                           const gate* src_gate,
                                           unsigned    src_output_idx)
{
  assert(input_idx < NI);       // Precondition check
  assert(src_gate != nullptr);  // Precondition check

  m_inputs[input_idx].connect_to(src_gate->get_output_lead(src_output_idx));
}

} // close namespace sim

#endif // ! defined(INCLUDED_SIM_GATE_BASE)

// Local Variables:
// c-basic-offset: 2
// End:
