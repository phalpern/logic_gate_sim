# Logic Gate Simulator

Atropos take-home exercise -- Pablo Halpern

# Summary

This software simulates a network of interconnected logic gates (OR, NAND, XOR,
etc.), e.g. for an ASIC or FPGA.  The simulation maintains a global clock
operating at the rate of one cycle per _tick_ (an arbitrary unit of time). A
count is maintained of the number of ticks since the start of the simulation,
incremented at the end of each clock cycle. Using an abstract time unit
simplifies the simulation engine and makes it adaptable to simulate different
semiconductor technologies having different switching speeds.

The program takes two types of input data:

1. A configuration of gates and the connections between them.

2. A time sequence of Boolean values for all input lines.

The program terminates when all input has been consumed and a specified number
of clock ticks have passed since timestamp of the last input.

Since each set of input bits has a timestamp (in ticks) when it occurs, all
inputs are guaranteed to be consumed (though not every change in the input will
necessarily result in a change to the output).

The main entry point returns a vector containing a time sequence of values for
each Boolean output line.

# Source Code and Build instructions

Building this project requires a C++23 compiler (I used Clang++ 18.1.3, but
earlier versions of Clang or GCC should also work), cmake, Ninja, and git. My
development platform was Ubuntu Linux 24.04 running in Windows under WSL.

Clone the repo:

```
$ git clone https://github.com/phalpern/logic_gate_sim.git
```

Configure using `cmake`:

```
$ cd logic_gate_sim
$ cmake -DCMAKE_BUILD_TYPE:STRING=Debug -S $PWD -B $PWD/build -G Ninja
```

Build and run the sample simulation (see below for the meaning of the output):

```
$ cmake --build build --config Debug --target run
```

Optionally build and run the unit tests:

```
$ cmake --build build --config Debug --target all --
$ ctest --test-dir build --output-on-failure
```

# Granularity and Constraints

The gates are not assumed to be perfect (e.g., they exhibit propagation delay),
but certain attributes are simplified for the purpose of simulation:

* Although the gates operate asynchronously, their operation is _quantized_ such
  that changes are observed only at the end of a clock cycle.

* Each gate has a propagation delay (switching time) of at least one tick. All
  propagation delays are (positive) integral multiples of one tick.

* Indeterminate (non-Boolean) voltages are not represented. At the end of each
  clock cycle, the outputs of each gate are stable, representing exactly 0
  or 1.

* The interconnects between the gates are lossless and have sub-tick
  propagation. However, a value written within one clock cycle is not available
  to the next connected gate until the start of the next cycle. Multiple changes
  within a single clock cycle are not allowed and would be considered a
  programming error within the simulation.

* A consequence of the previous constraint is that each gate's inputs are
  stable within a single clock cycle; changes to those inputs are invisible
  until the start of the next clock cycle.

* The simulation is deterministic: for a given circuit configuration and input
  time sequence, the output time sequence is uniquely defined and identical
  across runs. This holds despite potential parallel execution of gate
  evaluation, since all state changes are applied in discrete clock ticks with
  well-defined visibility rules.

# Sample Simulation

If this were a real product, the inputs (i.e., the circuit description and
input values) would be specified in external files (text, json, or something
like that). Given the limited time available, they are instead encoded as C++
objects using a primitive DSL.

The `2bit_adder.cpp` file contains a simulation of an adder circuit built from
XOR, AND, and OR gates. It adds two 2-bit unsigned integers and produces a
2-bit result plus carry (or a 3-bit result, depending on how you want to look
at it).  There are 4 input bits and 3 output bits. The inputs change every 4
cycles and go through all 16 combinations of 2-bit addends.

The program prints all of its inputs and outputs, interleaved according to
their timestamps (in ticks since the start of the program). The longest path
through the circuit is 3 gates deep, so one can watch the values propagate
through over the course of 1-3 cycles before stabilizing, depending on the
inputs. For example, in this excerpt, we can see that, when adding 1 and 0, the
result is computed in one cycle, but adding 2 (binary 10) and 0 takes two
cycles:

```
  4: in[01 + 00]
  5:             out[001]
  8: in[10 + 00]
  9:             out[000]
 10:             out[010]
```

A couple of simpler simulations can be found in the `sim_mainloop.t.cpp` test
driver, including an example of a circuit containing a cycle.


# Software Design

## Basic Structure

At a high level, the simulator models a directed graph of gates connected via
implicit interconnects (input leads pointing to output leads). At each clock
tick, all gates are evaluated (via `execute`), the clock advances, and any
resulting output changes are recorded.

The main objects in the system are gates and interconnects. Interconnects are
not represented by distinct objects; instead, each gate's input leads contain
pointers to other gates' output leads, forming a many-to-one mapping. Each gate
type is implemented as a class derived from `gate`, with gate-specific logic in
the `execute` virtual function. The special gate types `input_ports` and
`output_ports` are used directly by the simulation machinery; otherwise, all
gate types are treated uniformly. It is straightforward to extend the system
with new gate types.

