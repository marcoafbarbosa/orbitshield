### 0) Plan Title
- Targeted Flow Grayhole Refactor

### 1) Goal and Assumptions
- Goal: refactor the implemented grayhole work so OrbitShield generic functionality stays in `contrib/orbitshield/model`, while the targeted-flow grayhole research workflow lives under `contrib/orbitshield/experiments/targeted-flow-grayhole` with names that are meaningful to future OrbitShield users.
- Default implementation working directory: `contrib/orbitshield`.
- Commit operations must be performed in the OrbitShield Git repository rooted at `contrib/orbitshield`, not from the parent ns-3 repository unless explicitly needed for inspection only.
- Assumption: the existing Scenario 3 implementation has already landed in `model/orbitshield-scenario3-*`, `examples/orbitshield-scenario3-grayhole.cc`, `data/scenarios/scenario3-grayhole.yaml`, README sections, and `OrbitShieldScenario3*` tests; this plan refactors that implementation without changing its simulation behavior.
- Assumption: `targeted-flow-grayhole` is the public experiment name. The term `scenario3` should be removed from user-facing file names, executable names, profile paths, test case names, README sections, and generated artifact directories.
- Assumption: `OrbitShieldGrayholePolicy`, `SatelliteNetDevice` forwarding-policy support, route introspection, and satellite route exclusion are generic OrbitShield capabilities and remain in `model/` after removing any dependency on experiment-specific headers.
- Assumption: experiment-specific profile parsing, workload orchestration, deterministic detector policy, telemetry CSV schema, and the runnable experiment entry point move to `experiments/targeted-flow-grayhole/`.
- Assumption: tests remain registered in the `orbitshield` suite under `contrib/orbitshield/test`; they may compile experiment implementation files as test-only sources, but experiment-specific code should not be exported through `model/orbitshield-module.h` unless it is intentionally promoted to generic API.
- Out of scope: redesigning the grayhole algorithm, changing routing semantics, adding a trained AI detector, changing the Iridium dataset, or adding new attack families.

### 2) Reuse Inventory
- Reuse `Satellite`, `GroundStation`, `Constellation`, `SatelliteLink`, `SatelliteNetDevice`, `IslPropagationDelayModel`, and `OrbitShieldRoutingHelper` unchanged except where includes or generic API names must be cleaned up.
- Reuse `OrbitShieldGrayholePolicy` as the generic targeted-flow drop policy, but decouple it from the current `OrbitShieldScenario3Direction` enum by introducing a generic policy direction enum or equivalent local type.
- Reuse route path introspection and satellite exclusion APIs already implemented in `OrbitShieldRoutingHelper`; they are useful beyond this experiment.
- Reuse existing deterministic tests from `test/test-routing.cc` as behavioral coverage, renaming Scenario 3 test cases to targeted-flow grayhole names where appropriate.
- Reuse `yaml-cpp` for experiment profile parsing, but link it to experiment/test targets as needed instead of forcing all experiment code into the base OrbitShield model list.
- Reuse the existing `examples/` directory only for instructional examples. The full attack workflow should move to `experiments/targeted-flow-grayhole/` following the common addon pattern where examples are small demos and experiments contain research workflows.
- Gap: no `experiments/` build structure currently exists in OrbitShield.
- Gap: `OrbitShieldGrayholePolicy` currently depends on a Scenario 3 config header for direction values.
- Gap: README currently presents the attack workflow as “Scenario 3” and lists experiment support as base model components.
- Gap: validation commands and smoke-run paths currently use `orbitshield-scenario3-grayhole` and `data/scenarios/scenario3-grayhole.yaml`.

### 3) Task Plan

#### T1: Separate Generic Grayhole API
- Why this task exists: the base model layer must not include experiment-specific Scenario 3 headers, names, or concepts.
- Files likely touched:
  - `model/orbitshield-grayhole-policy.h`
  - `model/orbitshield-grayhole-policy.cc`
  - `model/satellite-net-device.h`
  - `model/satellite-net-device.cc`
  - `model/orbitshield-routing-helper.h`
  - `model/orbitshield-routing-helper.cc`
  - `model/orbitshield-module.h`
  - `test/test-routing.h`
  - `test/test-routing.cc`
