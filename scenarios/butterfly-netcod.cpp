/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-multiple-strategies.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"

#include "random-load-balancer-strategy.hpp"

using namespace ns3;
using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::FibHelper;
using ns3::ndn::StrategyChoiceHelper;
using ns3::ndn::AppDelayTracer;
using ns3::ndn::L3RateTracer;

NS_LOG_COMPONENT_DEFINE ("Butterfly");

#define _LOG_INFO(x) NS_LOG_INFO(x)

/**
 * This scenario simulates a simple multipath topology
 *
 *                                  /------\                  
 *                              +-->| CR 2 |<--+              
 *                             /    \------/    \             
 *                            /                  \            
 *  /--------\     /------\<-+                    +-->/--------\
 *  | Source |<--->| CR 1 |                           | Client |
 *  \--------/     \------/<-+                    +-->\--------/
 *							              \                  /            
 *							               \    /------\    /             
 *							              	+-->| CR 3 |<--+              
 *							              	    \------/                  
 *
 *
 * All links are 1Mbps with propagation 10ms delay.
 *
 * FIB is populated using NdnGlobalRoutingHelper.
 *
 * Consumer requests data from producer with frequency 100 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=multipath-simple
 */

int
main(int argc, char* argv[])
{

	CommandLine cmd;
	cmd.Parse(argc, argv);

	Config::SetDefault("ns3::PointToPointNetDevice::Mtu", StringValue("65535"));

	AnnotatedTopologyReader topologyReader("",25);
	topologyReader.SetFileName("topologies/topo-butterfly.txt");
	topologyReader.Read();

	// Getting containers for the consumer/producer
	NodeContainer sources;
	NodeContainer clients;
	NodeContainer interms;

	// Adding sources
	sources.Add("Source1");
	sources.Add("Source2");

	// Adding clients
	clients.Add("Client1");
	clients.Add("Client2");

	// Adding clients
	interms.Add("Interm1");
	interms.Add("Interm2");

	// Application container for custom start time 
	ApplicationContainer apps;

	// Install NDN stack on all other nodes
	StackHelper ndnHelper;
	ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
	ndnHelper.InstallAll();

	// Choosing forwarding strategy
	StrategyChoiceHelper::Install(sources, "/unibe", "/localhost/nfd/strategy/best-route");
	StrategyChoiceHelper::Install(interms, "/unibe", "/localhost/nfd/strategy/multicast");
	//StrategyChoiceHelper::Install<fd::fw::RandomLoadBalancerStrategy>(interms, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::RandomLoadBalancerStrategy>(clients, "/unibe");
	
	// Installing global routing interface on all nodes
	GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();
	
	// Sources: Producer
	AppHelper sourceHelper("ns3::ndn::ProducerNetworkCoding");
	sourceHelper.SetAttribute("PayloadSize", StringValue("1000"));
	ndnGlobalRoutingHelper.AddOrigins("/unibe/video.mp4", sources);
	sourceHelper.SetPrefix("/unibe/video.mp4");
		
	apps = sourceHelper.Install(sources);
	apps.Start(Seconds(0.0));

	// Sources: Consumer (pre-load the CS)
	AppHelper sourceConsumerHelper("ns3::ndn::ConsumerNetworkCodingCbr");
	sourceConsumerHelper.SetAttribute("Frequency", StringValue("1000.0")); // 10 interests a second
	sourceConsumerHelper.SetAttribute("MaxSeq", StringValue("110"));
	//sourceConsumerHelper.SetAttribute("RetxTimer", StringValue("500ms"));
	//sourceConsumerHelper.SetAttribute("LifeTime", StringValue("500ms"));
	sourceConsumerHelper.SetPrefix("/unibe/video.mp4");

	apps = sourceConsumerHelper.Install(sources);
	apps.Start(Seconds(1.0));
	//apps.Stop(Seconds(2.0));

	// Client: Consumer
	// Constant Bit Rate Consumer
	//AppHelper clientHelper("ns3::ndn::ConsumerNetworkCodingCbr");
	//clientHelper.SetAttribute("Frequency", StringValue("1000.0")); // 10 interests a second
		
	// Window Consumer
	AppHelper clientHelper("ns3::ndn::ConsumerNetworkCodingWindow");
	clientHelper.SetAttribute("Window", StringValue("2"));

	// General Consumer options
	clientHelper.SetAttribute("MaxSeq", StringValue("120"));
	clientHelper.SetAttribute("RetxTimer", StringValue("50ms"));
	clientHelper.SetAttribute("LifeTime", StringValue("2s"));
	clientHelper.SetPrefix("/unibe/video.mp4");

	apps = clientHelper.Install(clients);
	apps.Start(Seconds(2.0));

	// Calculate and install FIBs
	//GlobalRoutingHelper::CalculateAllPossibleRoutes();
	FibHelper::AddRoute("Client1", "/unibe", "Source1", 1);
	FibHelper::AddRoute("Client1", "/unibe", "Interm2", 1);
	FibHelper::AddRoute("Client2", "/unibe", "Source2", 1);
	FibHelper::AddRoute("Client2", "/unibe", "Interm2", 1);
	FibHelper::AddRoute("Interm2", "/unibe", "Interm1", 1);
	FibHelper::AddRoute("Interm1", "/unibe", "Source1", 1);
	FibHelper::AddRoute("Interm1", "/unibe", "Source2", 1);

	// Intalling Tracers
	AppDelayTracer::InstallAll("results/app-delays-trace.txt");
	//L3RateTracer::InstallAll("results/l3-rate-trace.txt", Seconds(0.5));
		
	Simulator::Stop(Seconds(10.0));

	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
