# agg‑mini Application Overview

This directory contains the source for the “agg‑mini” example in ndnSIM.  The layout is:

```
agg-mini/ ├── aggregator/ # Intermediate aggregator node │ ├── AggregatorApp.hpp │ └── AggregatorApp.cpp ├── buffer/ # In‑flight sequence buffer and manager │ ├── AggBuffer.hpp │ ├── AggBuffer.cpp │ ├── AggBufferManager.hpp │ └── AggBufferManager.cpp ├── common/ # Shared utilities │ ├── CommonUtils.hpp │ └── CommonUtils.cpp ├── leaf/ # Leaf producer application │ ├── LeafApp.hpp │ └── LeafApp.cpp ├── root/ # Root consumer application │ ├── RootApp.hpp │ └── RootApp.cpp └── ndn-tree-example.md # Topology and usage notes
```

## Components

- **aggregator/**  
  Implements `ns3::ndn::AggregatorApp`.  On receiving an Interest from its parent, it  
  1. Allocates a per‑sequence buffer  
  2. Fans‑out Interests to each child prefix  
  3. Collects responses and aggregates (sum)  
  4. Sends a single Data back upstream, even on timeout  

- **buffer/**  
  Contains `AggBuffer` (holds partial replies for one sequence) and  
  `AggBufferManager` (tracks all in‑flight sequences, schedules timeouts).

- **common/**  
  Shared utilities (e.g., `SetDummySignature`) used by both root/leaf/aggregator to  
  satisfy ndn‑cxx’s Data wire‑encoding signature requirements.

- **leaf/**  
  Implements `ns3::ndn::LeafApp`: a simple consumer of Interests under `/app/LeafX`,  
  replying with a one‑int payload.

- **root/**  
  Implements `ns3::ndn::RootApp`: expresses a fixed number of Interests under `/app/agg/seq=N`  
  and prints received aggregates.

- **ndn-tree-example.md**  
  AnnotatedTopologyReader input and step‑by‑step usage instructions for the example.  

---