- Implementation notes:
  - Replace `#include "orbitshield-scenario3-config.h"` in `OrbitShieldGrayholePolicy` with a generic policy direction declaration, for example `enum class OrbitShieldGrayholeDirection { FORWARD, REVERSE, BIDIRECTIONAL };`.
  - Update `SetDirection` and internal matching code to use the generic enum.
  - Keep the public policy name `OrbitShieldGrayholePolicy` because it describes a reusable attack model, not the experiment harness.
  - Preserve route-conditioned behavior: target-flow matching by IPv4 pair, attack window, compromised satellite membership, route-active satellite membership, and deterministic `dropProbability=1.0` behavior in tests.
  - Confirm `model/orbitshield-module.h` exports only generic model headers after this task.
- Acceptance criteria:
  - No file in `contrib/orbitshield/model` includes `orbitshield-scenario3-config.h` or any future `targeted-flow-grayhole-*` experiment header.
  - `OrbitShieldGrayholePolicyTest` still passes through the full `orbitshield` suite and verifies target-only drops, non-target forwarding, outside-window forwarding, and forwarding decision trace counts.
  - Existing routing, route membership, and route exclusion tests still pass with no API regression.
- Validation commands:
  - From `contrib/orbitshield`: `rg "scenario3|Scenario3|targeted-flow-grayhole" model`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield --verbose"`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield"`

#### T2: Create Experiment Directory Build
- Why this task exists: OrbitShield needs a first-class location for research workflows without mixing them into `model/` or instructional `examples/`.
- Files likely touched:
  - `CMakeLists.txt`
  - `experiments/CMakeLists.txt`
  - `experiments/targeted-flow-grayhole/CMakeLists.txt`
  - `examples/CMakeLists.txt`
  - `examples/orbitshield-scenario3-grayhole.cc`
- Implementation notes:
  - Add `experiments/targeted-flow-grayhole/` as the experiment root.
  - Move the current runner from `examples/orbitshield-scenario3-grayhole.cc` to `experiments/targeted-flow-grayhole/targeted-flow-grayhole.cc`.
  - Remove the Scenario 3 executable from `examples/CMakeLists.txt`.
  - Add an executable target named `orbitshield-targeted-flow-grayhole` built from the experiment runner and experiment support sources.
  - Prefer the existing ns-3 CMake helpers where possible. If `build_lib_example` is unsuitable outside `examples/`, use `build_exec` consistently with the existing `orbitshield-isl-visualizer` and `orbitshield-dynamic-topology` targets.
  - Set the runtime output path under `contrib/orbitshield/` so it is runnable through `../../ns3 run orbitshield-targeted-flow-grayhole`.
- Acceptance criteria:
  - `../../ns3 build` succeeds after adding the experiment target.
  - `../../ns3 run orbitshield-targeted-flow-grayhole -- --help` reaches the executable command-line parser and exits successfully.
  - `../../ns3 run orbitshield-scenario3-grayhole` is no longer documented or required as an OrbitShield validation command.
  - Existing instructional examples remain in `examples/` and still build.
- Validation commands:
  - From `contrib/orbitshield`: `../../ns3 build`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-targeted-flow-grayhole -- --help`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-basic-example`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-load-from-yaml`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-dynamic-topology`

#### T3: Move Experiment Configuration
- Why this task exists: profile parsing and experiment defaults are specific to targeted-flow grayhole runs and should not be part of the base model API.
- Files likely touched:
  - `model/orbitshield-scenario3-config.h`
  - `model/orbitshield-scenario3-config.cc`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-config.h`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-config.cc`
  - `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml`
  - `data/scenarios/scenario3-grayhole.yaml`
  - `CMakeLists.txt`
  - `experiments/targeted-flow-grayhole/CMakeLists.txt`
  - `test/test-routing.h`
  - `test/test-routing.cc`
- Implementation notes:
  - Rename config types from `OrbitShieldScenario3Config` to `OrbitShieldTargetedFlowGrayholeConfig`.
  - Move the default profile from `data/scenarios/scenario3-grayhole.yaml` to `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml`.
  - Keep shared constellation data in `data/iridium-20260312.yaml`; configure the moved profile to reference it by a robust relative path, for example `../../../data/iridium-20260312.yaml` from the profile directory.
  - Update default telemetry output from `results/scenario3` to `results/targeted-flow-grayhole`.
  - Keep loader validation behavior unchanged: path resolution, enum validation, positive durations, valid drop probability, configured ground stations, and satellite-name validation after constellation load.
  - Update tests to load the moved profile and rename the config test case away from Scenario 3.
- Config files to add/update:
  - Add `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml`.
  - Remove or deprecate `data/scenarios/scenario3-grayhole.yaml` once all tests/docs/commands use the new location.
- Parameter table:

| Name | Meaning | Type | Default | Valid Range |
|---|---|---|---|---|
| `constellation.ringFile` | Shared OrbitShield ring/TLE metadata file | string path | `../../../data/iridium-20260312.yaml` relative to profile | existing readable YAML |
| `simulation.durationSeconds` | Experiment run duration | double | `3000.0` | `> 0` |
| `simulation.seed` | ns-3 RNG seed | uint32 | `1` | `1..4294967295` |
| `simulation.run` | ns-3 RNG run | uint32 | `1` | `1..4294967295` |
| `topology.islMaxRangeMeters` | ISL creation range | double | `2000000.0` | `> 0` |
| `topology.groundMaxRangeMeters` | GSL creation range | double | `50000000.0` | `> 0` |
| `topology.refreshIntervalSeconds` | Dynamic topology refresh interval | double | `30.0` | `> 0` |
| `traffic.pingIntervalSeconds` | Ping interval for each configured pair | double | `30.0` | `> 0` |
| `traffic.pingSizeBytes` | ICMP payload size | uint32 | `56` | `1..65507` |
| `traffic.pairs` | Ground-station traffic matrix | list of `{source,destination}` | all 10 unordered Iridium GS pairs | names present in constellation |
| `attack.compromisedSatellites` | Satellites configured to behave as grayhole relays | list string | `IRIDIUM 113` | names present in constellation |
| `attack.targetPairs` | Ground pairs affected by targeted drops | list of `{source,destination}` | `Tempe -> Fairbanks` | subset of `traffic.pairs` |
| `attack.direction` | Target matching direction | enum string | `bidirectional` | `forward`, `reverse`, `bidirectional` |
| `attack.startSeconds` | Attack activation start | double | `600.0` | `>= 0`, `< duration` |
| `attack.stopSeconds` | Attack activation stop | double | `2400.0` | `> start`, `<= duration` |
| `attack.dropProbability` | Probability to drop a matching packet | double | `1.0` | `0.0..1.0` |
| `detection.enabled` | Enable deterministic route-based detector | bool | `true` | `true/false` |
| `detection.windowSeconds` | Detection aggregation window | double | `120.0` | `>= refreshIntervalSeconds` |
| `detection.minSamples` | Minimum target-flow samples before scoring | uint32 | `3` | `>= 1` |
| `detection.targetPdrThreshold` | PDR below which target route nodes receive suspicion | double | `0.6` | `0.0..1.0` |
| `detection.scoreThreshold` | Score needed to flag a satellite | double | `1.0` | `>= 0` |
| `mitigation.enabled` | Enable route exclusion after detection | bool | `true` | `true/false` |
| `mitigation.applyDelaySeconds` | Delay from flag to route exclusion | double | `30.0` | `>= 0` |
| `mitigation.maxExcludedSatellites` | Cap on excluded satellites | uint32 | `4` | `>= 0` |
| `telemetry.outputDir` | Directory for experiment artifacts | string path | `results/targeted-flow-grayhole` | writable path |
| `telemetry.routeSnapshotIntervalSeconds` | Route snapshot cadence | double | `30.0` | `> 0` |
| `telemetry.writeCsv` | Emit CSV telemetry artifacts | bool | `true` | `true/false` |

- How tests load and assert behavior for different parameter sets:
  - `OrbitShieldTargetedFlowGrayholeConfigTest` loads `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml` and verifies defaults, path resolution, target-pair parsing, and valid enum handling.
  - The same test writes temporary YAML variants under the ns-3 test temp directory to verify compromised-set sizes `1`, `2`, and `4`, disabled mitigation, forward-only targeting, reverse-only targeting, and invalid values.
- Acceptance criteria:
  - `model/orbitshield-scenario3-config.*` no longer exists or is no longer referenced by any build target.
  - `rg "scenario3|Scenario3" experiments/targeted-flow-grayhole test README.md CMakeLists.txt examples` returns no user-facing Scenario 3 names except in migration notes if explicitly retained.
  - The renamed config test passes through the full `orbitshield` suite.
  - Malformed profiles still fail cleanly with an error status/message.
- Validation commands:
  - From `contrib/orbitshield`: `rg "orbitshield-scenario3-config|OrbitShieldScenario3Config|scenario3-grayhole"`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield --verbose"`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield"`

