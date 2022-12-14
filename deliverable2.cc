/*
 * Deliverable 2: Uniquely coded using the OfSwitch13 module
 */

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/internet-apps-module.h>
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
NodeContainer switches;
InternetStackHelper internet; // Install the TCP/IP stack into hosts nodes
CsmaHelper csmaHelper;
NetDeviceContainer hostDevices;
NetDeviceContainer switchPorts;

void SetAllNodesXY(NodeContainer nodes, double x, double y, double deltaX);
void SetNodeXY(Ptr<Node> node, double x, double y);
void SetupSwitch(NodeContainer hosts, uint16_t switchID, uint16_t xCoord);
void SetupIpv4Addresses();
void InstallPing(Ptr<Node> src, Ptr<Node> dest);

void MacTxTrace(std::string context, Ptr<const Packet> pkt) {
	std::cout << context << std::endl;
	std::cout << "\tMaxTX Size=" << pkt->GetSize() << "\t" << Simulator::Now().As(ns3::Time::Unit::MS) << std::endl;
}

int
main (int argc, char *argv[])
{
	uint16_t simTime = 38;
	bool verbose = false;
	bool trace = false;

	// Configure command line parameters
	CommandLine cmd;
	cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
	cmd.AddValue ("verbose", "Enable verbose output", verbose);
	cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
	cmd.Parse (argc, argv);

	if (verbose)
	{
	  OFSwitch13Helper::EnableDatapathLogs ();
	  LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
	  LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
	}

	// Enable checksum computations (required by OFSwitch13 module)
	GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));


	switches.Create(3);

	// Use the CsmaHelper to connect host nodes to the switch node
	csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
	csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

	// Create the controller node
	Ptr<Node> controllerNode = CreateObject<Node> ();
	Names::Add("Controller", controllerNode);

	// Configure the OpenFlow network domain
	of13Helper->InstallController (controllerNode);

	NodeContainer hosts1, hosts2, hosts3;
	hosts1.Create (2); hosts2.Create(3); hosts3.Create(3);
	// Create two host nodes
	//switch 1
	SetupSwitch(hosts1, 0, 3);
	SetupSwitch(hosts2, 1, 10);
	SetupSwitch(hosts3, 2, 17);
	Config::Connect ("/NodeList/5/DeviceList/*/$ns3::CsmaNetDevice/MacRx", MakeCallback(&MacTxTrace));
	Config::Connect ("/NodeList/6/DeviceList/*/$ns3::CsmaNetDevice/MacRx", MakeCallback(&MacTxTrace));
	Config::Connect ("/NodeList/5/DeviceList/*/$ns3::CsmaNetDevice/PhyTxBegin", MakeCallback(&MacTxTrace));
	Config::Connect ("/NodeList/6/DeviceList/*/$ns3::CsmaNetDevice/PhyTxBegin", MakeCallback(&MacTxTrace));

	//OfSwitch Config
	of13Helper->SetChannelType(OFSwitch13Helper::ChannelType::DEDICATEDP2P);
	of13Helper->CreateOpenFlowChannels ();

	SetupIpv4Addresses();
	V4PingHelper pingHelper = V4PingHelper ("10.1.1.4");
	pingHelper.SetAttribute ("Interval", TimeValue (Seconds (100)));
	pingHelper.SetAttribute ("Verbose", BooleanValue (true));

	ApplicationContainer pingApps = pingHelper.Install(hosts1.Get(1));
	pingApps.Start (Seconds (3));

	SetNodeXY(controllerNode, 15, 5);
	SetAllNodesXY(switches, 5, 12.5, 7.5);

	std::vector<NodeContainer> allHosts;
	allHosts.push_back(hosts1);
	allHosts.push_back(hosts2);
	allHosts.push_back(hosts3);
	AnimationInterface anim("D2.xml");
	uint32_t switchImageID = anim.AddResource("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/Switch.png");
	uint32_t workstationImageID = anim.AddResource("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/workstation.png");
	uint32_t SDNImageID = anim.AddResource("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/SDN.png");
	uint32_t routerImageID = anim.AddResource("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/router.png");
	uint32_t laptopImageID = anim.AddResource("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/laptop.png");
	uint32_t serverImageID = anim.AddResource("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/server.png");
	anim.UpdateNodeColor(controllerNode, 0, 0, 0);
	anim.SetBackgroundImage("/home/brian-jesse/Downloads/bake/source/ns-3.32/scratch/background.png", -4, -5, 0.025, 0.0325, 1);
	for(uint16_t i = 0; i < allHosts.size(); i++) {
		NodeContainer hosts = allHosts.at(i);
		for(uint16_t j = 0; j < hosts.GetN(); j++) {
			anim.UpdateNodeColor(hosts.Get(j), 0, 255, 255);
		}
	}
	//Controller
	anim.UpdateNodeImage(3, SDNImageID);
	anim.UpdateNodeSize(3, 3, 3);
	//Switch
	for(uint16_t i = 0; i <= 2; i++) {
		anim.UpdateNodeImage(i, switchImageID);
		anim.UpdateNodeSize(i, 3, 3);
	}
	//Hosts
	anim.UpdateNodeSize(4, 3, 3);
	anim.UpdateNodeImage(4, routerImageID);
	anim.UpdateNodeSize(5, 3, 3);
	anim.UpdateNodeImage(5, laptopImageID);
	for(uint16_t i = 6; i <= 8; i++) {
		anim.UpdateNodeSize(i, 3, 3);
		anim.UpdateNodeImage(i, serverImageID);
	}
	for(uint16_t i = 9; i <= 11; i++) {
		anim.UpdateNodeSize(i, 3, 3);
		anim.UpdateNodeImage(i, workstationImageID);
	}

	// Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
	if (trace)
	{
	  of13Helper->EnableOpenFlowPcap ("openflow");
	  of13Helper->EnableDatapathStats ("switch-stats");
	  csmaHelper.EnablePcap ("switch", switchPorts, true);
	  csmaHelper.EnablePcap ("host", hostDevices);
	}

	// Run the simulation
	Simulator::Stop (Seconds (simTime));
	Simulator::Run ();
	Simulator::Destroy ();
}

