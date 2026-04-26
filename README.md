# Logic Gate Simulator

Atropos take-home exercise by Pablo Halpern

# Summary

This software simulates a network of interconnected logic gates (OR, NAND, XOR,
etc.), e.g. for an ASIC or FPGA.  The simulation maintains a global clock
operating at the rate of one cycle per _tick_ (an arbitrary unit of time). A
count is maintained of the number of ticks since the start of the simulation,
incremented at the end of each clock cycle. Using an abstract time unit
simplifies using the engine to simulate different semiconductor technologies
having different switching speeds.

The program takes two input files:

1. A configuration file that lists the gates and connections to other gate.

2. A file describing a time-sequence of values for each Boolean input line.

Since each input has a tick count at which it occurs, it is guaranteed that all
inputs will eventually be consumed (thought the gate array might ignore one or
more changes on the input lines).

The program terminates when all input has been consumed and one of the
following occur:

* The entire state of the system has not changed in 2 clock cycles (i.e., a
  steady state has been reached that will never change).

* The state of the _output_ lines has not changed in some configurable number
  of ticks (even if the internal system state is still changing).

The program writes the following output to the standard output stream:

1. A time-sequence of values for each Boolean output line.

2. The clock value reason for termination.

# Granularity and Constraints

The gates are not assumed to be perfect, but certain attributes are simplified
for the purpose of simulation:

* Although the gates operate asynchronously their operation is _quantized_ such
  that changes are observed only at the end of a clock cycle.

* Each gate has a propagation delay (switching time) of at least one tick. All
  propagation delays are (positive) integral multiples of one tick.

* Indeterminate (non-Boolean) values are not represented. At the end of each
  clock cycle, the outputs of each gate are stable, representing exactly 0
  or 1.

* The interconnects between the gates are lossless and have sub-tick
  propagation. However, a value written within one clock cycle is not available
  until start of the next cycle. Multiple changes within a single clock cycle
  are not allowed and are considered a programming error within the simulation.

* A consequence of the previous constraint is that each gate's inputs are
  stable within a single clock cycle; changes to those inputs are invisible
  until the start of the next clock cycle.

* Each gate has a maximum fan-out (number of wires) for each output. Attempting
  to connect more than that maximum to a single output is a configuration error.
