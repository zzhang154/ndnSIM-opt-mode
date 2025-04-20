# Task: Implement Hierarchical Data Aggregation in ndnSIM (CFNAgg)

You are required to implement a minimal executable demo of a hierarchical data aggregation system (**CFNAgg**) in ndnSIM 2.9, based on the provided official ndnSIM application examples (`src/ndnSIM/apps/`). The demo involves three types of applications: producer, aggregator, and root consumer. 

Explicitly implement the following components:

## Components to Implement

### 1. **CFNProducerApp** (`CFNProducerApp.hpp/cpp`):
- Inherit from `ndn::Producer` (`ndn-producer.hpp/cpp`).
- Runs on leaf nodes (`Leaf1`, `Leaf2`).
- Registers a unique prefix (e.g., node name).
- Responds to incoming Interests with Data packets containing a numeric payload (default value can be `1`, configurable).

### 2. **CFNAggregatorApp** (`CFNAggregatorApp.hpp/cpp`):
- Refer to `ndn::App` (`ndn-app.hpp`), `ndn::Producer` (`ndn-producer.hpp/cpp`), and `ndn::Consumer` (`ndn-consumer.hpp/cpp`) to implement dual functionality (consumer & producer) correctly.
- Runs on intermediate aggregator nodes (`Agg1`).
- Behavior specification:
  - Receives Interests from the parent node (`Root`), then immediately forwards Interests with the same sequence number to all child nodes (`Leaf1`, `Leaf2`).
  - Maintains aggregation state per sequence number using an `AggregationBuffer` class.
  - Aggregates received numeric Data payloads from children (simple summation).
  - Produces an aggregated Data packet upon receiving all expected child responses or upon a configurable timeout.
  - Implements partial aggregation if timeout occurs before all child Data is received.

### 3. **CFNRootApp** (`CFNRootApp.hpp/cpp`):
- Inherit from `ndn::Consumer` (`ndn-consumer.hpp`).
- Runs on the root node (`Root`).
- Explicitly generates a series of Interests toward its child (`Agg1`), each Interest marked with an incremental sequence number to indicate aggregation rounds.
- Logs or prints the final aggregated Data received from the aggregator node (`Agg1`) for each sequence number.

### 4. **AggregationBuffer** (`AggregationBuffer.hpp/cpp`):
- Tracks the aggregation status for each Interest sequence number at aggregator nodes.
- Records:
  - Number of expected vs. received child Data responses.
  - Partial aggregation results (sum of numeric payloads).
  - Status of responses from individual child nodes.
  - Timeout handling to ensure timely responses to parent nodes.

### 5. **Simulation Main File** (`src/ndnSIM/examples/cfnagg/cfnagg-simulation.cpp`):
- Implement the `main` function that sets up the network topology and aggregation tree according to provided files (`topologies/dcn.txt` and `topologies/aggtree-dcn.txt`).
- Instantiate and install the respective applications (`CFNProducerApp`, `CFNAggregatorApp`, `CFNRootApp`) on the appropriate nodes (`Leaf1`, `Leaf2`, `Agg1`, `Root`).
- Include simulation timing controls, interest generation at the Root node, and logging statements for verification.

### 6. **CFNAggConfig** (`CFNAggConfig.hpp/cpp`):
- Implement a configuration class to manage user input parameters, avoiding complexity in the `main` function.
- Parameters to manage:
  - Number of aggregation rounds (default: 5).
  - Partial aggregation timeout in milliseconds (default: 20ms).
  - Producer payload values (default: node identifier).
  - Simulation stop time (calculated based on aggregation rounds and expected round duration).
- Provide methods to parse and set these parameters, possibly from command-line arguments or a configuration file.

---

## Testing Requirements

Use the provided minimal testing topologies to verify correctness:

### -- Network Topology (`topologies/dcn.txt`):

```text
# Minimal DCN topology for CFNAgg test
router

# node    comment     yPos    xPos
Root      NA          1       2
Agg1      NA          2       2
Leaf1     NA          3       1
Leaf2     NA          3       3

link

# srcNode  dstNode    bandwidth   delay   queue
Root       Agg1       10Mbps      2ms     20
Agg1       Leaf1      10Mbps      1ms     20
Agg1       Leaf2      10Mbps      1ms     20
```

### -- Aggregation Tree (`topologies/aggtree-dcn.txt`):
```text
# Minimal Aggregation Tree for CFNAgg test
# Format: ParentNode Child1 Child2 ...

Root Agg1
Agg1 Leaf1 Leaf2
```

---

## Explicit Clarifications for the task:

- The Root node explicitly generates sequential Interests (with increasing sequence numbers) towards the Aggregator (`Agg1`).
- The Aggregator (`Agg1`) acts both as a consumer (for its child nodes) and as a producer (for its parent node), thus requiring careful dual implementation in `CFNAggregatorApp`.
- Producers (`Leaf1`, `Leaf2`) respond with simple Data packets with numeric payloads to simulate sensor data.
- Ensure that the implementation closely follows the style and structure provided in official ndnSIM application examples.
- Utilize the `CFNAggConfig` class to manage simulation parameters, promoting modularity and ease of configuration.