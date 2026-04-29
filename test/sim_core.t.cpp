/* sim_core.t.cpp                                                     -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <sim_core.h>
#include <gtest/gtest.h>

/// Test the `sim::time` class
TEST(SimCoreTest, Time) {
  // Clock starts at zero and stays until we advance it.
  EXPECT_EQ(0,  sim::time::clock());
  EXPECT_EQ(0,  sim::time::clock());

  // After each advance (and only when advanced), clock is incremented
  sim::time::advance_clock();
  EXPECT_EQ(1,  sim::time::clock());
  EXPECT_EQ(1,  sim::time::clock());
  sim::time::advance_clock();
  EXPECT_EQ(2,  sim::time::clock());
  EXPECT_EQ(2,  sim::time::clock());

  // Resetting clock sets it to zero again
  sim::time::reset_clock();
  EXPECT_EQ(0,  sim::time::clock());
  EXPECT_EQ(0,  sim::time::clock());
}

/// Single-thread test of the `sim::input_lead` and `sim::output_lead` classes.
TEST(SimCoreTest, InputOutputLead_ST) {
  {
    sim::output_lead ol;
    sim::input_lead  il; const sim::input_lead& ilC = il;
    il.connect_to(&ol);

    EXPECT_FALSE(ilC.get());

    sim::time::advance_clock();
    EXPECT_FALSE(ilC.get());  // Was not changed

    ol.set(true);
    EXPECT_FALSE(ilC.get());  // Does not see change

    sim::time::advance_clock();
    EXPECT_TRUE(ilC.get());  // Change is now visible

    ol.set(true); // No change
    EXPECT_TRUE(ilC.get());

    ol.set(false);
    EXPECT_TRUE(ilC.get());  // Does not see change

    ol.set(false);           // idempotent
    EXPECT_TRUE(ilC.get());  // Does not see change

    sim::time::advance_clock();
    EXPECT_FALSE(ilC.get());  // Change is now visible
  }

  {
    sim::output_lead ol(true);  // With initial value
    sim::input_lead  il; const sim::input_lead& ilC = il;
    il.connect_to(&ol);

    EXPECT_TRUE(ilC.get());  // See initial value

    sim::time::advance_clock();
    EXPECT_TRUE(ilC.get());  // No change

    ol.set(false);
    EXPECT_TRUE(ilC.get());  // Does not see change

    sim::time::advance_clock();
    EXPECT_FALSE(ilC.get());  // Change is now visible
  }
}

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

  void connect_input(unsigned, const gate* , unsigned) override
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
                     unsigned    src_output_idx) override
  {
    assert(0 == input_idx);  // Precondition check
    m_in.connect_to(src_gate->get_output_lead(src_output_idx));
    m_value = m_in.get();
  }

  void execute() override { m_value = m_in.get(); }
};

} // close namespace test_gate_prot_ns

/// Test the `gate` protocol
TEST(SimCoreTest, gateProtocol) {
  using namespace test_gate_prot_ns;

  bit_source_g source;
  bit_sink_g   sink;
  sink.connect_input(0, &source, 0);
  EXPECT_FALSE(sink.get_value());

  // First clock cycle: set `source` output to `false` (i.e. no change).
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Read old `source` value (also `false`)

  sim::time::advance_clock();

  // Second clock cycle: set `source` output to `true`
  EXPECT_FALSE(sink.get_value());  // Read new `source` value (no change)
  source.set_value(true);
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change not visible until next cycle

  sim::time::advance_clock();

  // Third clock cycle: set `source` output back to `false`
  source.set_value(false);
  source.execute();
  sink.execute();
  EXPECT_TRUE(sink.get_value());  // Change from last clock cycle now visible

  sim::time::advance_clock();

  // Fourth clock cycle: Last change propagates through
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change from last clock cycle now visible
}

namespace test_gateWithIO_prot_ns {

/// Test class that takes no input and simply sets a user-supplied Boolean
/// value to a single output lead. Used to test the `gate_with_IO` protocol.
class bit_source_g : public sim::gate_with_IO<0, 1>
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
/// `gate_with_IO` protocol.
class bit_sink_g : public sim::gate_with_IO<1, 0>
{
  bool            m_value = false;

public:
  bit_sink_g() = default;

  /// Return the value read from the input lead.
  bool get_value() const { return m_value; }

  void execute() override { m_value = m_inputs[0].get(); }
};

} // close namespace test_gateWithIO_prot_ns

/// Test the `gate_with_IO` protocol
TEST(SimCoreTest, gateWithIOProtocol) {
  using namespace test_gateWithIO_prot_ns;

  bit_source_g source;
  bit_sink_g   sink;
  sink.connect_input(0, &source, 0);
  EXPECT_FALSE(sink.get_value());

  // First clock cycle: set `source` output to `false` (i.e. no change).
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Read old `source` value (also `false`)

  sim::time::advance_clock();

  // Second clock cycle: set `source` output to `true`
  EXPECT_FALSE(sink.get_value());  // Read new `source` value (no change)
  source.set_value(true);
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change not visible until next cycle

  sim::time::advance_clock();

  // Third clock cycle: set `source` output back to `false`
  source.set_value(false);
  source.execute();
  sink.execute();
  EXPECT_TRUE(sink.get_value());  // Change from last clock cycle now visible

  sim::time::advance_clock();

  // Fourth clock cycle: Last change propagates through
  source.execute();
  sink.execute();
  EXPECT_FALSE(sink.get_value());  // Change from last clock cycle now visible
}

namespace test_simpleGate_ns {

/// Simple gate that implements A => B (A implies B), e.g., (!A || B)
class implies_g : public sim::gate_with_IO<2, 1>
{
  void execute() override
  {
    m_outputs[0].set(!m_inputs[0].get() || m_inputs[1].get());
  }
};

} // close namespace test_simpleGate_ns

TEST(SimCoreTest, simpleGate) {
  using namespace test_gateWithIO_prot_ns;
  using namespace test_simpleGate_ns;

  bit_source_g  A, B;
  bit_sink_g    result;
  implies_g     ig;
  std::array<sim::gate*, 4> gate_list{ &ig, &A, &B, &result };

  result.connect_input(0, &ig, 0);
  ig.connect_input(0, &A, 0);
  ig.connect_input(1, &B, 0);

  for (unsigned i = 0; i < 4; ++i) {
    bool a = i & 1;  // Low bit
    bool b = i & 2;  // High bit
    bool exp = !a || b;

    A.set_value(a);
    B.set_value(b);

    for (auto& g : gate_list)
      g->execute();

    sim::time::advance_clock();

    for (auto& g : gate_list)
      g->execute();

    sim::time::advance_clock();

    for (auto& g : gate_list)
      g->execute();

    sim::time::advance_clock();

    EXPECT_EQ(result.get_value(), exp);
  }
}

// Local Variables:
// c-basic-offset: 2
// End:
