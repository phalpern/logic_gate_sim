/* sim_gate_base.t.cpp                                                -*-C++-*-
 *
 * Copyright (C) 2026 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_gate_base.h>
#include <gtest/gtest.h>

namespace test_gate_prot_ns {

/// Test class that takes no input and simply sets a user-supplied Boolean
/// value to a single output lead. Used to test the `gate` protocol.
class bit_source_g : public sim::gate
{
  bool             m_value;
  sim::output_lead m_out;

public:
  explicit bit_source_g(bool initval = false)
    : m_value(initval), m_out(initval) { }

  /// Set value to set to output lead on `execute`
  void set_value(bool v) { m_value = v; }

  const sim::output_lead* get_output_lead(unsigned idx) const override
  {
    assert(0 == idx);
    return &m_out;
  }

  void connect_input(unsigned, const gate* , unsigned = 0) override
  {
    // No inputs to connect
    assert(false);
  }

  void execute() override { m_out.set(m_value); }
};

/// Test class that has no output leads but simply makes the value from its
/// input lead available via a `get_value` method. Used to test the `gate`
/// protocol.
class bit_sink_g : public sim::gate
{
  bool            m_value = false;
  sim::input_lead m_in;

public:
  bit_sink_g() = default;

  /// Return the value read from the input lead.
  bool get_value() const { return m_value; }

  const sim::output_lead* get_output_lead(unsigned) const override
  {
    assert(false);  // Precondition check
    return nullptr;
  }

  void connect_input(unsigned    input_idx,
                     const gate* src_gate,
                     unsigned    src_output_idx = 0) override
  {
    assert(0 == input_idx);  // Precondition check
    m_in.connect_to(src_gate->get_output_lead(src_output_idx));
    m_value = m_in.get();
  }

  void execute() override { m_value = m_in.get(); }
};

} // close namespace test_gate_prot_ns

/// Test the `gate` protocol
TEST(SimGateBaseTest, gateProtocol) {
  using namespace test_gate_prot_ns;

  bit_source_g source;
  bit_sink_g   sink;
  sink.connect_input(0, &source);
  EXPECT_FALSE(sink.get_value());

  // First clock cycle: set `source` output to `false` (i.e. no change).
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Read old `source` value (also `false`)

  sim::clock::advance();

  // Second clock cycle: set `source` output to `true`
  EXPECT_FALSE(sink.get_value());  // Read new `source` value (no change)
  source.set_value(true);
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change not visible until next cycle

  sim::clock::advance();

  // Third clock cycle: set `source` output back to `false`
  source.set_value(false);
  source.execute();
  sink.execute();
  EXPECT_TRUE(sink.get_value());  // Change from last clock cycle now visible

  sim::clock::advance();

  // Fourth clock cycle: Last change propagates through
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change from last clock cycle now visible
}

namespace test_gateWithIO_prot_ns {

/// Test class that takes no input and simply sets a user-supplied Boolean
/// value to a single output lead. Used to test the `gate_with_leads` protocol.
class bit_source_g : public sim::gate_with_leads<0, 1>
{
  bool             m_value;

public:
  explicit bit_source_g(bool initval = false)
    : m_value(initval) { this->m_outputs[0].set(initval); }

  /// Set value to set to output lead on `execute`
  void set_value(bool v) { m_value = v; }

  void execute() override { m_outputs[0].set(m_value); }
};

/// Test class that has no output leads but simply makes the value from its
/// input lead available via a `get_value` method. Used to test the
/// `gate_with_leads` protocol.
class bit_sink_g : public sim::gate_with_leads<1, 0>
{
  bool            m_value = false;

public:
  bit_sink_g() = default;

  /// Return the value read from the input lead.
  bool get_value() const { return m_value; }

  void execute() override { m_value = m_inputs[0].get(); }
};

} // close namespace test_gateWithIO_prot_ns

/// Test the `gate_with_leads` protocol
TEST(SimGateBaseTest, gateWithIOProtocol) {
  using namespace test_gateWithIO_prot_ns;

  bit_source_g source;
  bit_sink_g   sink;
  sink.connect_input(0, &source);
  EXPECT_FALSE(sink.get_value());

  // First clock cycle: set `source` output to `false` (i.e. no change).
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Read old `source` value (also `false`)

  sim::clock::advance();

  // Second clock cycle: set `source` output to `true`
  EXPECT_FALSE(sink.get_value());  // Read new `source` value (no change)
  source.set_value(true);
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change not visible until next cycle

  sim::clock::advance();

  // Third clock cycle: set `source` output back to `false`
  source.set_value(false);
  source.execute();
  sink.execute();
  EXPECT_TRUE(sink.get_value());  // Change from last clock cycle now visible

  sim::clock::advance();

  // Fourth clock cycle: Last change propagates through
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change from last clock cycle now visible
}

namespace test_simpleGate_ns {

/// Simple gate that implements A => B (A implies B), e.g., (!A || B)
class implies_g : public sim::gate_with_leads<2, 1>
{
  void execute() override
  {
    m_outputs[0].set(!m_inputs[0].get() || m_inputs[1].get());
  }
};

} // close namespace test_simpleGate_ns

TEST(SimGateBaseTest, simpleGate) {
  using namespace test_gateWithIO_prot_ns;
  using namespace test_simpleGate_ns;

  bit_source_g  A, B;
  bit_sink_g    result;
  implies_g     ig;
  std::array<sim::gate*, 4> gate_list{ &ig, &A, &B, &result };

  result.connect_input(0, &ig);
  ig.connect_input(0, &A);
  ig.connect_input(1, &B);

  for (unsigned i = 0; i < 4; ++i) {
    bool a = i & 1;  // Low bit
    bool b = i & 2;  // High bit
    bool exp = !a || b;

    A.set_value(a);
    B.set_value(b);

    for (auto& g : gate_list)
      g->execute();

    sim::clock::advance();

    for (auto& g : gate_list)
      g->execute();

    sim::clock::advance();

    for (auto& g : gate_list)
      g->execute();

    sim::clock::advance();

    EXPECT_EQ(result.get_value(), exp);
  }
}

// Local Variables:
// c-basic-offset: 2
// End:
