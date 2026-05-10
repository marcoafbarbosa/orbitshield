# OrbitShield Network Routing Implementation Plan

## Overview

This workplan describes the test-driven effort to enable routed IP traffic across the OrbitShield constellation and ground stations using the existing module.

It is intended to be updated as implementation progresses. Each milestone contains phases with an explicit status marker.

## Build and Test Commands

From the ns-3 root directory (`/home/marco/ns-3-dev`):

```bash
./ns3 configure --enable-tests
./ns3 build
./ns3 run test-runner -- --suite=orbitshield
```

Builds and test runs may take a long time.

Execution monitoring policy for long-running terminal tasks:

- Prefer waiting for command completion instead of frequent polling.
- Poll at most once per minute only to confirm progress when needed.
- Do not interrupt active tasks unless they appear stalled (no progress) for 3 or more minutes.
- If a task appears stalled for 3 or more minutes, capture output once more and then decide whether to interrupt and retry.

For focused execution of an individual new test case later:

```bash
./ns3 run test-runner -- --suite=orbitshield --testcase=<TestCaseName>
```

## Test Data Source

All tests shall use the existing Iridium dataset at:

- `contrib/orbitshield/data/iridium-20260312.yaml`

Test scenarios should be driven from that file so behavior remains realistic and reproducible.

## Guiding Design Principles

Follow the current OrbitShield design and ns-3 coding style:

- Keep the module structure aligned with `contrib/orbitshield/model`, `test`, `examples`, and `tools`.
- Use `Ptr<>`, `CreateObject<>`, `DynamicCast<>`, `MakeCallback`, and `TypeId()` idioms.
- Continue existing logging patterns: `NS_LOG_COMPONENT_DEFINE`, `NS_LOG_INFO`, `NS_ASSERT_MSG`, and `NS_OBJECT_ENSURE_REGISTERED`.
- Keep public APIs small in `model/*.h` and implementation details in `model/*.cc`.
- Maintain the current build conventions: use `build_lib`, `build_exec`, and link against the OrbitShield library from `contrib/orbitshield/CMakeLists.txt`.
- Write tests as ns-3 `TestCase` subclasses under `contrib/orbitshield/test/`.

## Status Summary

- Milestone 1: API scaffolding and test setup — **Not implemented**
- Milestone 2: Multi-link node support — **Not implemented**
- Milestone 3: IPv4 stack and simple ping path — **Not implemented**
- Milestone 4: Topology refresh integration — **Not implemented**
- Milestone 5: Complex routing behavior — **Not implemented**
- Milestone 6: Cleanup and documentation — **Not implemented**

## Milestone 1: API Scaffolding and Test Setup

### Phase 1.1: Establish test harness and data-driven scenario

- Status: **Not implemented**
- Goal: Add OrbitShield test cases that use the Iridium YAML dataset.
- Work:
  - Add test scaffolding to `contrib/orbitshield/test/`.
  - Create an initial `OrbitShieldIridiumTopologyTest` that loads `contrib/orbitshield/data/iridium-20260312.yaml` via `Constellation::LoadFromRingFile()`.
  - Verify the test can parse the ring file and discover the satellite and ground station objects.

### Phase 1.2: Define minimal APIs and placeholder mocks

- Status: **Not implemented**
- Goal: Define the APIs needed by later routing tests without yet implementing behavior.
- Work:
  - Create `contrib/orbitshield/model/orbitshield-routing-helper.h` and `.cc` with the following stub public API:
    - `class OrbitShieldRoutingHelper` (a plain C++ helper class, not an ns-3 Object)
    - `void Install(Ptr<Constellation> constellation)` — stub: no-op, returns immediately
    - `void RecomputeRoutes(Ptr<Constellation> constellation)` — stub: no-op, returns immediately
  - Extend `Constellation` with the following stub:
    - `void SetRouteUpdateCallback(Callback<void, Ptr<Constellation>> cb)` — stores the callback but never invokes it in this milestone
    - `Callback<void, Ptr<Constellation>> m_routeUpdateCallback` — private member to hold it
  - Register `orbitshield-routing-helper.cc` in `contrib/orbitshield/CMakeLists.txt` as a module source file.
  - Add tests that construct `OrbitShieldRoutingHelper` and call `Install()` and `RecomputeRoutes()` without crashing.
  - Update `contrib/orbitshield/README.md` with Milestone 1 scaffolding details and temporary limitations.
- Outcome: all stubs compile and link; tests pass trivially; real behavior is implemented in Milestones 3–4.

## Milestone 2: Multi-Link Node Support

### Phase 2.1: Refactor `SatelliteNetDevice` for multiple links per node

- Status: **Not implemented**
- Goal: Allow satellites and ground stations to maintain multiple concurrent `NetworkDevice`/`Channel` pairs.
- Work:
  - Change the link association model so `SatelliteNetDevice` is not limited to one `SatelliteLink`.
  - Ensure each active link can still use its own `IslPropagationDelayModel` and perform packet delivery independent of other links.
  - Preserve existing API semantics for a single-link path while enabling multi-link growth.
  - Evaluate and correct the `NetDevice` trait methods on `SatelliteNetDevice`. Currently: `NeedsArp()→true`, `IsBroadcast()→true`, `IsPointToPoint()→false`. ISL and GSL links are point-to-point unicast channels; with `NeedsArp()==true`, IPv4 will attempt ARP resolution which will silently fail on these channels. The correct values for P2P satellite links are `NeedsArp()→false`, `IsBroadcast()→false`, `IsPointToPoint()→true`. Correct these during this refactor and update `GetBroadcast()` to assert/unreachable accordingly.
- Note: this is the core enabler for any routing architecture.

### Phase 2.2: Ground station multi-link support

- Status: **Not implemented**
- Goal: Ensure `GroundStation` nodes can host multiple outgoing and incoming satellite links.
- Work:
  - Mirror the satellite multi-link support on the ground station side.
  - Add test coverage that each ground station object can have multiple attached `SatelliteNetDevice` devices.
  - Confirm the ground station mobility model is still attached correctly for range checks.
  - Update `contrib/orbitshield/README.md` with multi-link model changes and compatibility notes.

## Milestone 3: IPv4 Stack and Simple Ping Path

### Phase 3.1: Install the standard ns-3 Internet stack

- Status: **Not implemented**
- Goal: Use standard IPv4 interfaces on satellites and ground stations.
- Work:
  - Add `${libinternet}` and `${libapplications}` to `LIBRARIES_TO_LINK` in `contrib/orbitshield/CMakeLists.txt`. This is a hard prerequisite — the build will fail at link time without it.
  - Implement `OrbitShieldRoutingHelper::Install(Ptr<Constellation>)` to call `InternetStackHelper::Install()` on all satellite and ground station nodes.
  - Enable IPv4 forwarding on all satellite nodes by setting the `Ipv4::IpForward` attribute to `true` after stack installation.
  - Assign IPv4 addresses to each point-to-point link interface using `Ipv4AddressHelper` with base address `10.0.0.0` and mask `255.255.255.252` (`/30` subnets, one per link pair). Address blocks are allocated sequentially across all ISL and then GSL interfaces in creation order.
  - Keep helper code consistent with OrbitShield module structure.

### Phase 3.2: Build end-to-end ping path over the Iridium topology

- Status: **Not implemented**
- Goal: Verify GS->satellite->satellite->GS traffic using the Iridium dataset.
- Work:
  - Assign IPv4 addresses to each link interface created by ISLs and GSLs using the `/30` scheme defined in Phase 3.1.
  - Use `Ipv4StaticRouting` to populate routes for the simple path.
  - Add a ping/echo test that sends ICMP from ground station **Tempe** (33.41°N, -111.94°W) to ground station **Fairbanks** (64.84°N, -147.72°W). Both are defined in `iridium-20260312.yaml`.
  - Run the simulation for a fixed `Simulator::Stop(Seconds(60.0))` window starting at TLE epoch (no time offset).
  - Assert that at least one ICMP echo reply is received at Tempe within the 60-second window.
  - Update `contrib/orbitshield/README.md` with IPv4/routing setup and ping test usage.
- Success criteria:
  - At least one ICMP echo reply is received at Tempe.
  - The reply round-trip time is non-zero and ≤ 500 ms (consistent with satellite propagation delay).
  - The test demonstrates that a multi-hop path exists across the satellite mesh.

## Milestone 4: Topology Refresh Integration

### Phase 4.1: Connect route updates to `Constellation::RefreshIslTopology()`

- Status: **Not implemented**
- Goal: Make route updates react to the dynamic ISL/GSL refresh mechanism.
- Work:
  - Add a refresh callback or event hook in `Constellation` by invoking `m_routeUpdateCallback` (if set) at the end of `RefreshIslTopology()`.
  - Implement `OrbitShieldRoutingHelper::RecomputeRoutes(Ptr<Constellation>)` using shortest-hop Dijkstra over the full ISL+GSL graph. Edge weight is 1 per active link. Run from each ground station as a source node and populate `Ipv4StaticRouting` tables on all transit satellite and ground station nodes. Clear all existing routes before repopulating to avoid stale entries.
  - Preserve existing `CreateIslLinks()` / `CreateGroundLinks()` behavior while adding routing integration.
- Note: this phase is critical for correct handling of moving links.

### Phase 4.2: Validate dynamic behavior with new tests

- Status: **Not implemented**
- Goal: Confirm that routes are updated after topology changes.
- Work:
  - Add a dynamic route refresh test using the Iridium YAML dataset.
  - Run the simulation for `Simulator::Stop(Seconds(600.0))` with `SetIslRefreshInterval(Seconds(60.0))` (10 route recomputations).
  - Send one ICMP echo from Tempe to Fairbanks per refresh interval.
  - Assert that at least one ICMP echo reply is received within the first 120 seconds (initial connectivity).
  - Assert that the simulation completes without crash or assertion failure regardless of whether intermediate outages occur (graceful degradation).
  - Record and log the count of delivered vs. sent packets per interval. Do not assert a specific delivery ratio in this phase — that is deferred to Milestone 5.
  - Update `contrib/orbitshield/README.md` with topology-refresh and route-update behavior.

## Milestone 5: Complex Routing Behavior

### Phase 5.1: Advanced Iridium scenario with multiple ground stations

- Status: **Not implemented**
- Goal: Test a realistic multi-GS and multi-satellite routing scenario.
- Work:
  - Use the same Iridium dataset with all 5 defined ground stations: Tempe, Fairbanks, Svalbard, Izhevsk, Punta Arenas (10 GS pairs).
  - Run the simulation for `Simulator::Stop(Seconds(300.0))` with `SetIslRefreshInterval(Seconds(30.0))` (10 route recomputations).
  - Send one ICMP echo per GS pair per refresh interval and record delivery outcome, round-trip time, and hop count.
- Pass conditions (all must hold):
  - At least 7 of 10 GS pairs achieve a delivery ratio ≥ 80% over the 300-second window.
  - Every delivered packet has a measured round-trip time ≤ 500 ms.
  - The maximum hop count across all delivered packets is ≤ 8.
  - At least 3 distinct GS pairs achieve 100% delivery over the window.
  - The simulation completes without crash, assertion failure, or unhandled error.

### Phase 5.2: Select and validate routing strategy

- Status: **Not implemented**
- Goal: Decide whether to keep static route recomputation or adopt an ns-3 routing protocol.
- Work:
  - Compare static route recomputation against AODV or another ns-3 protocol.
  - If AODV is selected, install it on all nodes and validate the same test scenarios.
  - If static routing is kept, ensure the route update path is robust under fast refresh intervals.
  - Update `contrib/orbitshield/README.md` with advanced routing strategy details and scenario usage guidance.
- Recommendation: use static route recomputation for early phases, then optionally move to AODV in Phase 5.2 if required.

## Milestone 6: Cleanup, Style, and Documentation

### Phase 6.1: Code cleanup and consolidation

- Status: **Not implemented**
- Goal: Remove temporary mocks and clarify any API rough edges.
- Work:
  - Refactor helper functions into `model/` source files as needed.
  - Keep public API names simple and consistent with existing OrbitShield conventions.
  - Ensure `Constellation` remains the central manager for topology discovery.

### Phase 6.2: Documentation and README updates

- Status: **Not implemented**
- Goal: Perform a final documentation consistency pass after incremental milestone updates.
- Work:
  - Consolidate and normalize README content that was updated milestone-by-milestone.
  - Verify README reflects final APIs, test commands, topology behavior, and known limitations.
  - Keep `contrib/orbitshield/data/iridium-20260312.yaml` documented as the canonical test dataset.

## Test-Driven Implementation Strategy

- Add tests before implementing behavior.
- Use ns-3 `TestCase` subclasses under `contrib/orbitshield/test/`.
- Keep initial tests simple and data driven from the Iridium YAML file.
- Add API stubs when needed to satisfy compile-time dependencies.
- Confirm the tests fail first, then implement the minimal behavior to make them pass.
- Evolve tests as behavior becomes more complete.

## OrbitShield Coding Style Notes

Implementors should follow the existing OrbitShield style and structure:

- `model/` contains the C++ module core.
- `test/` contains unit tests and integration-style test cases.
- `examples/` contains runnable example executables.
- `tools/` contains utility programs and scripts.
- Source headers and sources are split cleanly, with one publicly visible interface in `.h` and behavior in `.cc`.
- Use `NS_LOG_COMPONENT_DEFINE("<Component>")` at the top of each `.cc`.
- Use `NS_OBJECT_ENSURE_REGISTERED(ClassName)` for ns-3 object registration.
- Use `Ptr<T>` and `CreateObject<T>()` rather than raw pointers.
- Use `std::optional`, `std::vector`, `std::map`, and `std::unordered_map` consistently where appropriate.
- Use `NS_ASSERT_MSG` and `NS_TEST_ASSERT_MSG_EQ` in tests.
- Follow the ns-3 and OrbitShield convention for Doxygen comments on all public APIs and new classes.
- Keep new code aligned with the existing `build_lib` / `build_exec` model in `contrib/orbitshield/CMakeLists.txt`.

## Agent and Sub-Agent Usage Policy

Long multi-milestone implementation runs are susceptible to context rot: accumulated history causes the agent to lose precision, drift from the original plan, and produce lower-quality code. Follow this policy to mitigate it.

### Core Principle

Treat this workplan document as the single source of truth. Agent memory is ephemeral. The document is not.

### Per-Milestone Context Reset

- Start each milestone in a fresh agent invocation.
- At the beginning of each invocation, re-read this workplan in full before taking any action.
- Do not rely on recalled context from a previous milestone's session.
- Use the phase status blocks and result summaries in this document to reconstruct current state.

### Sub-Agent Usage

- Use sub-agents for scoped read-only tasks: codebase exploration, file searches, API audits, test output analysis.
- Do not use sub-agents for writes. All file edits and commits are performed by the main agent.
- Keep each sub-agent invocation narrowly focused on a single question or file set.
- Summarize sub-agent findings into the main agent's working context immediately; discard the sub-agent after use.

### Context Hygiene Rules

- At the start of each phase within a milestone, re-read only the files relevant to that phase. Do not load the entire codebase into context speculatively.
- After completing a phase, record the outcome in the phase status block of this document before moving to the next phase. This creates a durable checkpoint.
- Avoid accumulating long chains of tool calls without a checkpoint write. After every 5 tool calls, assess whether a status update should be written to this document.
- If agent output quality appears to degrade (incorrect file references, hallucinated APIs, repeated retries), stop the current invocation, write a status block capturing the current state, and start a fresh agent invocation from the document.

### Resumability

- Any agent invocation must be able to resume from a cold start by reading this workplan alone.
- No implementation state should exist only in agent memory. If something is not written in this document or in a committed file, it does not exist for the purposes of subsequent invocations.

## Git Policy

- All OrbitShield development work is done on branch `network-routing-implementation`.
- Verify the branch before starting any implementation: `git -C /home/marco/ns-3-dev branch --show-current` must return `network-routing-implementation`.
- One commit is made per completed milestone, at the end of the milestone.
- Commit message format: `milestone X implementation` (e.g., `milestone 1 implementation`).
- Commits are made only when all milestone phases pass their phase completion gates.

### Git Safeguards

