#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include "apps/cfnagg-lite/CFNProducerApp.hpp"
#include "apps/cfnagg-lite/CFNAggregatorApp.hpp"
#include "apps/cfnagg-lite/CFNRootApp.hpp"
#include "apps/cfnagg-lite/CFNAggConfig.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream> // For std::cout

// Include necessary headers for FIB access
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/names.h" // For Names::FindName
#include "ns3/log.h"          // make sure NS_LOG_INFO is visible

#include "ns3/ndnSIM/helper/ndn-strategy-choice-helper.hpp"

using namespace ns3;
using namespace ns3::ndn;

NS_LOG_COMPONENT_DEFINE("cfnagg-simulation");

// Helper function to print FIB entries for a node
void PrintFib(Ptr<Node> node, std::ostream& os = std::cout) {
  if (!node) {
      os << "PrintFib: Error - Node is null." << std::endl;
      return;
  }
  std::string nodeName = Names::FindName(node);
  os << "--- FIB entries for node: " << (nodeName.empty() ? std::to_string(node->GetId()) : nodeName) << " ---" << std::endl;

  try {
      auto l3 = node->GetObject<ns3::ndn::L3Protocol>();
      if (!l3) {
          os << "  Error: Could not get L3Protocol object." << std::endl;
          return;
      }
      auto forwarder = l3->getForwarder();
      if (!forwarder) {
          os << "  Error: Could not get Forwarder object." << std::endl;
          return;
      }
      const ::nfd::fib::Fib& fib = forwarder->getFib();

      if (fib.begin() == fib.end()) {
          os << "  (FIB is empty)" << std::endl;
      } else {
          for (auto it = fib.begin(); it != fib.end(); ++it) {
              const auto& entry = *it;
              os << "  Prefix: " << entry.getPrefix().toUri() << " -> ";
              const auto& nhs = entry.getNextHops();
              if (nhs.empty()) {
                  os << "(No next hops)";
              } else {
                  for (const auto& nh : nhs) {
                      os << "[faceId=" << nh.getFace().getId()
                         << ", cost=" << nh.getCost() << "] ";
                  }
              }
              os << std::endl;
          }
      }
  } catch (const std::exception& e) {
      os << "  Error accessing FIB: " << e.what() << std::endl;
  } catch (...) {
      os << "  Unknown error accessing FIB." << std::endl;
  }
   os << "--------------------------------------" << std::endl;
}

// Wrapper function for scheduling PrintFib with std::cout
void SchedulePrintFib(Ptr<Node> node) {
  PrintFib(node, std::cout); // Explicitly pass std::cout
}

// 1. add a helper at file scope (above main):
static void
CalculateRoutesWithLog()
{
  NS_LOG_INFO("Calculating global routes...");
  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();
  NS_LOG_INFO("Global routes calculated.");
}

int
main(int argc, char* argv[])
{
  // Replace your current logging line with:
  LogComponentEnableAll(LOG_PREFIX_ALL);
  LogComponentEnableAll(LOG_PREFIX_NODE);

  // Enable logging for our custom apps.
  LogComponentEnable("ndn.CFNProducerApp", LOG_LEVEL_INFO);
  LogComponentEnable("ndn.CFNAggregatorApp", LOG_LEVEL_INFO);
  LogComponentEnable("ndn.CFNRootApp", LOG_LEVEL_INFO);
  LogComponentEnable("cfnagg-simulation", LOG_LEVEL_INFO);
  // near the top of your main()
  LogComponentEnable("ndn.L3Protocol", LOG_LEVEL_DEBUG);
  LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_INFO);
  LogComponentEnable("ndn.GlobalRoutingHelper", LOG_LEVEL_INFO);
  LogComponentEnable("ndn.FibHelper", LOG_LEVEL_INFO);

  LogComponentEnable("ndn-cxx.nfd.Forwarder", LOG_LEVEL_INFO);
  LogComponentEnable("ndn-cxx.nfd.BestRouteStrategy", LOG_LEVEL_INFO);
  // or even debug level
  // LogComponentEnable("ndn.L3Protocol", LOG_LEVEL_DEBUG);

  // Parse command-line arguments.
  CommandLine cmd;
  std::string configFile = "cfnagg-config.conf";
  // Add a new parameter for base directory with a default value
  std::string baseDir = "src/ndnSIM/examples/cfnagg-lite/";

  cmd.AddValue("configFile", "CFNAgg Configuration file", configFile);
  cmd.AddValue("baseDir", "Base directory for simulation files", baseDir);
  cmd.Parse(argc, argv);

  // If baseDir doesn't end with a slash, add one
  if (!baseDir.empty() && baseDir[baseDir.length() - 1] != '/') {
    baseDir += '/';
  }

  // Construct full paths
  std::string fullConfigPath = baseDir + configFile;
  std::string topologyPath = baseDir + "topologies/dcn.txt";
  std::string treeFilePath = baseDir + "topologies/aggtree-dcn.txt";

  NS_LOG_INFO("Using base directory: " << baseDir);
  NS_LOG_INFO("Config file path: " << fullConfigPath);
  NS_LOG_INFO("Topology file path: " << topologyPath);
  NS_LOG_INFO("Tree file path: " << treeFilePath);

  // Load simulation parameters using CFNAggConfig.
  CFNAggConfig config;
  if (!config.LoadFromFile(fullConfigPath)) {
    NS_LOG_ERROR("cfnagg-simulation: Could not load configuration file " << fullConfigPath);
    return 1;
  }

  // Read the network topology from file.
  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName(topologyPath);
  topologyReader.Read();

  // Right after topologyReader.Read();
  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); ++node) {
    std::cout << "Node ID=" << (*node)->GetId() << ", Name=" << Names::FindName(*node) << std::endl;
    std::cout << "  # of NetDevices: " << (*node)->GetNDevices() << std::endl;
    for (uint32_t d = 0; d < (*node)->GetNDevices(); d++) {
      std::cout << "    Device[" << d << "]: " << (*node)->GetDevice(d)->GetInstanceTypeId()
                << std::endl;
    }
  }

  // Install ndnSIM on all nodes.
  StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // 2. **Insert the explicit best‐route binding here:**
  StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Install global routing.
  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Parse the aggregation tree from file.
  std::ifstream treeFile(treeFilePath);
  if (!treeFile.is_open()) {
    NS_LOG_ERROR("cfnagg-simulation: Could not open aggregation tree file " << treeFilePath);
    return 1;
  }
  std::map<std::string, std::vector<std::string>> aggregationTree;
  std::string line;
  while (std::getline(treeFile, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream iss(line);
    std::string parent;
    iss >> parent;
    std::string child;
    while (iss >> child) {
      aggregationTree[parent].push_back(child);
    }
  }
  treeFile.close();

  // Retrieve nodes by name.
  Ptr<Node> root = Names::Find<Node>("Root");
  Ptr<Node> agg1 = Names::Find<Node>("Agg1");
  Ptr<Node> leaf1 = Names::Find<Node>("Leaf1");
  Ptr<Node> leaf2 = Names::Find<Node>("Leaf2");

  // --- Print Root FIB before app installation and routing calculation ---
  NS_LOG_INFO("Printing Root FIB before application installation and route calculation:");
  PrintFib(root);
  // --- Print Agg1 FIB before app installation and routing calculation ---
  NS_LOG_INFO("Printing Agg1 FIB before application installation and route calculation:");
  PrintFib(agg1);

  // --- Use AppHelper with specific methods where possible ---

  // Install applications on the leaf nodes.
  AppHelper producerHelper("ns3::ndn::CFNProducerApp");

  // --- Producer 1 (Leaf1) ---
  std::string leaf1Name = Names::FindName(leaf1); // Get node name
  std::string leaf1Prefix = "/agg/" + leaf1Name;  // Construct prefix
  producerHelper.SetPrefix(leaf1Prefix);         // Use dynamic prefix
  ApplicationContainer producerApps1 = producerHelper.Install(leaf1);
  Ptr<CFNProducerApp> producer1 = producerApps1.Get(0)->GetObject<CFNProducerApp>();
  producer1->SetPayloadValue(config.GetProducerPayload(leaf1Name)); // Use name for payload lookup
  producerApps1.Start(Seconds(0.1));
  producerApps1.Stop(Seconds(config.GetStopTime()));
  ndnGlobalRoutingHelper.AddOrigins(leaf1Prefix, leaf1); // Register dynamic prefix

  // --- Producer 2 (Leaf2) ---
  std::string leaf2Name = Names::FindName(leaf2); // Get node name
  std::string leaf2Prefix = "/agg/" + leaf2Name;  // Construct prefix
  producerHelper.SetPrefix(leaf2Prefix);         // Use dynamic prefix
  ApplicationContainer producerApps2 = producerHelper.Install(leaf2);
  Ptr<CFNProducerApp> producer2 = producerApps2.Get(0)->GetObject<CFNProducerApp>();
  producer2->SetPayloadValue(config.GetProducerPayload(leaf2Name)); // Use name for payload lookup
  producerApps2.Start(Seconds(0.1));
  producerApps2.Stop(Seconds(config.GetStopTime()));
  ndnGlobalRoutingHelper.AddOrigins(leaf2Prefix, leaf2); // Register dynamic prefix

  // Install the aggregator application on Agg1.
  AppHelper aggregatorHelper("ns3::ndn::CFNAggregatorApp");
  // aggregatorHelper.SetPrefix("/agg/Agg1"); // Aggregator serves the parent prefix /agg
  aggregatorHelper.SetAttribute("Prefix", StringValue("/agg/Agg1"));
  aggregatorHelper.SetAttribute("PartialTimeout", TimeValue(MilliSeconds(config.GetPartialTimeout())));
  ApplicationContainer aggregatorApps = aggregatorHelper.Install(agg1);
  Ptr<CFNAggregatorApp> aggregator = aggregatorApps.Get(0)->GetObject<CFNAggregatorApp>();

  // Aggregator needs to know the prefixes of its children
  aggregator->AddChildPrefix(Name(leaf1Prefix));
  aggregator->AddChildPrefix(Name(leaf2Prefix));
  aggregatorApps.Start(Seconds(0.0));
  aggregatorApps.Stop(Seconds(config.GetStopTime()));

  // OLD – causes the duplicate nexthop
  ndnGlobalRoutingHelper.AddOrigins("/agg/Agg1", agg1);
  // NEW – announce one level above the application prefix
  // ndnGlobalRoutingHelper.AddOrigins("/agg", agg1);

  // Install the Root application on Root.
  AppHelper rootHelper("ns3::ndn::CFNRootApp");
  rootHelper.SetPrefix("/agg/Agg1"); // Root requests the parent prefix /agg
  rootHelper.SetAttribute("StartSeq", IntegerValue(0));
  rootHelper.SetAttribute("Rounds", IntegerValue(config.GetRounds()));
  ApplicationContainer rootApps = rootHelper.Install(root);
  rootApps.Start(Seconds(0.2));
  rootApps.Stop(Seconds(config.GetStopTime()));

  // --- End AppHelper usage ---

  ns3::ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(0.5));

  // // --- Calculate and install the FIB entries ---
  // NS_LOG_INFO("Calculating global routes...");
  // GlobalRoutingHelper::CalculateRoutes();
  // NS_LOG_INFO("Global routes calculated.");

  // 2. schedule it instead of the lambda
  Simulator::Schedule(Seconds(0.01),
  &CalculateRoutesWithLog);

  // --- Schedule FIB printing AFTER applications start ---
  // Schedule slightly after Root app starts (0.2s) to ensure routes are set
  // Use the wrapper function SchedulePrintFib
  Simulator::Schedule(Seconds(0.3), &SchedulePrintFib, root);
  Simulator::Schedule(Seconds(0.001), &SchedulePrintFib, agg1);
  Simulator::Schedule(Seconds(0.3), &SchedulePrintFib, agg1);
  Simulator::Schedule(Seconds(1.0), &SchedulePrintFib, agg1);

  // --- Simulation Run ---
  NS_LOG_INFO("Starting simulation...");
  Simulator::Stop(Seconds(config.GetStopTime() + 1));
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Simulation finished.");
  return 0;
}