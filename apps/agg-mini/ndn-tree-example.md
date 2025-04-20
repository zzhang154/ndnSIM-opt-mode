# ndn-tree-tracers Example Scenario

**Demo Files Included:**
- `src/ndnSIM/apps/ndn-consumer.cpp`
- `src/ndnSIM/apps/ndn-consumer.hpp`
- `src/ndnSIM/apps/ndn-consumer-cbr.cpp`
- `src/ndnSIM/apps/ndn-consumer-cbr.hpp`
- `src/ndnSIM/apps/ndn-producer.cpp`
- `src/ndnSIM/apps/ndn-producer.hpp`
- `src/ndnSIM/examples/ndn-tree-example.cpp`
- `src/ndnSIM/examples/ndn-tree-tracers.cpp`
- `src/ndnSIM/examples/topologies/topo-tree.txt`

## Overview

This scenario simulates an NDN network with a tree topology. Four "leaf" nodes act as consumers, requesting data from a single "root" node acting as a producer. The simulation utilizes the ndnSIM module within ns-3 to model NDN communication, routing, and application behavior. It also demonstrates the use of rate tracing.

## Topology

The network topology is defined in `src/ndnSIM/examples/topologies/topo-tree.txt` and loaded using `ns3::AnnotatedTopologyReader`. It represents a simple tree:

```
leaf-1 leaf-2 leaf-3 leaf-4 (Consumers) \ / \ / +--------+ +--------+ (Links: 10Mbps / 1ms) | rtr-1 | | rtr-2 | (Routers) +--------+ +--------+ \ / +--------------------+ (Link: 10Mbps / 1ms) | root | (Producer) +--------------------+
```

*   **Nodes:** `leaf-1`, `leaf-2`, `leaf-3`, `leaf-4`, `rtr-1`, `rtr-2`, `root`.
*   **Links:** Point-to-point links with specified bandwidth and delay connect the nodes as shown.

## Components and Relationships

1.  **Core ns-3 & ndnSIM Modules:**
    *   `ns3/core-module.h`, `ns3/network-module.h`: Basic ns-3 simulation and networking components.
    *   `ns3/ndnSIM-module.h`: Includes necessary ndnSIM headers.

2.  **Applications:**
    *   **`ns3::ndn::ConsumerCbr` (`ndn-consumer-cbr.hpp`, `ndn-consumer-cbr.cpp`):**
        *   Installed on `leaf-1` through `leaf-4`.
        *   **Inheritance:** `ConsumerCbr` -> `ns3::ndn::Consumer` (`ndn-consumer.hpp`, `ndn-consumer.cpp`) -> `ns3::ndn::App` (`ndn-app.hpp`).
        *   **Functionality:** Sends Interest packets at a specified frequency (`Frequency` attribute, set to 100 Interests/sec). It inherits basic Interest sending, retransmission, and Data/Nack handling logic from `Consumer`. The `Cbr` variant specifically schedules packet sending based on the frequency.
        *   **Configuration:** Each consumer is configured with a unique prefix: `/root/<leaf-name>` (e.g., `/root/leaf-1`). It appends sequence numbers to this prefix for individual Interests (e.g., `/root/leaf-1/0`, `/root/leaf-1/1`, ...).
    *   **`ns3::ndn::Producer` (`ndn-producer.hpp`, `ndn-producer.cpp`):**
        *   Installed on the `root` node.
        *   **Inheritance:** `Producer` -> `ns3::ndn::App`.
        *   **Functionality:** Listens for incoming Interest packets. When an Interest arrives whose name matches the Producer's configured prefix, it generates and sends back a corresponding Data packet.
        *   **Configuration:** Configured with the prefix `/root` and a payload size (`PayloadSize` attribute, set to 1024 bytes).

3.  **Helpers:**
    *   `ns3::AnnotatedTopologyReader`: Reads the topology definition from the specified file.
    *   `ns3::ndn::StackHelper`: Installs the NDN protocol stack (including CS, PIT, FIB) on all nodes.
    *   `ns3::ndn::StrategyChoiceHelper`: Sets the forwarding strategy for specified prefixes. Here, it sets the strategy for `/prefix` to `/localhost/nfd/strategy/best-route`. Note that `/prefix` seems like a general placeholder; the routing for `/root` relies on the global routing helper.
    *   `ns3::ndn::GlobalRoutingHelper`: Manages the computation and installation of routing information (FIB entries) across all nodes based on advertised prefix origins.
    *   `ns3::ndn::AppHelper`: A helper class to simplify the installation and configuration of NDN applications (`ConsumerCbr`, `Producer`) on nodes.

4.  **Tracer:**
    *   `ns3::ndn::L3RateTracer`: Installs trace sinks on all nodes to record packet rates (Interests/Data per second) at the network layer (L3). Output is written to `rate-trace.txt`.

## Prefix Installation and Routing

Setting up correct routing is crucial for Interests from consumers to reach the producer and for Data to return. This scenario uses the `GlobalRoutingHelper` for this purpose. Here's a step-by-step breakdown:

1.  **Install Global Routing:**
    *   `ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;`
    *   `ndnGlobalRoutingHelper.InstallAll();`
    *   This installs the necessary components on each node to participate in the global routing calculation and receive FIB updates.

2.  **Configure Producer Prefix:**
    *   `ndn::AppHelper producerHelper("ns3::ndn::Producer");`
    *   `producerHelper.SetPrefix("/root");`
    *   This configures the `Producer` application *instance* that will be installed on the `root` node. It tells the application which namespace it is responsible for serving data under (i.e., `/root`). When the producer application starts (`StartApplication` method in `ndn-producer.cpp`), it uses `FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);` to add a route for `/root` to its *local* FIB, pointing towards the application itself (via `m_face`). This ensures the producer node knows to deliver Interests for `/root` to this application.

3.  **Advertise Producer Prefix Origin:**
    *   `ndnGlobalRoutingHelper.AddOrigins("/root", producer);`
    *   This is a key step for global routing. It informs the `GlobalRoutingHelper` that the node where the `producer` application is installed (the `root` node) is the authoritative source (origin) for the `/root` prefix. This information will be used to calculate routes *towards* the `root` node for this prefix from all other nodes.

4.  **Configure Consumer Prefixes:**
    *   `ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");`
    *   `consumerHelper.SetPrefix("/root/" + Names::FindName(consumers[i]));`
    *   This configures each `ConsumerCbr` application *instance* with the base name it should use when generating Interests (e.g., `/root/leaf-1`). The consumer appends sequence numbers to this prefix. This `SetPrefix` call **only** configures the application's behavior; it does not directly install FIB entries or advertise origins.

5.  **Calculate and Install Routes (FIB Population):**
    *   `ndn::GlobalRoutingHelper::CalculateRoutes();`
    *   This triggers the central routing calculation. The `GlobalRoutingHelper` uses:
        *   The network topology (nodes and links) provided by the `AnnotatedTopologyReader`.
        *   The prefix origin information provided by `AddOrigins` (i.e., `/root` originates at the `root` node).
    *   Based on this, it computes the shortest paths (according to the default metric, usually hop count) from all nodes to the registered origins.
    *   It then installs the resulting FIB entries on *all* nodes in the simulation. For example:
        *   On `leaf-1`, a FIB entry for `/root` will be created pointing towards `rtr-1`.
        *   On `rtr-1`, a FIB entry for `/root` will be created pointing towards `root`.
        *   On `rtr-2`, a FIB entry for `/root` will be created pointing towards `root`.
        *   And so on.

**In Summary:** `SetPrefix` on the producer tells the *application* what names it handles and registers it locally. `AddOrigins` tells the *global routing system* where that prefix physically exists. `SetPrefix` on the consumer tells the *application* what names to request. `CalculateRoutes` uses the origin information and topology to populate FIBs across the network, enabling Interests matching `/root` (sent by consumers) to be forwarded towards the `root` node.