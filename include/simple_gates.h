/* simple_gates.h                                                     -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * @brief Simple gate declarations used by the logic gate simulation.
 *
 * This component defines the familiar logic gates NOT, AND, NAND, OR,
 * etc.. These are not the only kinds of gates possible within the simulator
 * infrastructure and the simulator does not need any of these gates to
 * operate. For this reason, these gates are not in the `sim` namespace, but
 * are in a namespace of their own, `simple_gates`.
 */

#ifndef INCLUDED_SIMPLE_GATES
#define INCLUDED_SIMPLE_GATES

#include <sim_gate_base.h>

namespace simple_gates {

/// One-input `NOT` gate
struct NOT : sim::gate_with_IO<1, 1>
{
  void execute() override;
};

/// Two-input `AND` gate
struct AND : sim::gate_with_IO<2, 1>
{
  void execute() override;
};

/// Two-input `OR` gate
struct OR : sim::gate_with_IO<2, 1>
{
  void execute() override;
};

/// Two-input `NAND` gate
struct NAND : sim::gate_with_IO<2, 1>
{
  void execute() override;
};

/// Two-input `NOR` gate
struct NOR : sim::gate_with_IO<2, 1>
{
  void execute() override;
};

/// Two-input `XOR` gate
struct XOR : sim::gate_with_IO<2, 1>
{
  void execute() override;
};

} // close namespace simple_gates

#endif // ! defined(INCLUDED_SIMPLE_GATES)

// Local Variables:
// c-basic-offset: 2
// End:
