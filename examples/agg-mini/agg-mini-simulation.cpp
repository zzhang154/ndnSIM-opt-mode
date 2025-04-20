/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Automated Root–Agg–Leaf topology demo for ndnSIM‑2.9 using AnnotatedTopologyReader and tree.txt
 * Replicates the logic of the simple 3-node example with prefixes /app/agg and /app/leaf.
 */

 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/ndnSIM-module.h"
 
 #include <fstream>
 #include <sstream>
 #include <map>
 #include <vector>
 #include <set>
 #include <string>
 #include <stdexcept> // For runtime_error
 
 using namespace ns3;
 
 // Define the logging component for this file
 NS_LOG_COMPONENT_DEFINE("agg-mini-simulation");
 
 // Structure to hold tree relationships
 struct TreeNodeInfo {
   std::string name;
   std::vector<std::string> children;
   std::string parent;
   bool isLeaf = false;
   bool isAggregator = false;
   bool isRoot = false;
 };
 
 int
 main(int argc, char* argv[])
 {
   CommandLine cmd;
   std::string baseDir = "src/ndnSIM/examples/agg-mini/";
   cmd.AddValue("baseDir", "Base directory for simulation files", baseDir);
   cmd.Parse(argc, argv);
 
   if (!baseDir.empty() && baseDir.back() != '/')
     baseDir += '/';
 
   std::string topoPath = baseDir + "topologies/agg-mini-topo.txt";
   std::string treePath = baseDir + "topologies/tree.txt";
 
   // --- 1) Load physical topology from file ---
   AnnotatedTopologyReader topologyReader("", 1.0);
   topologyReader.SetFileName(topoPath);
   topologyReader.Read();
   NS_LOG_INFO("Loaded topology from: " << topoPath);
 
   // --- 2) Parse tree.txt to build logical tree structure ---
   std::map<std::string, TreeNodeInfo> nodeInfoMap;
   std::set<std::string> allNodeNames, childNodeNames;
   std::ifstream treeFile(treePath);
   if (!treeFile.is_open()) {
     NS_LOG_ERROR("Could not open tree file: " << treePath);
     return 1;
   }
   NS_LOG_INFO("Parsing tree structure from: " << treePath);
   std::string line;
   while (std::getline(treeFile, line)) {
     if (line.empty() || line[0] == '#')
       continue;
     std::istringstream iss(line);
     std::string parentName, childName;
     if (!(iss >> parentName))
       continue;
     allNodeNames.insert(parentName);
     nodeInfoMap[parentName].name = parentName;
     while (iss >> childName) {
       nodeInfoMap[parentName].children.push_back(childName);
       nodeInfoMap[childName].name = childName;
       nodeInfoMap[childName].parent = parentName;
       allNodeNames.insert(childName);
       childNodeNames.insert(childName);
       NS_LOG_DEBUG("Parsed link: " << parentName << " -> " << childName);
     }
   }
   treeFile.close();
 
   // --- 3) Identify node roles (Root, Aggregator, Leaf) ---
   std::string rootName = "";
   std::vector<std::string> aggregatorNames;
   std::vector<std::string> leafNames;
   for (auto& pair : nodeInfoMap) {
     std::string name = pair.first;
     TreeNodeInfo& info = pair.second;
     if (childNodeNames.count(name) == 0 && !info.children.empty()) {
       info.isRoot = true;
       if (!rootName.empty()) {
         NS_LOG_WARN("Multiple roots found (" << rootName << ", " << name
                      << "). Using first one: " << rootName);
       }
       else {
         rootName = name;
         NS_LOG_INFO("Identified Root: " << rootName);
       }
     }
     else if (!info.parent.empty() && !info.children.empty()) {
       info.isAggregator = true;
       aggregatorNames.push_back(name);
       NS_LOG_INFO("Identified Aggregator: " << name);
     }
     else if (!info.parent.empty() && info.children.empty()) {
       info.isLeaf = true;
       leafNames.push_back(name);
       NS_LOG_INFO("Identified Leaf: " << name);
     }
     else if (info.parent.empty() && info.children.empty()
              && allNodeNames.size() == 1) {
       info.isRoot = true;
       rootName = name;
       NS_LOG_INFO("Identified single node as Root: " << rootName);
     }
   }
   if (rootName.empty() && !allNodeNames.empty()) {
     NS_LOG_ERROR("Could not identify a root node.");
     return 1;
   }
   if (aggregatorNames.size() > 1) {
     NS_LOG_WARN("Multiple aggregators found. This simple example logic "
                 "assumes one aggregator.");
   }
   if (aggregatorNames.empty() && !leafNames.empty()) {
     NS_LOG_ERROR("Leaves found but no aggregator identified.");
     return 1;
   }
   std::string aggName = aggregatorNames.empty() ? "" : aggregatorNames[0];
 
   // --- 4) Install NDN stack ---
   ns3::ndn::StackHelper ndnHelper;
   ndnHelper.InstallAll();
 
   // --- 6) Set up FIB routes based on tree structure ---
   const std::string aggPrefix = "/app/agg";
   if (!rootName.empty() && !aggName.empty()) {
     ns3::ndn::FibHelper::AddRoute(rootName, aggPrefix, aggName, 1);
     NS_LOG_INFO("FIB: Added route on " << rootName
                 << " for " << aggPrefix << " via " << aggName);
   }
   if (!aggName.empty()) {
     for (const std::string& leafName : leafNames) {
       std::string childPrefix = "/app/" + leafName;
       ns3::ndn::FibHelper::AddRoute(aggName, childPrefix, leafName, 1);
       NS_LOG_INFO("FIB: Added route on " << aggName
                   << " for " << childPrefix << " via " << leafName);
     }
   }
 
   // --- 5) Install Strategy ---
   ns3::ndn::StrategyChoiceHelper::InstallAll(
     "/", "/localhost/nfd/strategy/best-route");
 
   // --- 7) Install custom apps based on roles ---
   if (!rootName.empty()) {
     Ptr<Node> rootNode = Names::Find<Node>(rootName);
     if (!rootNode) {
       throw std::runtime_error("Root node '" + rootName + "' not found in topology");
     }
     ns3::ndn::AppHelper rootHelper("ns3::ndn::RootApp");
     rootHelper.SetPrefix(aggPrefix);
     rootHelper.SetAttribute("MaxSeq", UintegerValue(3));
     rootHelper.SetAttribute("Interval", DoubleValue(1.0));
     rootHelper.Install(rootNode).Start(Seconds(0.0));
     NS_LOG_INFO("Installed RootApp on " << rootName
                 << " sending for " << aggPrefix);
   }
   if (!aggName.empty()) {
     Ptr<Node> aggNode = Names::Find<Node>(aggName);
     for (const std::string& leafName : leafNames) {
       std::string childPrefix = "/app/" + leafName;
       ns3::ndn::AppHelper aggHelper("ns3::ndn::AggregatorApp");
       aggHelper.SetPrefix(aggPrefix);
       aggHelper.SetAttribute("ChildPrefix", StringValue(childPrefix));
       aggHelper.Install(aggNode).Start(Seconds(0.0));
       NS_LOG_INFO("Installed AggregatorApp on " << aggName
                   << " for " << aggPrefix << " → " << childPrefix);
     }
   }
   for (const std::string& leafName : leafNames) {
     Ptr<Node> leafNode = Names::Find<Node>(leafName);
     std::string childPrefix = "/app/" + leafName;
     ns3::ndn::AppHelper leafHelper("ns3::ndn::LeafApp");
     leafHelper.SetPrefix(childPrefix);
     leafHelper.Install(leafNode).Start(Seconds(0.0));
     NS_LOG_INFO("Installed LeafApp on " << leafName
                 << " listening for " << childPrefix);
   }
 
   // --- 8) Set up global routing origins ---
   ns3::ndn::GlobalRoutingHelper globalRouting;
   globalRouting.InstallAll();
   if (!aggName.empty()) {
     Ptr<Node> aggNode = Names::Find<Node>(aggName);
     globalRouting.AddOrigins(aggPrefix, aggNode);
     NS_LOG_INFO("GlobalRouting: Added origin " << aggPrefix
                 << " at " << aggName);
   }
   for (const std::string& leafName : leafNames) {
     Ptr<Node> leafNode = Names::Find<Node>(leafName);
     std::string childPrefix = "/app/" + leafName;
     globalRouting.AddOrigins(childPrefix, leafNode);
     NS_LOG_INFO("GlobalRouting: Added origin " << childPrefix
                 << " at " << leafName);
   }
   ns3::ndn::GlobalRoutingHelper::CalculateRoutes();
   NS_LOG_INFO("Calculated global routes.");
 
   // --- 9) Run simulation ---
   NS_LOG_INFO("Starting simulation for 10 seconds.");
   Simulator::Stop(Seconds(10.0));
   Simulator::Run();
   Simulator::Destroy();
   NS_LOG_INFO("Simulation finished.");
 
   return 0;
 }