void SetAllNodesXY(NodeContainer nodes, double x, double y, double deltaX) {
	MobilityHelper mobileHosts;
	mobileHosts.SetPositionAllocator ("ns3::GridPositionAllocator",
											"MinX", DoubleValue (x),
											"MinY", DoubleValue (y),
											"DeltaX", DoubleValue (deltaX),
										   "DeltaY", DoubleValue (10.0),
										   "GridWidth", UintegerValue (nodes.GetN()),
											 "LayoutType", StringValue ("RowFirst"));
	mobileHosts.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobileHosts.Install (nodes);
}

void SetNodeXY(Ptr<Node> node, double x, double y) {
	MobilityHelper mobileHosts;
	mobileHosts.SetPositionAllocator ("ns3::GridPositionAllocator",
											"MinX", DoubleValue (x),
											"MinY", DoubleValue (y),
											"DeltaX", DoubleValue (10.0),
										   "DeltaY", DoubleValue (10.0),
										   "GridWidth", UintegerValue (3),
											 "LayoutType", StringValue ("RowFirst"));
	mobileHosts.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobileHosts.Install (node);
}

void SetupSwitch(NodeContainer hosts, uint16_t switchID, uint16_t xCoord) {
	for (size_t i = 0; i < hosts.GetN (); i++)
	{
	  NodeContainer pair (hosts.Get (i), switches.Get(switchID));
	  NetDeviceContainer link = csmaHelper.Install (pair);
	  hostDevices.Add (link.Get (0));
	  switchPorts.Add (link.Get (1));
	}
	of13Helper->InstallSwitch (switches.Get(switchID), switchPorts);
	internet.Install (hosts);
	SetAllNodesXY(hosts, xCoord, 20, 2);
}

void SetupIpv4Addresses() {
	Ipv4AddressHelper ipv4helpr;
	Ipv4InterfaceContainer hostIpIfaces;
	ipv4helpr.SetBase ("10.1.1.0", "255.255.255.0");
	hostIpIfaces = ipv4helpr.Assign (hostDevices);
}

void InstallPing(Ptr<Node> src, Ptr<Node> dest) {
	// Configure ping application between hosts
	V4PingHelper pingHelper = V4PingHelper ("10.1.1.2");
	pingHelper.SetAttribute ("Interval", TimeValue (Seconds (100)));
	pingHelper.SetAttribute ("Verbose", BooleanValue (true));

	ApplicationContainer pingApps = pingHelper.Install(src);
	pingApps.Start (Seconds (3));
}
