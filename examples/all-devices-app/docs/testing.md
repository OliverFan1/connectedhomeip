# Testing & Simulation Guide

This document describes interactive and automated testing for the
`all-devices-app` simulator.

This application is used as a test target for Python integration scripts and
`chip-tool`.

---

## 1. Interactive Verification via `chip-tool`

### Prerequisite: Compile Target Applications

Before interactive testing, compile both the simulator and `chip-tool` from
source:

```bash
source scripts/activate.sh
./scripts/build/build_examples.py --target linux-x64-all-devices-clang build
./scripts/build/build_examples.py --target linux-x64-chip-tool-clang build
```

Use `chip-tool` to commission the simulator and execute Interaction Model
commands.

### Launch the Simulator (Terminal A)

Clean up persistent storage and launch a multi-endpoint topology (e.g., an
Occupancy Sensor on Endpoint 1 and a Chime on Endpoint 2):

```bash
source scripts/activate.sh
rm -rf /tmp/chip_all_devices_kvs
./out/linux-x64-all-devices-clang/all-devices-app \
    --device occupancy-sensor:1 \
    --device chime:2 \
    --KVS /tmp/chip_all_devices_kvs \
    --discriminator 3840
```

### Commission the Target (Terminal B)

Commission the simulator with `chip-tool` (assigning Node ID `1`):

```bash
source scripts/activate.sh
./out/linux-x64-chip-tool-clang/chip-tool pairing onnetwork 1 20202021
```

### Send Operational Commands

Interact with Code-Driven clusters on specific endpoints:

**Occupancy Sensor (Endpoint 1):**

```bash
# Read the current operational Occupancy state
./out/linux-x64-chip-tool-clang/chip-tool occupancy-sensing read occupancy 1 1
```

**Chime (Endpoint 2):**

```bash
# Invoke the Chime sound playback command (ChimeID 0 = Ding Dong)
./out/linux-x64-chip-tool-clang/chip-tool chime play-chime-sound 1 2 --ChimeID 0
```

---

## 2. Ambient Sensing Union Device Testing

### Launch the Simulator with Ambient Sensing Union

```bash
rm -rf /tmp/chip_all_devices_kvs
./out/linux-x64-all-devices-clang/all-devices-app \
    --device ambient-sensing-union:1 \
    --KVS /tmp/chip_all_devices_kvs \
    --app-pipe /tmp/chip_all_devices_fifo \
    --discriminator 3840
```

### Commission and Read Attributes via `chip-tool`

```bash
# Commission
./out/linux-x64-chip-tool-clang/chip-tool pairing onnetwork 1 20202021

# Read the union name attribute
./out/linux-x64-chip-tool-clang/chip-tool ambient-sensing-union read union-name 1 1

# Read the contributor list attribute
./out/linux-x64-chip-tool-clang/chip-tool ambient-sensing-union read contributor-list 1 1

# Read occupancy state (optional cluster, also present on same endpoint)
./out/linux-x64-chip-tool-clang/chip-tool occupancy-sensing read occupancy 1 1
```

### Named Pipe Commands (Terminal B)

The application opens a named FIFO at startup. Find the PID of the running
process and locate the pipe path:

**Set the union name:**

```bash
echo '{"Name": "SetAmbientSensingUnionName", "EndpointId": 1, "UnionName": "LivingRoomUnion"}' \
    > /tmp/chip_all_devices_fifo
```

**Add a Matter contributor (Online):**

```bash
echo '{"Name": "AddAmbientSensingContributor", "EndpointId": 1, "NodeId": "0x1234567890ABCDEF", "ContributorEndpointId": 2, "Status": 0}' \
    > /tmp/chip_all_devices_fifo
```

**Add a non-Matter contributor:**

```bash
echo '{"Name": "AddAmbientSensingNonMatterContributor", "EndpointId": 1, "ContributorName": "NonMatterSensor1", "Status": 0}' \
    > /tmp/chip_all_devices_fifo
```

**Update Matter contributor status to Offline:**

```bash
echo '{"Name": "UpdateAmbientSensingContributorStatus", "EndpointId": 1, "NodeId": "0x1234567890ABCDEF", "ContributorEndpointId": 2, "Status": 1}' \
    > /tmp/chip_all_devices_fifo
```

**Update non-Matter contributor status to Offline:**

```bash
echo '{"Name": "UpdateAmbientSensingContributorStatus", "EndpointId": 1, "ContributorName": "NonMatterSensor1", "Status": 1}' \
    > /tmp/chip_all_devices_fifo
```

**Remove a Matter contributor:**

```bash
echo '{"Name": "RemoveAmbientSensingContributor", "EndpointId": 1, "NodeId": "0x1234567890ABCDEF", "ContributorEndpointId": 2}' \
    > /tmp/chip_all_devices_fifo
```

**Remove a non-Matter contributor:**

```bash
echo '{"Name": "RemoveAmbientSensingNonMatterContributor", "EndpointId": 1, "ContributorName": "NonMatterSensor1"}' \
    > /tmp/chip_all_devices_fifo
```

### Named Pipe Status Values Reference

| Value | Meaning |
|-------|---------|
| `0`   | `Online` |
| `1`   | `Offline` |

---

## 3. Automated Integration Testing (`src/python_testing/`)

The SDK executes Python integration and certification test scripts against
`all-devices-app`.

### Setup the Python Environment

Before running tests, compile and activate the Python virtual environment:

```bash
./scripts/build_python.sh -i out/venv --enable_ipv4 true
source out/venv/bin/activate
```

### Execution Method A: Recommended CI Harness (`local.py`)

The automated test runner parses target definitions from test headers. Supply
the binary override path:

```bash
./scripts/tests/local.py python-tests \
    --test-filter TC_IDM_2_3 \
    --override-binary-path ALL_DEVICES_APP ./out/linux-x64-all-devices-clang/all-devices-app
```

### Execution Method B: Explicit Combined Harness (`run_python_test.py`)

To execute tests without CI metadata headers, invoke `run_python_test.py`:

```bash
./scripts/tests/run_python_test.py \
    --factory-reset \
    --app ./out/linux-x64-all-devices-clang/all-devices-app \
    --app-args "--device contact-sensor:1 --discriminator 1234 --KVS /tmp/chip_test_kvs" \
    --script src/python_testing/TC_IDM_2_3.py \
    --script-args "--storage-path admin_storage.json --commissioning-method on-network --discriminator 1234 --passcode 20202021 --PICS src/app/tests/suites/certification/ci-pics-values"
```

### Execution Method C: Decoupled Interactive Harness (Two Terminals)

For direct GDB attachment or live logging observation, execute the app and
script separately:

**1. Launch the Target under Test:**

```bash
./out/linux-x64-all-devices-clang/all-devices-app --device on-off-light:1 --discriminator 1234 --KVS /tmp/test_kvs
```

**2. Execute the verification script:**

```bash
source out/venv/bin/activate
python3 src/python_testing/TC_IDM_2_3.py \
    --storage-path admin_storage.json \
    --commissioning-method on-network \
    --discriminator 1234 \
    --passcode 20202021 \
    --endpoint 1
```