Once a circuit is built up, the simulation runs in a main loop that can be
summarized as

 1. In a subloop, call the `execute` method on each gate.
 2. Advance the clock by one tick.
 3. If any output port changed, append the timestamped outputs to the results
    vector.

## Design for Parallelism

Theoretically, a large simulation could be accelerated using multiple CPUs. The
system is designed so that `execute` can be called on any number of gates in
parallel. An interconnect can have one writer concurrently with any number of
readers, without using synchronization (including memory barriers). The
simulated clock is always advanced serially within the main loop, never in
parallel with a call to `execute`.

I experimented with running `execute` in a parallel `for_each`. The program
seemed to run correctly, but the simulation is much too small to see
speedup. Moreover, I was unable to verify correctness using Thread Sanitizer
because of a known interaction with the parallelism runtime that results in
many false positives for data races.

## Design Trade-off 1: Implementation of Interconnects

Despite the lock-free implementation of the interconnects, there are
disadvantages to implementing them as implicit connections among objects rather
than as separate `interconnect` objects.  The current implementation of
`output_lead` and `input_lead` relies on a clever algorithm utilizing the
global clock. This use of the clock prevents multiple simulations from running
concurrently, as each instance would need a separate clock. If the
interconnects were represented as separate objects, they would not need to rely
on clever algorithms or a global clock. Like gates, interconnect objects could
be updated in parallel, changing the main loop to something like:

 1. In a subloop, call the `execute` method on each gate.
 2. In a subloop, process each interconnect so that the just-set value becomes
    visible to readers.
 3. Advance the clock by one tick.
 4. If any output port changed, append the timestamped outputs to the results
    vector.

The subloops in steps 1 and 2 can each be parallelized, but step 1 cannot be
run in parallel with step 2.

Finally, having a separate data structure to represent the interconnect would
make it easier to observe and debug the operation of the simulator, especially
if graphic visualizations were added.

## Design Trade-off 2: Storing Pointers to the Gates in a Single Vector

The current implementation maintains a heterogeneous collection of gate objects
as a vector of base-class pointers.  Calling the `execute` virtual function in
the hot subloop (step 1 in the main loop) can be inefficient because (1) it
requires pointer indirection and (2) it requires virtual-function dispatch,
which can defeat the CPU's branch predictor and prefetch machinery.

A better alternative might be to sort the gates into multiple collections,
with each collection being homogeneous. The `execute` method would belong to
the collection, rather than to the individual gate, so the virtual-function
overhead would be paid only once per collection. Moreover, within each
collection, the gates could be stored directly, rather than as pointers,
eliminating a level of pointer indirection and providing a more cache-efficient
packing in memory.

## Design Trade-off 3: Representation of Outputs

The simulation runs to completion, then returns a vector of timestamped output
values. This approach yields a simple interface, but can consume a lot of
memory for large simulations. Moreover, it does not lend itself well to
real-time visualizations of the simulation's progress.

A better approach might be to have the main invoke a passed-in functor whenever
it detected a change to the output.

# Next Steps

Aside from exploring the alternative designs described above, I would consider
adding a robust external file representation of the circuit graph and input
values, rather than having them encoded directly into the C++ program. The
circuit description language might include macros for describing repeating
subcircuits.

To explore the limits of the simulation engine, I would add more sophisticated
gate types, like flip-flops, that have propagation delays of more than one
tick.

Another step might be to add some visualizations, including a graphic
representation of the circuit schematic, with color-coded data flows.

Higher-level simulations should be possible using the same basic architecture
by adding interconnects wider than one bit (e.g., a 32-bit bus) high-level
components like ALUs, and buffered queues between logic units.

# Notes on the Process

## Use of LLMs

I made minimal use of LLMs (mostly ChatGPT and Microsoft Copilot) in the
programming process.  Specifically:

1. Creating the initial `CMakeLists.txt` file
2. Creating `launch.json` for VS Code
3. Given the class definitions in `simple_gates.h` and the implementation of
   the `NOT` gate, I let the AI generate the `execute` function for all of the
   other gates in that component.
4. Similarly, given the implementation of test cases for `NOT` and `AND`, I let
   the AI generate the remaining test cases.
5. I used the AI to rename identifiers across the project during refactoring
   passes.
6. I used ChatGPT to proof read this `README` file and suggest improvements.

Items 1 and 2 were big time savers. Items 3, 4, and 5 might've saved a little
time, but they were mostly experimentation with what the AI was capable
of. The changes resulting from item 6 were modest, but useful.

## Time Spent

I spent about 25 hours, total, on specification and coding. I admit to
significantly exceeding the 16-hour time allotment. A major portion of that
extra time was spent on test drivers and refactoring interfaces.

When I started my project, I had a new computer that was not completely set up
for coding. I was careful not to include setup time in my accounting, but some
setup headaches did add significantly to the calendar time.
