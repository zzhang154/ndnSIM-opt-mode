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
 #include <iostream>  // For std::cout, std::cerr, std::endl

 using namespace ns3;

 // Define the logging component for this file
 // NS_LOG_COMPONENT_DEFINE("agg-mini-simulation"); // Replaced with std::cout

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
   std::cout << "agg-mini-simulation: Starting main function." << std::endl;
   // Enable logging for specific components - Now done via std::cout in respective files
   // LogComponentEnable("agg-mini-simulation", LOG_LEVEL_ALL);
   // LogComponentEnable("ndn.RootApp", LOG_LEVEL_ALL);
   // LogComponentEnable("ndn.LeafApp", LOG_LEVEL_ALL);
   // LogComponentEnable("ndn.AggregatorApp", LOG_LEVEL_ALL);
   // ns3::LogComponentEnable ("ndn.AggBufferManager", ns3::LOG_LEVEL_ALL);

   CommandLine cmd;
   std::string baseDir = "src/ndnSIM/examples/agg-mini/";
   cmd.AddValue("baseDir", "Base directory for simulation files", baseDir);
   cmd.Parse(argc, argv);

   if (!baseDir.empty() && baseDir.back() != '/')
     baseDir += '/';

   std::string topoPath = baseDir + "topologies/agg-mini-topo.txt";
   std::string treePath = baseDir + "topologies/tree.txt";
   std::cout << "agg-mini-simulation: Using baseDir=" << baseDir
             << ", topoPath=" << topoPath << ", treePath=" << treePath << std::endl;

   // --- 1) Load physical topology from file ---
   AnnotatedTopologyReader topologyReader("", 1.0);
   topologyReader.SetFileName(topoPath);
   topologyReader.Read();
   std::cout << "agg-mini-simulation: Loaded topology from: " << topoPath << std::endl;

   // --- 2) Parse tree.txt to build logical tree structure ---
   std::map<std::string, TreeNodeInfo> nodeInfoMap;
   std::set<std::string> allNodeNames, childNodeNames;
   std::ifstream treeFile(treePath);
   if (!treeFile.is_open()) {
     std::cerr << "agg-mini-simulation: ERROR - Could not open tree file: " << treePath << std::endl;
     return 1;
   }
   std::cout << "agg-mini-simulation: Parsing tree structure from: " << treePath << std::endl;
   std::string line;
   while (std::getline(treeFile, line)) {
     if (line.empty() || line[0] == '#')
       continue;
     std::istringstream iss(line);
     std::string parentName, childName;
     if (!(iss >> parentName))
       continue;
     allNodeNames.insert(parentName);
     nodeInfoMap[parentName].name = parentName; // Ensure node exists even if it has no children listed here
     while (iss >> childName) {
       nodeInfoMap[parentName].children.push_back(childName);
       nodeInfoMap[childName].name = childName; // Ensure child node exists
       nodeInfoMap[childName].parent = parentName;
       allNodeNames.insert(childName);
       childNodeNames.insert(childName);
       // NS_LOG_DEBUG("Parsed link: " << parentName << " -> " << childName); // DEBUG level
       // std::cout << "agg-mini-simulation: DEBUG - Parsed link: " << parentName << " -> " << childName << std::endl; // Uncomment for debug
     }
   }
   treeFile.close();
   std::cout << "agg-mini-simulation: Finished parsing tree file. Found " << allNodeNames.size() << " unique nodes." << std::endl;

   // --- 3) Identify node roles (Root, Aggregator, Leaf) ---
   std::string rootName = "";
   std::vector<std::string> aggregatorNames;
   std::vector<std::string> leafNames;
   for (auto const& [name, info] : nodeInfoMap) { // Use structured binding (C++17)
     // Check if node is a parent but not a child -> Root
     if (childNodeNames.count(name) == 0 && !info.children.empty()) {
       nodeInfoMap[name].isRoot = true; // Mark in map
       if (!rootName.empty()) {
         std::cout << "agg-mini-simulation: WARNING - Multiple roots found (" << rootName << ", " << name
                      << "). Using first one: " << rootName << std::endl;
       }
       else {
         rootName = name;
         std::cout << "agg-mini-simulation: Identified Root: " << rootName << std::endl;
       }
     }
     // Check if node is a child AND a parent -> Aggregator
     else if (!info.parent.empty() && !info.children.empty()) {
       nodeInfoMap[name].isAggregator = true; // Mark in map
       aggregatorNames.push_back(name);
       std::cout << "agg-mini-simulation: Identified Aggregator: " << name << std::endl;
     }
     // Check if node is a child but NOT a parent -> Leaf
     else if (!info.parent.empty() && info.children.empty()) {
       nodeInfoMap[name].isLeaf = true; // Mark in map
       leafNames.push_back(name);
       std::cout << "agg-mini-simulation: Identified Leaf: " << name << std::endl;
     }
     // Handle single-node case (is both root and leaf, treat as root)
     else if (info.parent.empty() && info.children.empty()
              && allNodeNames.size() == 1) {
       nodeInfoMap[name].isRoot = true; // Mark in map
       rootName = name;
       std::cout << "agg-mini-simulation: Identified single node as Root: " << rootName << std::endl;
     }
     // Else: could be an isolated node not part of the main tree, or error in tree.txt
     else if (info.parent.empty() && info.children.empty() && allNodeNames.size() > 1) {
         std::cout << "agg-mini-simulation: WARNING - Node " << name << " appears isolated in tree.txt." << std::endl;
     }
   }
   // Sanity checks
   if (rootName.empty() && !allNodeNames.empty()) {
     std::cerr << "agg-mini-simulation: ERROR - Could not identify a root node." << std::endl;
     return 1;
   }
   if (aggregatorNames.size() > 1) {
     std::cout << "agg-mini-simulation: WARNING - Multiple aggregators found. This simple example logic "
                 "assumes one aggregator. Using first: " << aggregatorNames[0] << std::endl;
   }
   if (aggregatorNames.empty() && !leafNames.empty()) {
     std::cerr << "agg-mini-simulation: ERROR - Leaves found but no aggregator identified." << std::endl;
     return 1;
   }
   std::string aggName = aggregatorNames.empty() ? "" : aggregatorNames[0]; // Use first aggregator if multiple exist

   // --- 4) Install NDN stack ---
   ns3::ndn::StackHelper ndnHelper;
   ndnHelper.InstallAll();
   std::cout << "agg-mini-simulation: Installed NDN stack on all nodes." << std::endl;

   // --- 6) Set up FIB routes based on tree structure ---
   // Note: GlobalRoutingHelper (step 8) might override these, but setting them
   // explicitly based on the tree can be useful for understanding/debugging.
   const std::string aggPrefix = "/app/agg";
   if (!rootName.empty() && !aggName.empty()) {
     ns3::ndn::FibHelper::AddRoute(rootName, aggPrefix, aggName, 1);
     std::cout << "agg-mini-simulation: FIB - Added route on " << rootName
                 << " for " << aggPrefix << " via " << aggName << std::endl;
   }
   if (!aggName.empty()) {
     for (const std::string& leafName : leafNames) {
       std::string childPrefix = "/app/" + leafName; // Assuming prefix matches leaf name
       ns3::ndn::FibHelper::AddRoute(aggName, childPrefix, leafName, 1);
       std::cout << "agg-mini-simulation: FIB - Added route on " << aggName
                   << " for " << childPrefix << " via " << leafName << std::endl;
     }
   }

   // --- 5) Install Strategy ---
   ns3::ndn::StrategyChoiceHelper::InstallAll(
     "/", "/localhost/nfd/strategy/best-route"); // Use default BestRoute strategy
   std::cout << "agg-mini-simulation: Installed BestRoute strategy on all nodes." << std::endl;

   // --- 7) Install custom apps based on roles ---
   if (!rootName.empty()) {
     Ptr<Node> rootNode = Names::Find<Node>(rootName);
     if (!rootNode) {
       std::cerr << "agg-mini-simulation: ERROR - Root node '" + rootName + "' not found in topology" << std::endl;
       return 1; // Use return instead of throw for cleaner exit in main
     }
     ns3::ndn::AppHelper rootHelper("ns3::ndn::RootApp");
     rootHelper.SetPrefix(aggPrefix);
     rootHelper.SetAttribute("MaxSeq", UintegerValue(3)); // Example: send 3 sequences
     rootHelper.SetAttribute("Interval", DoubleValue(1.0)); // Send every 1 second
     rootHelper.Install(rootNode).Start(Seconds(0.0));
     std::cout << "agg-mini-simulation: Installed RootApp on " << rootName
                 << " sending for " << aggPrefix << std::endl;
   }
   if (!aggName.empty()) {
    Ptr<Node> aggNode = Names::Find<Node>(aggName);
    if (!aggNode) {
      std::cerr << "agg-mini-simulation: ERROR - Aggregator node '" + aggName + "' not found" << std::endl;
      return 1;
    }
    ns3::ndn::AppHelper aggHelper("ns3::ndn::AggregatorApp");
    aggHelper.SetPrefix(aggPrefix); // Listens for upstream interests

    // Construct ChildPrefixes attribute string
    std::ostringstream oss;
    bool first = true;
    for (auto const& leafName : leafNames) {
        if (!first) oss << " ";
        oss << "/app/" << leafName; // Assuming prefix matches leaf name
        first = false;
    }
    aggHelper.SetAttribute("ChildPrefixes", StringValue(oss.str()));
    // Set other AggregatorApp attributes if needed (BufferCapacity, StragglerTimeout use defaults)
    aggHelper.Install(aggNode).Start(Seconds(0.0));

    std::cout << "agg-mini-simulation: Installed AggregatorApp on " << aggName
                << " for " << aggPrefix
                << " -> children {" << oss.str() << "}" << std::endl;
  }

  for (const std::string& leafName : leafNames) {
    Ptr<Node> leafNode = Names::Find<Node>(leafName);
     if (!leafNode) {
       std::cerr << "agg-mini-simulation: ERROR - Leaf node '" + leafName + "' not found" << std::endl;
       // Continue installing on other leaves if possible, or return 1
       continue;
     }
    std::string childPrefix = "/app/" + leafName; // Assuming prefix matches leaf name
    ns3::ndn::AppHelper leafHelper("ns3::ndn::LeafApp");
    leafHelper.SetPrefix(childPrefix); // Producer listens on this prefix
    // Set LeafApp attributes if any (none in this simple version)
    leafHelper.Install(leafNode).Start(Seconds(0.0));
    std::cout << "agg-mini-simulation: Installed LeafApp on " << leafName
                << " listening for " << childPrefix << std::endl;
  }

  // --- 8) Set up global routing origins ---
  // This calculates and installs routes across the whole topology
  ns3::ndn::GlobalRoutingHelper globalRouting;
  globalRouting.InstallAll();
  // Register origins for prefixes served by producers/aggregators
  if (!aggName.empty()) {
    Ptr<Node> aggNode = Names::Find<Node>(aggName);
    if (aggNode) { // Check if node exists
        globalRouting.AddOrigins(aggPrefix, aggNode);
        std::cout << "agg-mini-simulation: GlobalRouting - Added origin " << aggPrefix
                    << " at " << aggName << std::endl;
    }
  }
  for (const std::string& leafName : leafNames) {
    Ptr<Node> leafNode = Names::Find<Node>(leafName);
     if (leafNode) { // Check if node exists
        std::string childPrefix = "/app/" + leafName;
        globalRouting.AddOrigins(childPrefix, leafNode);
        std::cout << "agg-mini-simulation: GlobalRouting - Added origin " << childPrefix
                    << " at " << leafName << std::endl;
     }
  }
  // Calculate and install routes
  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();
  std::cout << "agg-mini-simulation: Calculated and installed global routes." << std::endl;

  // --- 9) Run simulation ---
  double stopTime = 10.0; // seconds
  std::cout << "agg-mini-simulation: Starting simulation for " << stopTime << " seconds." << std::endl;
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run();
  Simulator::Destroy();
  std::cout << "agg-mini-simulation: Simulation finished." << std::endl;

  return 0;
 }