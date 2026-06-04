# Future Integration: compiled-policy-c

## Current State
Zero-dependency C library for deploying compiled tile policies on microcontrollers. Translates high-level policy specifications into efficient C code that runs on ESP32 and other constrained devices.

## Integration Opportunities

### With ternary-locks (Rust)
Lock algebra defines access control policies. `compiled-policy-c` compiles those policies for edge deployment. A `Lock` with `Pattern` becomes a bitmask check in C. `LockComposition::And` becomes a chain of checks. The pipeline: define policies in Rust, compile to C, deploy on ESP32.

### With ternary-compiler-v2
The compiler generates optimized IR; `compiled-policy-c` is one of its targets. Compile a policy through the v2 pipeline: policy spec → ternary IR → optimization → C emission. The v2 optimizer removes dead policy branches and compresses the policy graph.

### With All C Ports (Policy Wrapper)
Every C port (avoidance-cascade, conservation-matrix, negative-space, fitness, evolution, inference, etc.) can be wrapped in compiled policies. A policy says "run cascade detection when population > threshold" or "enforce conservation laws on every tick." The policy IS the orchestration layer for edge devices.

## Potential in Mature Systems
In room-as-codespace, compiled policies are the room operating system for edge devices. An ESP32 runs a compiled policy that orchestrates all the C libraries: conservation enforcement, cascade detection, fitness evaluation, inference. The policy is the room's "firmware" — compiled once, runs forever.

## Cross-Pollination Ideas
- Policies as room configuration — change room behavior by changing the compiled policy
- Policy hot-reloading via OTA firmware updates
- Policy composition: combine small policies (cascade + conservation + fitness) into room-level policy

## Dependencies for Next Steps
- Integration with ternary-locks for policy specification
- Integration with ternary-compiler-v2 for optimization pipeline
- ESP32 OTA update mechanism for policy deployment
