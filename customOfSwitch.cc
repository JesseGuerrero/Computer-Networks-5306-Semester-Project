/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 *         Vitor M. Eichemberger <vitor.marge@gmail.com>
 *
 * Two hosts connected to a single OpenFlow switch.
 * The switch is managed by the default learning controller application.
 *
 *                       Learning Controller
 *                                |
 *                       +-----------------+
 *            Host 0 === | OpenFlow switch | === Host 1
 *                       +-----------------+
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
void SetupSwitch(NodeContainer hosts);
void SetupIpv4Addresses();
void InstallPing(Ptr<Node> src, Ptr<Node> dest);

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
	csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
	csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (500)));

	// Create the controller node
	Ptr<Node> controllerNode = CreateObject<Node> ();
	Names::Add("Controller", controllerNode);

	// Configure the OpenFlow network domain
	of13Helper->InstallController (controllerNode);

	//Southbound Network
	NodeContainer hosts1, hosts2, hosts3;
	hosts1.Create (3); hosts2.Create(3); hosts3.Create(3);
	SetupSwitch(hosts1);
	SetupSwitch(hosts2);
	SetupSwitch(hosts3);

	//OfSwitch Config
	of13Helper->SetChannelType(OFSwitch13Helper::ChannelType::DEDICATEDP2P);
	of13Helper->CreateOpenFlowChannels ();

	SetupIpv4Addresses();
	//InstallPing(hosts1.Get(0), hosts2.Get(0));

	//Visuals
	SetNodeXY(controllerNode, 15, 5);
	SetAllNodesXY(switches, 5, 12.5, 7.5);
	AnimationInterface anim("OfExampleAnim.xml");

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
	//anim.SetConstantPosition(node, x, y, 0);
}

void SetupSwitch(NodeContainer hosts) {
	for (size_t i = 0; i < hosts.GetN (); i++)
	{
	  NodeContainer pair (hosts.Get (i), switches.Get(2));
	  NetDeviceContainer link = csmaHelper.Install (pair);
	  hostDevices.Add (link.Get (0));
	  switchPorts.Add (link.Get (1));
	}
	of13Helper->InstallSwitch (switches.Get(2), switchPorts);
	internet.Install (hosts);
	SetAllNodesXY(hosts, 17, 20, 1);
}

void SetupIpv4Addresses() {
	Ipv4AddressHelper ipv4helpr;
	Ipv4InterfaceContainer hostIpIfaces;
	ipv4helpr.SetBase ("10.1.1.0", "255.255.255.0");
	hostIpIfaces = ipv4helpr.Assign (hostDevices);
}

void InstallPing(Ptr<Node> src, Ptr<Node> dest) {
	// Configure ping application between hosts
	V4PingHelper pingHelper = V4PingHelper (dest->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());
	pingHelper.SetAttribute ("Interval", TimeValue (Seconds (100)));
	pingHelper.SetAttribute ("Verbose", BooleanValue (true));

	ApplicationContainer pingApps = pingHelper.Install(src);
	pingApps.Start (Seconds (3));
}