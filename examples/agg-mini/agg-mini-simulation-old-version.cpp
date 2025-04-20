/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Simple Root–Agg–Leaf topology demo for ndnSIM‑2.9 using AnnotatedTopologyReader
 */

 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/ndnSIM-module.h"
 
 using namespace ns3;
 
 int
 main(int argc, char* argv[])
 {
   CommandLine cmd;
   std::string baseDir = "src/ndnSIM/examples/agg-mini/";
   cmd.AddValue("baseDir", "Base directory for simulation files", baseDir);
   cmd.Parse(argc, argv);
 
   if (!baseDir.empty() && baseDir[baseDir.length() - 1] != '/')
     baseDir += '/';
 
   std::string topoPath = baseDir + "topologies/agg-mini-topo.txt";
 
   // 1) Load topology from file
   AnnotatedTopologyReader topologyReader("", 1.0);
   topologyReader.SetFileName(topoPath);
   topologyReader.Read();
 
   // 2) Install NDN stack and BestRoute strategy
   ns3::ndn::StackHelper ndnHelper;
   ndnHelper.InstallAll();
   ns3::ndn::FibHelper::AddRoute("Root", "/app/agg",  "Agg",  1);
   ns3::ndn::FibHelper::AddRoute("Agg",  "/app/leaf", "Leaf", 1);
   ns3::ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
 
   // 3) Install custom apps
   Ptr<Node> root = Names::Find<Node>("Root");
   Ptr<Node> agg  = Names::Find<Node>("Agg");
   Ptr<Node> leaf = Names::Find<Node>("Leaf");
 
   // RootApp: sends 3 Interests at /app/agg
   ns3::ndn::AppHelper rootHelper("ns3::ndn::RootApp");
   rootHelper.SetPrefix("/app/agg");
   rootHelper.SetAttribute("MaxSeq", UintegerValue(3));
   rootHelper.SetAttribute("Interval", DoubleValue(1.0));
   rootHelper.Install(root).Start(Seconds(0.0));
 
   // AggregatorApp: relays /app/agg → /app/leaf
   ns3::ndn::AppHelper aggHelper("ns3::ndn::AggregatorApp");
   aggHelper.SetPrefix("/app/agg");
   aggHelper.SetAttribute("ChildPrefix", StringValue("/app/leaf"));
   aggHelper.Install(agg).Start(Seconds(0.0));
 
   // LeafApp: replies to /app/leaf Interests
   ns3::ndn::AppHelper leafHelper("ns3::ndn::LeafApp");
   leafHelper.SetPrefix("/app/leaf");
   leafHelper.Install(leaf).Start(Seconds(0.0));
 
   // 4) Set up global routing for /app/leaf and /app/agg, then compute FIBs
   ns3::ndn::GlobalRoutingHelper globalRouting;
   globalRouting.InstallAll();
   globalRouting.AddOrigins("/app/leaf", leaf);
   globalRouting.AddOrigins("/app/agg", agg);
   ns3::ndn::GlobalRoutingHelper::CalculateRoutes();
 
   // 5) Run simulation for 10 seconds
   Simulator::Stop(Seconds(10.0));
   Simulator::Run();
   Simulator::Destroy();
 
   return 0;
 }