- Only files under `contrib/orbitshield/` may be staged and committed. Do not stage or modify any files outside this path.
- Do not amend, rebase, squash, reset, or otherwise alter existing git history.
- Do not force-push (`--force` or `-f`) under any circumstances.
- Do not create, switch to, or delete any branch other than using `network-routing-implementation` as the working branch.
- Do not push to any remote without explicit human instruction.
- Before committing, verify staged files with `git diff --name-only --cached` and confirm all are within `contrib/orbitshield/`.

### Commit Template

```bash
cd /home/marco/ns-3-dev
git add contrib/orbitshield/
git diff --name-only --cached   # verify only contrib/orbitshield/ files are staged
git commit -m "milestone X implementation"
```

## Autopilot Execution Contract

This section defines strict execution rules for an automated Copilot Autopilot run.

- Execute milestones in order. Do not skip phases.
- Complete one phase fully before starting the next phase.
- For each phase: implement -> build -> run required tests -> update phase status.
- Do not mark a phase as implemented unless all phase verification commands pass.
- Keep changes minimal and phase-scoped. Avoid unrelated refactors.
- Only modify files under `contrib/orbitshield/`. Do not touch any other part of the repository.
- If a phase fails after the retry policy is exhausted, stop and request human decision.
- Update `contrib/orbitshield/README.md` at the end of each milestone with that milestone's behavior/API/test-command delta.
- Update the "Which Milestone/Phase to Do Next" section of this document after completing each phase, pointing to the next phase whose status is `Not implemented`.
- Commit once at the end of each completed milestone using the prescribed git commit procedure.
- Follow the Agent and Sub-Agent Usage Policy (see above) for context management, fresh invocation rules, and checkpoint hygiene.

### Phase Completion Gate

A phase is complete only when all of the following are true:

- Build succeeds with exit code 0.
- Required phase tests pass with exit code 0.
- Phase status block is updated in this document.
- Any newly added tests are registered in the OrbitShield test suite and can be invoked via `test-runner`.
- Milestone-level README updates are completed and verified for accuracy.
- For the final phase of each milestone: one git commit is made with message `milestone X implementation`, staging only files under `contrib/orbitshield/`.

## API Contract Baseline for Autopilot

The following baseline contract must be respected while implementing phased changes:

- `Constellation` remains responsible for topology discovery, link creation, and refresh scheduling.
- Routing installation and route recomputation logic should be placed in helper APIs (or tightly scoped extensions) and not mixed into unrelated model responsibilities.
- New public APIs must define behavior for edge cases:
  - empty topology,
  - no active links,
  - missing ground stations,
  - unreachable destinations,
  - refresh while traffic is active.
- All new public classes and methods require Doxygen comments describing intent, parameters, return values, and failure behavior.

## Routing Decision Gate

Use this deterministic decision rule to avoid autopilot stalls:

- Milestones 1-4: implement and validate static route recomputation only.
- Milestone 5 Phase 5.2: evaluate AODV only after static routing passes all Milestone 4 verification checks.
- If static routing fails Milestone 4 checks after retry policy is exhausted, stop and request human decision before starting AODV migration.

## Retry, Stop, and Failure Policy

- Maximum automated retries per phase: 2.
- On build failure:
  - inspect compiler errors,
  - apply minimal fixes,
  - rebuild,
  - if still failing after retries, stop.
- On test failure:
  - inspect failing testcase output,
  - apply targeted fix,
  - rerun only affected testcase first,
  - rerun full required phase checks,
  - if still failing after retries, stop.
- On long-running commands:
  - wait for completion by default,
  - poll at most once per minute,
  - interrupt only if no apparent progress for 3 or more minutes.

## Per-Phase File Plan

The following file map is the default target list for autonomous edits.

### Milestone 1

- Phase 1.1:
  - add/update tests in `contrib/orbitshield/test/`.
  - update test registration in `contrib/orbitshield/test/test-orbitshield.cc`.
  - if needed, include new test source in `contrib/orbitshield/CMakeLists.txt`.
- Phase 1.2:
  - update API headers in `contrib/orbitshield/model/*.h`.
  - add stub/trivial implementations in `contrib/orbitshield/model/*.cc`.
  - update tests in `contrib/orbitshield/test/` to compile against the new APIs.
  - `contrib/orbitshield/README.md`

### Milestone 2

- Phase 2.1:
  - `contrib/orbitshield/model/satellite-net-device.h`
  - `contrib/orbitshield/model/satellite-net-device.cc`
  - `contrib/orbitshield/model/satellite-link.h`
  - `contrib/orbitshield/model/satellite-link.cc`
  - `contrib/orbitshield/model/constellation.h`
  - `contrib/orbitshield/model/constellation.cc`
  - relevant tests in `contrib/orbitshield/test/`.
- Phase 2.2:
  - `contrib/orbitshield/model/ground-station.h`
  - `contrib/orbitshield/model/ground-station.cc`
  - `contrib/orbitshield/model/constellation.cc`
  - relevant tests in `contrib/orbitshield/test/`.
  - `contrib/orbitshield/README.md`

### Milestone 3

- Phase 3.1:
  - add routing/stack helper implementation under `contrib/orbitshield/model/`.
  - update `contrib/orbitshield/model/orbitshield-module.h` if exports are required.
  - update `contrib/orbitshield/CMakeLists.txt` if new source files are added.
- Phase 3.2:
  - update/create tests in `contrib/orbitshield/test/` for ping path checks.
  - update registration in `contrib/orbitshield/test/test-orbitshield.cc`.
  - `contrib/orbitshield/README.md`

### Milestone 4

- Phase 4.1:
  - `contrib/orbitshield/model/constellation.h`
  - `contrib/orbitshield/model/constellation.cc`
  - routing helper files under `contrib/orbitshield/model/`.
- Phase 4.2:
  - dynamic behavior tests in `contrib/orbitshield/test/`.
  - test registration updates in `contrib/orbitshield/test/test-orbitshield.cc`.
  - `contrib/orbitshield/README.md`

### Milestone 5

- Phase 5.1:
  - advanced scenario tests in `contrib/orbitshield/test/`.
  - optional scenario data update under `contrib/orbitshield/data/` if additional ground stations are needed.
- Phase 5.2:
  - routing strategy integration changes in `contrib/orbitshield/model/`.
  - corresponding tests in `contrib/orbitshield/test/`.
  - `contrib/orbitshield/README.md`

### Milestone 6

- Phase 6.1:
  - cleanup in `contrib/orbitshield/model/` and `contrib/orbitshield/test/`.
- Phase 6.2:
  - `contrib/orbitshield/README.md` (final consistency pass)
  - this workplan file for final status and outcomes.

## Per-Phase Verification Matrix

Use these commands as mandatory checks.

### Common build check (all phases)

```bash
cd /home/marco/ns-3-dev
./ns3 build
```

Expected:

- Exit code 0.
- No compilation/link errors.

### Common suite check (all phases)

```bash
cd /home/marco/ns-3-dev
./ns3 run test-runner -- --suite=orbitshield
```

Expected:

- Exit code 0.
- `PASS orbitshield` present in output.

### Focused testcase check template (phase-local)

```bash
cd /home/marco/ns-3-dev
./ns3 run test-runner -- --suite=orbitshield --testcase=<TestCaseName>
```

Expected:

- Exit code 0 for required phase testcases.

## Per-Phase Status Update Schema

After each phase, update the phase block using this schema:

- Status: `Not implemented` | `In progress` | `Implemented` | `Blocked`
- Date: `YYYY-MM-DD`
- Commit: `<commit-hash>` for the final phase of a milestone only; `N/A` for all intermediate phases (commits happen once per completed milestone, not per phase)
- Commands run:
  - `<command 1>`
  - `<command 2>`
- Result summary:
  - `<short outcome>`
  - `<remaining issue, if any>`

This schema is required for reliable autopilot continuation across multiple runs.

## Which Milestone/Phase to Do Next

- The next work item is **Milestone 1, Phase 1.1**: build the initial test harness and verify that the Iridium YAML dataset loads correctly.
- After that, proceed to **Milestone 1, Phase 1.2** for API stubs and compile-time scaffolding.

---

The document is intended to be updated as implementation moves forward, with each milestone/phase receiving a status update after completion.
