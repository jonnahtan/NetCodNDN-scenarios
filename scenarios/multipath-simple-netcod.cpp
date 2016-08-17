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
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

#include "random-load-balancer-strategy.hpp"

using namespace ns3;
using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::FibHelper;
using ns3::ndn::StrategyChoiceHelper;
using ns3::ndn::AppDelayTracer;
using ns3::ndn::L3RateTracer;

NS_LOG_COMPONENT_DEFINE ("Multipath");

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

	AnnotatedTopologyReader topologyReader("", 25);
	topologyReader.SetFileName("topologies/topo-5-multipath-simple.txt");
	topologyReader.Read();

	// Install NDN stack on all nodes
	StackHelper ndnHelper;
	ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
	ndnHelper.InstallAll();

	// Choosing forwarding strategy
	StrategyChoiceHelper::Install<nfd::fw::RandomLoadBalancerStrategy>(NodeContainer::GetGlobal(), "/unibe");
	
	// Installing global routing interface on all nodes
	GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();

	// Getting containers for the consumer/producer
	Ptr<Node> source = Names::Find<Node>("Source");
	Ptr<Node> client = Names::Find<Node>("Client");

	// CBR Consumer
	AppHelper clientHelper("ns3::ndn::ConsumerNetworkCodingCbr");
	clientHelper.SetAttribute("Frequency", StringValue("2100.0")); // 10 interests a second
	clientHelper.SetAttribute("MaxSeq", StringValue("125")); //130

	// Window Consumer
	//AppHelper clientHelper("ns3::ndn::ConsumerNetworkCodingWindow");
	//clientHelper.SetAttribute("Window", StringValue("5"));
	//clientHelper.SetAttribute("MaxSeq", StringValue("120"));

	// General Consumer options
	clientHelper.SetAttribute("LifeTime", StringValue("1s"));
	clientHelper.SetPrefix("/unibe/video.mp4");

	clientHelper.Install(client);

	AppHelper producerHelper("ns3::ndn::ProducerNetworkCoding");
	producerHelper.SetAttribute("PayloadSize", StringValue("1000"));

	// Register /unibe/video.mp4 prefix with global routing controller and
	// install producer that will satisfy Interests in /unibe/video.mp4 namespace
	ndnGlobalRoutingHelper.AddOrigins("/unibe/video.mp4", source);
	producerHelper.SetPrefix("/unibe/video.mp4");
	producerHelper.Install(source);

	// Calculate and install FIBs
	//GlobalRoutingHelper::CalculateAllPossibleRoutes();
	FibHelper::AddRoute("Client", "/unibe", "Inter2", 1);
	FibHelper::AddRoute("Client", "/unibe", "Inter3", 1);
	FibHelper::AddRoute("Inter2", "/unibe", "Inter1", 1);
	FibHelper::AddRoute("Inter3", "/unibe", "Inter1", 1);
	FibHelper::AddRoute("Inter1", "/unibe", "Source", 1);

	// Intalling Tracers
	AppDelayTracer::InstallAll("results/app-delays-trace.txt");
	//L3RateTracer::InstallAll("results/l3-rate-trace.txt", Seconds(0.5));
		
	Simulator::Stop(Seconds(10.0));

	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