#### T4: Move Experiment Runtime Code
- Why this task exists: telemetry, detector, and runner orchestration are research-workflow code and should live with the experiment that owns their assumptions.
- Files likely touched:
  - `model/orbitshield-scenario3-detector.h`
  - `model/orbitshield-scenario3-detector.cc`
  - `model/orbitshield-scenario3-experiment.h`
  - `model/orbitshield-scenario3-experiment.cc`
  - `model/orbitshield-scenario3-telemetry.h`
  - `model/orbitshield-scenario3-telemetry.cc`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-detector.h`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-detector.cc`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-runner.h`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-runner.cc`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-telemetry.h`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole-telemetry.cc`
  - `experiments/targeted-flow-grayhole/targeted-flow-grayhole.cc`
  - `CMakeLists.txt`
  - `experiments/targeted-flow-grayhole/CMakeLists.txt`
  - `test/test-routing.h`
  - `test/test-routing.cc`
- Implementation notes:
  - Rename runtime types from `OrbitShieldScenario3Telemetry`, `OrbitShieldScenario3Detector`, and `OrbitShieldScenario3ExperimentSummary` to targeted-flow grayhole names.
  - Keep the detector description truthful: deterministic route/PDR scoring for this experiment, not a trained AI model.
  - Keep CSV schemas stable unless a name contains Scenario 3. Prefer artifact names that describe content: `flow_samples.csv`, `route_snapshots.csv`, `forwarding_events.csv`, `node_labels.csv`, and `mitigation_events.csv`.
  - Build the experiment executable from these files without adding them to `SOURCE_FILES` or `HEADER_FILES` for the base `orbitshield` library.
  - For unit tests, either add the experiment implementation files to `TEST_SOURCES` or factor testable experiment helpers into the experiment target and compile them test-only. Do not reintroduce the experiment code into `model/` for convenience.
