/* simple_gates.cpp                                                   -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <simple_gates.h>

namespace simple_gates {

void NOT::execute()
{
  bool in = this->m_inputs[0].get();
  this->m_outputs[0].set(!in);
}

void AND::execute()
{
  bool in0 = this->m_inputs[0].get();
  bool in1 = this->m_inputs[1].get();
  this->m_outputs[0].set(in0 && in1);
}

void OR::execute()
{
  bool in0 = this->m_inputs[0].get();
  bool in1 = this->m_inputs[1].get();
  this->m_outputs[0].set(in0 || in1);
}

void NAND::execute()
{
  bool in0 = this->m_inputs[0].get();
  bool in1 = this->m_inputs[1].get();
  this->m_outputs[0].set(!(in0 && in1));
}

void NOR::execute()
{
  bool in0 = this->m_inputs[0].get();
  bool in1 = this->m_inputs[1].get();
  this->m_outputs[0].set(!(in0 || in1));
}

void XOR::execute()
{
  bool in0 = this->m_inputs[0].get();
  bool in1 = this->m_inputs[1].get();
  this->m_outputs[0].set(in0 != in1);
}

} // close namespace simple_gates
