#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include "apps/cfnagg/cfn-producer-app.hpp"
#include "apps/cfnagg/cfn-aggregator-app.hpp"
#include "apps/cfnagg/cfn-root-app.hpp"
#include "apps/cfnagg/trace-collector.hpp"

#include <sstream>

// Define log component for this file
NS_LOG_COMPONENT_DEFINE("CFNAggregationSim");

using namespace ns3;

inline void
PrintFib(Ptr<Node> node, std::ostream &os)
{
  // Get L3Protocol from node
  Ptr<ns3::ndn::L3Protocol> l3 = node->GetObject<ns3::ndn::L3Protocol>();
  if (!l3) {
    os << "Node " << Names::FindName(node) << " has no L3Protocol installed." << std::endl;
    return;
  }
  // Get forwarder from L3Protocol
  auto forwarder = l3->getForwarder();
  if (!forwarder) {
    os << "Node " << Names::FindName(node) << " has no forwarder." << std::endl;
    return;
  }
  // Access the FIB
  nfd::fib::Fib &fib = forwarder->getFib();

  // Iterate to output FIB entries
  os << "FIB entries for node " << Names::FindName(node) << ":" << std::endl;
  for (auto it = fib.begin(); it != fib.end(); ++it) {
    const auto &entry = *it;
    os << entry.getPrefix().toUri() << " -> ";
    const auto &nhs = entry.getNextHops();
    for (const auto &nh : nhs) {
      os << "[face " << nh.getFace().getId() 
         << ", cost " << nh.getCost() << "] ";
    }
    os << std::endl;
  }
}

int main(int argc, char* argv[]) {
  std::string topologyFile = "topologies/dcn.txt";
  std::string aggTreeFile = "topologies/aggtree-dcn.txt";
  std::string ccAlgorithm = "AIMD";
  double simTime = 20.0;
  std::string logFile = "cfnagg-trace.csv";

  CommandLine cmd;
  cmd.AddValue("topology", "Path to the topology file (dcn.txt)", topologyFile);
  cmd.AddValue("aggTree", "Path to the aggregation tree file (aggtree-dcn.txt)", aggTreeFile);
  cmd.AddValue("cc", "Congestion control algorithm (AIMD, CUBIC, BBR)", ccAlgorithm);
  cmd.AddValue("simTime", "Simulation duration (seconds)", simTime);
  cmd.AddValue("logFile", "Output log file name", logFile);
  cmd.Parse(argc, argv);

  // Read the network topology
  AnnotatedTopologyReader topoReader("", 25);
  topoReader.SetFileName(topologyFile);
  topoReader.Read();

  // Debug node names
  std::cout << "Debugging node names:" << std::endl;
  NodeContainer testNodes = NodeContainer::GetGlobal();
  for (uint32_t i = 0; i < testNodes.GetN(); i++) {
    std::cout << "Node " << i << " name: '" << Names::FindName(testNodes.Get(i)) << "'" << std::endl;
  }

  // Enable detailed NFD logging for debugging
  NS_LOG_INFO("Enabling detailed NFD logging");
  ns3::LogComponentEnable("ndn-cxx.nfd.Forwarder", ns3::LOG_LEVEL_DEBUG);
  ns3::LogComponentEnable("ndn.L3Protocol", ns3::LOG_LEVEL_DEBUG);
  ns3::LogComponentEnable("ndn-cxx.nfd.FaceTable", ns3::LOG_LEVEL_DEBUG);
  ns3::LogComponentEnable("ndn-cxx.nfd.FaceManager", ns3::LOG_LEVEL_DEBUG);
  
  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(10000);
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.InstallAll();

  // Set forwarding strategy: best-route for all names
  ns3::ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Install global routing interface on all nodes
  ns3::ndn::GlobalRoutingHelper globalRouting;
  globalRouting.InstallAll();

  // Parse the aggregation tree configuration file
  std::map<std::string, std::vector<std::string>> childrenMap;
  std::ifstream aggFile(aggTreeFile);
  if (!aggFile.is_open()) {
    std::cerr << "ERROR: Cannot open aggregation tree file: " << aggTreeFile << std::endl;
    return 1;
  }
  std::string line;
  while (std::getline(aggFile, line)) {
    if (line.size() == 0 || line[0] == '#') continue;
    std::stringstream ss(line);
    std::string parent;
    ss >> parent;
    std::vector<std::string> childList;
    std::string child;
    while (ss >> child) {
      childList.push_back(child);
    }
    childrenMap[parent] = childList;
  }
  aggFile.close();

  // Determine the root of the aggregation tree (node that is never a child)
  std::set<std::string> allChildren;
  for (auto& kv : childrenMap) {
    for (auto& child : kv.second ) {
      allChildren.insert(child);
    }
  }
  std::string rootName;
  for (auto& kv : childrenMap) {
    const std::string& node = kv.first;
    if (allChildren.find(node) == allChildren.end()) {
      rootName = node;
      break;
    }
  }
  if (rootName.empty()) {
    std::cerr << "ERROR: Root node not identified in aggregation tree\n";
    return 1;
  }

  // Identify all nodes in the aggregation tree (union of all parents and children)
  std::set<std::string> allNodes;
  for (auto& kv : childrenMap) {
    allNodes.insert(kv.first);
    for (auto& child : kv.second) {
      allNodes.insert(child);
    }
  }

  // Identify leaf nodes (nodes without children)
  std::vector<std::string> leaves;
  for (const std::string& node : allNodes) {
    if (childrenMap.find(node) == childrenMap.end()) {
      leaves.push_back(node);
    }
  }

  // Install root application on the root node
  Ptr<Node> rootNode = Names::Find<Node>(rootName);
  ns3::ndn::AppHelper rootHelper("ns3::CFNRootApp");
  rootHelper.SetPrefix("/" + rootName);
  rootHelper.SetAttribute("CongestionControl", StringValue(ccAlgorithm));
  rootHelper.Install(rootNode);
  Ptr<CFNRootApp> rootApp = DynamicCast<CFNRootApp>(rootNode->GetApplication(0));

  // Install aggregator applications on intermediate nodes (all parents except root)
  std::map<std::string, Ptr<CFNAggregatorApp>> aggAppMap;
  for (auto& kv : childrenMap) {
    const std::string& parent = kv.first;
    if (parent == rootName) continue;
    Ptr<Node> parentNode = Names::Find<Node>(parent);
    
    // Create the prefix for this node
    std::string nodePrefix = "/" + parent;
    
    // Print debug information
    std::cout << "DEBUG: Creating aggregator app for node [" << parent 
              << "] with prefix [" << nodePrefix << "]" << std::endl;
    
    ns3::ndn::AppHelper aggHelper("ns3::CFNAggregatorApp");
    aggHelper.SetPrefix(nodePrefix);
    aggHelper.Install(parentNode);
    Ptr<CFNAggregatorApp> aggApp = DynamicCast<CFNAggregatorApp>(parentNode->GetApplication(0));
    aggAppMap[parent] = aggApp;
  }

  // Install producer applications on all leaf nodes
  for (const std::string& leaf : leaves) {
    Ptr<Node> leafNode = Names::Find<Node>(leaf);
    ns3::ndn::AppHelper producerHelper("ns3::CFNProducerApp");
    producerHelper.SetPrefix("/" + leaf);
    producerHelper.Install(leafNode);
    // Optional: set producer payload attributes here if desired
  }

  // Configure each aggregator and the root with their children list
  for (auto& kv : childrenMap) {
    const std::string& parent = kv.first;
    const std::vector<std::string>& childList = kv.second;
    if (parent == rootName) {
      if (rootApp) {
        std::cout << "DEBUG: Setting children for Root [" << parent << "]: ";
        for (const auto& child : childList) {
          std::cout << child << " ";
        }                           
        std::cout << std::endl;
        rootApp->SetChildren(childList);
        rootApp->SetCongestionControl(ccAlgorithm);
      }
    } else {
      if (aggAppMap.find(parent) != aggAppMap.end()) {
        std::cout << "DEBUG: Setting children for Aggregator [" << parent << "]: ";
        for (const auto& child : childList) {
          std::cout << child << " ";
        }
        std::cout << std::endl;
        aggAppMap[parent]->SetChildren(childList);
      }
    }
  }

  // ***** Revised Section: Only add global origin prefixes for producers (leaf nodes) *****
  for (const std::string& nodeName : leaves) {
    Ptr<Node> node = Names::Find<Node>(nodeName);
    if (node != nullptr) {
      globalRouting.AddOrigins("/" + nodeName, node);
    }
  }

  // Add aggregator prefixes (every parent except the root)
  for (auto& kv : childrenMap) {
    const std::string& parent = kv.first;
    if (parent != rootName) {
      Ptr<Node> parentNode = Names::Find<Node>(parent);
      if (parentNode != nullptr) {
        globalRouting.AddOrigins("/" + parent, parentNode);
      }
    }
  }

  // Optionally, also add the root node’s prefix if you need the root 
  // to originate data under /rootName
  // Ptr<Node> rootNode = Names::Find<Node>(rootName);
  // globalRouting.AddOrigins("/" + rootName, rootNode);

  // ***************************************************************************

  // Calculate and install FIB routes
  globalRouting.CalculateRoutes();

  // After adding routes, print FIB entries for debugging
  Simulator::Schedule(MilliSeconds(100), [rootNode]() {
    std::cout << "DEBUG: Printing FIB entries on Root after delay:" << std::endl;
    PrintFib(rootNode, std::cout);
  });

  Ptr<Node> agg1Node = Names::Find<Node>("Agg1");
  Simulator::Schedule(MilliSeconds(100), [agg1Node]() {
    std::cout << "DEBUG: Printing FIB entries on Agg1 after delay:" << std::endl;
    PrintFib(agg1Node, std::cout);
  });

  // Install tracing helpers
  ns3::ndn::L3RateTracer::InstallAll("packet-trace.txt", Seconds(0.5));
  ns3::ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");

  // Open the trace log file
  TraceCollector::Open(logFile);

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  // Close the trace log
  TraceCollector::Close();

  return 0;
}