- Acceptance criteria:
  - `model/orbitshield-scenario3-detector.*`, `model/orbitshield-scenario3-experiment.*`, and `model/orbitshield-scenario3-telemetry.*` no longer exist or are no longer referenced by any build target.
  - `model/orbitshield-module.h` does not export targeted-flow grayhole experiment headers.
  - The renamed telemetry test verifies in-memory records, CSV headers, and at least one data row per enabled artifact.
  - The renamed detector test verifies low-PDR target windows flag only satellites on target routes and non-target traffic does not create flags.
  - The renamed experiment test verifies a shortened deterministic run with mitigation and a no-mitigation variant.
- Validation commands:
  - From `contrib/orbitshield`: `rg "OrbitShieldScenario3|scenario3|Scenario 3" model experiments test README.md CMakeLists.txt examples`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield --verbose"`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield"`

#### T5: Rename Tests and Fixtures
- Why this task exists: validation should describe the supported feature, not the old planning-list number.
- Files likely touched:
  - `test/test-routing.h`
  - `test/test-routing.cc`
  - `test/test-orbitshield.cc`
  - `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml`
  - `CMakeLists.txt`
- Implementation notes:
  - Rename Scenario 3 test classes and display names to targeted-flow grayhole names.
  - Suggested mappings:
    - `OrbitShieldScenario3ConfigTest` -> `OrbitShieldTargetedFlowGrayholeConfigTest`
    - `OrbitShieldScenario3TelemetryTest` -> `OrbitShieldTargetedFlowGrayholeTelemetryTest`
    - `OrbitShieldScenario3DetectorTest` -> `OrbitShieldTargetedFlowGrayholeDetectorTest`
    - `OrbitShieldScenario3ExperimentTest` -> `OrbitShieldTargetedFlowGrayholeExperimentTest`
  - Keep generic tests unchanged when they validate generic behavior, for example `OrbitShieldGrayholePolicyTest`, `OrbitShieldRouteMembershipTest`, and `OrbitShieldRouteExclusionTest`.
  - Update temporary profile helper names and fixture comments to avoid Scenario 3 terminology.
  - Keep assertions behavior-based, not path-string-only.
- Acceptance criteria:
  - Verbose test-runner output shows the renamed targeted-flow grayhole test case names.
  - Full `orbitshield` suite passes.
  - `rg "Scenario3|Scenario 3|scenario3" test experiments README.md CMakeLists.txt examples model` returns no unintended references.
- Validation commands:
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield --verbose"`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield"`
  - From `contrib/orbitshield`: `rg "Scenario3|Scenario 3|scenario3" test experiments README.md CMakeLists.txt examples model`

