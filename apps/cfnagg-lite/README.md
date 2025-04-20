# NDN Aggregation Framework Class Summary

## About this Project
This project is built on ndnSIM-2.9, the NDN simulation framework for NS-3. The official GitHub repository for ndnSIM can be found at [https://github.com/named-data-ndnSIM/ndnSIM](https://github.com/named-data-ndnSIM/ndnSIM).

### Source Files
The following files are from the official ndnSIM-2.9 implementation:
- `ndn-app.cpp` and `ndn-app.hpp`
- `ndn-consumer.cpp` and `ndn-consumer.hpp`
- `ndn-producer.cpp` and `ndn-producer.hpp`

All other files in this directory are custom implementations created specifically for this aggregation framework.

## Class Inheritance Hierarchy

- **App** (Base class for all NDN applications)
  - **Consumer** (Inherits from App)
    - **CFNRootApp** (Inherits from Consumer)
  - **Producer** (Inherits from App)
    - **CFNProducerApp** (Inherits from Producer)
  - **CFNAggregatorApp** (Inherits from App)

## Class Relationships

The classes form a hierarchical aggregation tree:

1. **CFNRootApp** sits at the top of the tree (root node)
   - Sends Interests to Aggregators
   - Receives final aggregated results

2. **CFNAggregatorApp** serves as middle nodes
   - Receives Interests from parent nodes
   - Forwards Interests to child nodes
   - Aggregates Data from children
   - Returns aggregated results to parent
   - **Note:** The aggregator is the most sophisticated component in this framework as it plays a dual role - acting as a consumer for its children (sending Interests and collecting responses) and as a producer for its parent (processing Interests and returning aggregated Data).

3. **CFNProducerApp** forms the leaf nodes
   - Generates actual data values (numeric payloads)
   - Responds to Interests with Data packets containing these values

## Core Functions by Class

### App (Base Class)
- Provides core NDN application functionality
- Registers with NDN stack via face creation
- Handles application lifecycle (start/stop)
- Defines basic event handlers for Interests, Data, and Nacks

### Consumer (Intermediate Class)
- Specializes in sending Interest packets
- Manages sequence numbers and retransmissions
- Tracks metrics (RTT, timeouts, etc.)
- Implements interest scheduling and timeout handling

### Producer (Intermediate Class)
- Specializes in responding to Interest packets with Data
- Registers prefix in FIB for Interest routing
- Creates and signs Data packets
- Manages data freshness and payload size

### CFNRootApp
- **StartApplication()**: Initializes application and schedules first Interest
- **SendPacket()**: Creates and sends Interest packets with sequence numbers
- **ScheduleNextPacket()**: Schedules subsequent Interest packets for future rounds
- **OnData()**: Processes aggregated responses from aggregator nodes

### CFNAggregatorApp
- **StartApplication()**: Sets up aggregator with its prefix
- **OnInterest()**: Processes incoming Interest from parent, creates aggregation buffer
- **ForwardInterestToChildren()**: Forwards Interests to all child nodes
- **OnData()**: Processes Data from children, adds to aggregation buffer
- **AggregationTimeout()**: Handles partial aggregation if not all children respond in time
- **AddChildPrefix()**: Registers a child node prefix for forwarding

### CFNProducerApp
- **StartApplication()**: Inherits from Producer to register prefix
- **OnInterest()**: Generates Data with numeric payload value
- **SetPayloadValue()**: Sets the numeric value to be returned in Data packets

The framework implements a compute aggregation tree where numerical values from leaf nodes (CFNProducerApp) are summed up at each aggregator level (CFNAggregatorApp), eventually returning a final aggregated sum to the root node (CFNRootApp).