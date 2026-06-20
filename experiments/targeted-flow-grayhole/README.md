# Targeted Flow Grayhole Experiment

This experiment runs a deterministic route-conditioned grayhole workflow on the OrbitShield Iridium dataset. It uses generic OrbitShield models for topology, routing, route introspection, grayhole forwarding policy, and route exclusion, while the profile parser, detector, telemetry collector, and runner live in this directory.

## Run

From the ns-3 root:

```bash
./ns3 run orbitshield-targeted-flow-grayhole -- \
  --config=contrib/orbitshield/experiments/targeted-flow-grayhole/profiles/targeted-flow-grayhole.yaml \
  --durationSeconds=300 \
  --outputDir=contrib/orbitshield/results/targeted-flow-grayhole-smoke
```

Supported scalar overrides are `--durationSeconds`, `--refreshIntervalSeconds`, `--attackDropProbability`, `--mitigationEnabled=true|false`, and `--outputDir`.

## Profile

The default profile is [profiles/targeted-flow-grayhole.yaml](profiles/targeted-flow-grayhole.yaml). It references the shared constellation metadata at [../../data/iridium-20260312.yaml](../../data/iridium-20260312.yaml) through a path resolved relative to the profile file.

Top-level maps:

| Section | Purpose |
|---|---|
| `constellation` | Ring/TLE metadata file |
| `simulation` | Duration and ns-3 RNG seed/run values |
| `topology` | ISL/GSL range and refresh cadence |
| `traffic` | ICMP-style ground-station traffic matrix and packet parameters |
| `attack` | Compromised satellites, target ground pairs, direction, timing, and drop probability |
| `detection` | Deterministic detector thresholds and minimum sample count |
| `mitigation` | Route-exclusion enable flag, delay, and exclusion cap |
| `telemetry` | Output directory, route snapshot cadence, and CSV enable flag |

## Telemetry

When `telemetry.writeCsv` is true, the runner writes these artifacts:

| File | Contents |
|---|---|
| `flow_samples.csv` | Flow window delivery counts, PDR, RTT, and attack-window labels |
| `route_snapshots.csv` | Target flow route membership snapshots |
| `forwarding_events.csv` | Grayhole forwarding decisions and drop reasons |
| `node_labels.csv` | Compromised and flagged node labels |
| `mitigation_events.csv` | Detector flag/exclusion and route recomputation actions |

The detector is deterministic route/PDR scoring. It does not use a trained model or external inference runtime.

## Validation

```bash
./ns3 run "test-runner --suite=orbitshield"
./ns3 run orbitshield-targeted-flow-grayhole -- --help
```