#### T6: Document Experiment Layout
- Why this task exists: README is the primary OrbitShield documentation source and must teach users where generic models end and experiments begin.
- Files likely touched:
  - `README.md`
  - `experiments/targeted-flow-grayhole/README.md`
  - `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml`
- Implementation notes:
  - Preserve the current README style, tone, and overall layout.
  - Replace “Scenario 3 Grayhole Experiments” with “Targeted Flow Grayhole Experiment”.
  - Add `experiments/` to the module structure section, with a short explanation that each subdirectory contains a research workflow built on generic OrbitShield models.
  - Move experiment-specific component descriptions out of “Models and Core Components” and into a dedicated experiment section.
  - Keep `OrbitShieldGrayholePolicy` documented as generic model functionality.
  - Document the new runner command, profile path, telemetry output path, and command-line overrides.
  - Add a concise experiment-local README only if it helps keep the top-level README readable; it should explain purpose, profile fields, runner command, telemetry artifacts, and validation commands without task-tracking language.
  - Do not claim a trained AI model exists. Describe the detector as deterministic/configurable route/PDR scoring.
- Acceptance criteria:
  - Top-level README accurately documents `experiments/targeted-flow-grayhole/` and no longer presents the workflow as Scenario 3.
  - README preserves current layout and does not advertise unimplemented capabilities.
  - The test table includes renamed targeted-flow grayhole tests.
  - The examples list no longer includes `orbitshield-scenario3-grayhole`.
  - Documentation commands run successfully.
- Validation commands:
  - From `contrib/orbitshield`: `rg "Scenario3|Scenario 3|scenario3|orbitshield-scenario3-grayhole" README.md experiments examples model test CMakeLists.txt`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-targeted-flow-grayhole -- --config=contrib/orbitshield/experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml --durationSeconds=300 --outputDir=contrib/orbitshield/results/targeted-flow-grayhole-smoke`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield"`

#### T7: Validate Refactored Workflow
- Why this task exists: the refactor is complete only if the moved experiment builds, runs, produces artifacts, and leaves generic OrbitShield examples/tests intact.
- Files likely touched:
  - `CMakeLists.txt`
  - `experiments/CMakeLists.txt`
  - `experiments/targeted-flow-grayhole/CMakeLists.txt`
  - `README.md`
  - `.workplans/20260606-targeted-flow-grayhole-refactoring.md`
- Implementation notes:
  - Run the full OrbitShield suite after all moves and renames.
  - Run the new targeted-flow grayhole smoke command with a shortened duration and output directory under `contrib/orbitshield/results/targeted-flow-grayhole-smoke`.
  - Run existing OrbitShield instructional examples to confirm the `examples/` cleanup did not break existing targets.
  - Run `rg` checks for old Scenario 3 names and decide whether any remaining hits are acceptable historical references inside `.workplans/` only. User-facing code/docs should be clean.
  - Update this workplan status table with verification evidence after implementation, before committing.
- Acceptance criteria:
  - Full `orbitshield` test suite passes.
  - `orbitshield-targeted-flow-grayhole` smoke run exits successfully and writes telemetry artifacts to the requested output directory.
  - Existing `orbitshield-basic-example`, `orbitshield-load-from-yaml`, and `orbitshield-dynamic-topology` examples run successfully.
  - No user-facing code, build target, profile path, README section, or test name uses Scenario 3 terminology.
  - Any commit is created from the OrbitShield repository rooted at `contrib/orbitshield`.
- Validation commands:
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield"`
  - From `contrib/orbitshield`: `../../ns3 run "test-runner --suite=orbitshield --verbose"`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-targeted-flow-grayhole -- --config=contrib/orbitshield/experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml --durationSeconds=300 --outputDir=contrib/orbitshield/results/targeted-flow-grayhole-smoke`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-basic-example`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-load-from-yaml`
  - From `contrib/orbitshield`: `../../ns3 run orbitshield-dynamic-topology`
  - From `contrib/orbitshield`: `git status --short`

### 4) Test Matrix

| Test | Behavior Verified | Failure Signal | Determinism Notes |
|---|---|---|---|
| `OrbitShieldTargetedFlowGrayholeConfigTest` | Experiment profile parsing, defaults, variants, invalid values, and relative path resolution after moving profiles | Moved default profile fails to load, bad enum accepted, invalid values silently pass, old profile path still required | Uses fixed YAML profile plus temporary YAML fixtures |
| `OrbitShieldRouteMembershipTest` | Generic current route path and satellite transit membership for a target ground-station pair | Empty path, invalid edge transition, missing transit satellite on multi-hop route | Uses fixed Iridium YAML and shortest-hop routing |
| `OrbitShieldGrayholePolicyTest` | Generic route-conditioned grayhole policy drops only target packets inside the attack window | Target packets forwarded during attack, non-target packets dropped, outside-window packet dropped, trace counts wrong | Uses `dropProbability=1.0` and controlled timings |
| `OrbitShieldTargetedFlowGrayholeTelemetryTest` | Experiment in-memory records and CSV artifacts after moving telemetry code | Missing record type, wrong attack-active label, missing CSV header/data row | Uses temporary output directory and fixed event sequence |
| `OrbitShieldRouteExclusionTest` | Generic excluded satellites are omitted from recomputed routes and stale host routes are removed | Excluded satellite remains on path or stale host route remains | Uses deterministic recomputation after explicit exclusion |
| `OrbitShieldTargetedFlowGrayholeDetectorTest` | Experiment detector scores low-PDR target-route satellites only | Non-route satellite flagged, non-target traffic affects target flags, score threshold ignored | Uses synthetic telemetry windows and fixed thresholds |
| `OrbitShieldTargetedFlowGrayholeExperimentTest` | Short deterministic targeted-flow grayhole run exercises attack, detection, mitigation, telemetry | No drops, no mitigation when enabled, mitigation when disabled, output summary missing | Uses shortened profile, fixed seed/run, and deterministic drop probability |
| Existing `OrbitShieldMultiGroundStationRoutingTest` | Baseline multi-GS routing remains intact with no attack policy configured | Delivery ratio, RTT, or hop count regression | Existing deterministic workload unchanged |
| Existing `OrbitShieldStaticRoutingStrategyTest` | Fast refresh route recomputation remains robust after route-exclusion changes remain generic | Crash/assertion or no delivery under fast refresh | Existing deterministic workload unchanged |
| `orbitshield-targeted-flow-grayhole` smoke run | New experiment executable loads moved profile, applies overrides, writes telemetry | Executable not found, profile not found, nonzero exit, missing artifact directory | Uses shortened duration and explicit output directory |

### 5) Execution Order and Risk Control
- Recommended implementation order:
  1. T1 Separate Generic Grayhole API.
  2. T2 Create Experiment Directory Build.
  3. T3 Move Experiment Configuration.
  4. T4 Move Experiment Runtime Code.
  5. T5 Rename Tests and Fixtures.
  6. T6 Document Experiment Layout.
  7. T7 Validate Refactored Workflow.
- Risk 1: moving experiment sources out of the base library may break test linkage because current tests include or instantiate Scenario 3 classes through `orbitshield-module.h`.
- Mitigation 1: compile experiment implementation files as test-only sources or include experiment headers directly in the tests; do not move them back into `model/` just to satisfy linkage.
- Risk 2: ns-3 CMake helpers may not automatically recurse into a new `experiments/` directory the same way they handle `examples/`.
- Mitigation 2: explicitly include `experiments/targeted-flow-grayhole` from OrbitShield `CMakeLists.txt` and use `build_exec` if `build_lib_example` is constrained to example directories.
- Risk 3: relative paths in the moved YAML profile can become fragile when running from ns-3 root versus `contrib/orbitshield`.
- Mitigation 3: keep loader path resolution based on the profile file directory, add config tests for moved profile resolution, and validate with the exact `../../ns3 run orbitshield-targeted-flow-grayhole -- --config=...` command.

### 6) Definition of Done
- All implementation steps are performed from `contrib/orbitshield` unless a command explicitly requires the ns-3 root equivalent.
- Any commits are made in the OrbitShield repository rooted at `contrib/orbitshield`.
- Generic OrbitShield functionality remains in `model/`: satellite models, links, routing helper, route introspection, route exclusion, `SatelliteNetDevice` forwarding-policy hook, and `OrbitShieldGrayholePolicy`.
- Experiment-specific functionality lives under `experiments/targeted-flow-grayhole/`: profile loader, default profile, telemetry collector, deterministic detector, runner orchestration, and executable entry point.
- `model/` does not include or depend on targeted-flow grayhole experiment headers.
- The executable is named `orbitshield-targeted-flow-grayhole`.
- The default profile is `experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml`.
- Default telemetry output uses `results/targeted-flow-grayhole`.
- User-facing code, test names, README sections, profile paths, and executable names no longer use `scenario3`, `Scenario3`, or `Scenario 3`.
- README explains the `experiments/` structure and accurately documents the targeted-flow grayhole experiment while preserving current style, tone, and layout.
- New or renamed tests remain in `contrib/orbitshield/test` and are registered in the `orbitshield` suite.
- These commands pass from `contrib/orbitshield` before completion:
  - `../../ns3 run "test-runner --suite=orbitshield"`
  - `../../ns3 run "test-runner --suite=orbitshield --verbose"`
  - `../../ns3 run orbitshield-targeted-flow-grayhole -- --config=contrib/orbitshield/experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml --durationSeconds=300 --outputDir=contrib/orbitshield/results/targeted-flow-grayhole-smoke`
  - `../../ns3 run orbitshield-basic-example`
  - `../../ns3 run orbitshield-load-from-yaml`
  - `../../ns3 run orbitshield-dynamic-topology`
- For clean setup or CI parity, these root-level commands also pass when needed:
  - `./ns3 configure --enable-tests --enable-examples`
  - `./ns3 build`

### 7) Current Task Implementation Status

| Task ID | Task Name | Status | Last Verification | Commit | Notes |
|---|---|---|---|---|---|
| T1 | Separate Generic Grayhole API | implemented | `../../ns3 run "test-runner --suite=orbitshield --verbose"` pass; `../../ns3 run "test-runner --suite=orbitshield"` pass; broad `rg` still reports Scenario 3 files scheduled for T3/T4 | 63e309f | Generic grayhole policy no longer includes Scenario 3 config; module umbrella exports generic headers only |
| T2 | Create Experiment Directory Build | implemented | `../../ns3 build` pass; `../../ns3 run orbitshield-targeted-flow-grayhole -- --help` pass; basic, YAML, and dynamic-topology examples pass | cb47afb | Runner moved to `experiments/targeted-flow-grayhole/`; old Scenario 3 example target removed from examples CMake |
| T3 | Move Experiment Configuration | implemented | `../../ns3 build` pass; verbose and non-verbose `orbitshield` suite pass; `rg "orbitshield-scenario3-config|OrbitShieldScenario3Config|scenario3-grayhole"` now reports README docs only | 0f2151c | Config/profile moved under `experiments/targeted-flow-grayhole/`; config test renamed; README/runtime Scenario names remain for T4-T6 |
| T4 | Move Experiment Runtime Code | implemented | `../../ns3 build` pass; verbose and non-verbose `orbitshield` suite pass; Scenario-name audit clean for code/build/tests except README docs | 8d20498 | Detector, telemetry, runner, and config now compile as experiment/test-only sources outside the base model library |
| T5 | Rename Tests and Fixtures | implemented | Verbose and non-verbose `orbitshield` suite pass; verbose output shows targeted-flow grayhole test names; Scenario-name audit reports README docs only | 2546a28 | No file edits in T5; required test/fixture renames were completed by the T4 runtime API rename |
| T6 | Document Experiment Layout | implemented | Scenario-name doc audit pass; documented smoke command pass; `../../ns3 run "test-runner --suite=orbitshield"` pass | b12f1aa | README documents experiment layout, runner/profile/output paths, renamed tests, and experiment-local README |
| T7 | Validate Refactored Workflow | implemented | Final non-verbose and verbose `orbitshield` suites pass; targeted-flow smoke run pass with all CSV artifacts verified; basic, YAML, and dynamic-topology examples pass; old-name audit pass | this commit | Generated smoke artifacts verified and removed before commit; unrelated untracked workplans/project left untouched |
