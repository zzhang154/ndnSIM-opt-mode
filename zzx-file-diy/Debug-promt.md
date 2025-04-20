# Fixing NDN Interest Dispatching in CFNAgg Hierarchical Aggregation Framework

## Project Overview
I'm working with a hierarchical data aggregation system called CFNAgg, implemented in ndnSIM 2.9 (NDN Forwarding Daemon 22.02). The system allows a tree of NDN nodes to aggregate data from leaves to a root, with interests flowing down the tree and data flowing back up with aggregation at each level.

## Reference Documentation
- ndnSIM-2.9 GitHub Repository: https://github.com/named-data-ndnSIM/ndnSIM
- ndnSIM Documentation: https://ndnsim.net/current/

## System Components
CFNAgg consists of:
- **CFNRootApp**: Initiates data collection by sending Interests down the tree
- **CFNAggregatorApp**: Intermediate nodes that forward interests to children and aggregate responses
- **StragglerManager**: Handles timeout management for unresponsive children
- **cfnagg-simulation.cpp**: Main simulation file that sets up the network topology, configures the aggregation tree, installs applications, and establishes routing between nodes
- **aggtree-dcn.txt** and **dcn.txt**: topology and link file.

## Current Issue
The CFNAggregator application nodes **are not receiving** interests sent by the root node. The network-level forwarding is working properly (I can see `onIncomingInterest` and `onOutgoingInterest` in logs), but the actual application's `OnInterest` handler is never called.

Looking at line 145 in `cfn-root-app.cpp`, there's a method `onReceiveInterest` which seems to be used for dispatching interests through the application link. This suggests there might be an issue with how interest handlers are registered or matched in the application.

## Attempted Solutions That Failed

### 1. Application-level Registration Attempts (in `CFNAggregatorApp::StartApplication`)
- **Using `registerPrefix` on `m_face`**: Failed due to type mismatch - in ndnSIM Apps, `m_face` is of type `nfd::face::Face`, not the application-level `ndn::Face`.
- **Using `addInterestFilter` on `m_appLink`**: Failed because the `m_appLink` object doesn't have an `addInterestFilter` method in ndnSIM-2.9.
- **Using `SetInterestFilter` method**: Failed because this method doesn't exist in ndnSIM-2.9 App class.
- **Registering explicit routes for specific sequences**: Failed because adding FIB entries only helps with forwarding, not application dispatch.

### 2. Interest Sending Approaches (in `CFNRootApp::SendInterest`)
- **Using `m_face->sendInterest()` instead of `m_appLink->onReceiveInterest()`**: While the correct method for sending interests, logs show interests are properly forwarded by NFD but not dispatched to the OnInterest callback.
- **Using only `m_appLink->onReceiveInterest()`**: Tried with the same results.

### 3. Simulation Setup Changes (in `cfnagg-simulation.cpp` main function)
- **Changing from `SetAttribute("Prefix", StringValue(...))` to `SetPrefix(...)`**: This correctly sets the prefix but doesn't solve the dispatch issue.
- **Switching forwarding strategies (e.g., multicast)**: Not relevant as the issue is with application interest dispatching, not forwarding decisions.

## Important Code Sections to Understand
In `CFNRootApp::SendInterest`, interests are sent using:
```cpp
m_appLink->onReceiveInterest(*interest);
```
Where m_appLink is of type ns3::ndn::AppLinkService* (from ns3::ndn::App::m_appLink)

In ndn-app-link-service.cpp:
```cpp
void
AppLinkService::onReceiveInterest(const Interest& interest)
{
  this->receiveInterest(interest, 0);
}
```
And in link-service.cpp:
```cpp
void
LinkService::receiveInterest(const Interest& interest, const EndpointId& endpoint)
{
  NFD_LOG_FACE_TRACE(__func__);

  ++this->nInInterests;

  afterReceiveInterest(interest, endpoint);
}
```

## Current Log Output
Debugging node names:
Node 0 name: 'Root'
Node 1 name: 'Agg1'
Node 2 name: 'Leaf1'
Node 3 name: 'Leaf2'
Aggregator Agg1: added FIB route for exact prefix /Agg1 via local face 260
Aggregator Agg1: CRITICAL - Explicitly added interest filters for /Agg1 and /Agg1/%2A
CFNAggregatorApp started on node Agg1 [prefix=/Agg1, children=0]
CRITICAL: Registered dispatcher for Agg1 with prefixes /Agg1 and /Agg1/*
DEBUG: Setting children for Aggregator [Agg1]: Leaf1 Leaf2
DEBUG: Setting children for Root [Root]: Agg1
Adding explicit route: Agg1 -> /Leaf1
Adding explicit route: Leaf1 -> /Agg1
Adding explicit route: Agg1 -> /Leaf2
Adding explicit route: Leaf2 -> /Agg1
Adding explicit route: Root -> /Agg1
Adding explicit route: Agg1 -> /Root
CFNProducerApp started on node 2 [prefix=/Leaf1]
CFNProducerApp started on node 3 [prefix=/Leaf2]
DEBUG: Printing FIB entries on Root after delay:
FIB entries for node Root:
/ -> [face 257, cost 2147483647]
/localhost/nfd/rib -> [face 256, cost 0]
/Leaf2 -> [face 257, cost 2]
/Root -> [face 258, cost 0]
/Leaf1 -> [face 257, cost 2]
/Agg1 -> [face 257, cost 1]
/localhost/nfd -> [face 1, cost 0]
DEBUG: Printing FIB entries on Agg1 after delay:
FIB entries for node Agg1:
/ -> [face 257, cost 2147483647] [face 258, cost 2147483647] [face 259, cost 2147483647]
/localhost/nfd/rib -> [face 256, cost 0]
/Leaf2 -> [face 259, cost 1]
/Root -> [face 257, cost 1]
/Leaf1 -> [face 258, cost 1]
/Agg1 -> [face 260, cost 0]
/localhost/nfd -> [face 1, cost 0]
/Agg1/%2A -> [face 260, cost 1]
Root sending Interest /Agg1/1 to child [Agg1]
onIncomingInterest in=(258,0) interest=/Agg1/1
onContentStoreMiss interest=/Agg1/1
onOutgoingInterest out=257 interest=/Agg1/1
Root straggler timeout for seq 1 (received 0/1)

## Key Question
- What mechanism connects NDN forwarding interests to application-level OnInterest handlers in ndnSIM 2.9?
- How does the AppLinkService::onReceiveInterest method get triggered for incoming interests?
- Is there a method in ndnSIM 2.9 to register interest filters at the application layer?
- Could the problem be in how the m_appLink is connecting to the forwarding plane?
- What's the proper way to make an application "listen" for interests in ndnSIM 2.9?
- Most important, how can I solve the current problem that Agg1 cannot receive any packet, and "CFNAggregatorApp::OnInterest" is never triggered.

## Request
Give me a step-by-step solution, tell me which code block in which function needs to be revised, compared to the old one.