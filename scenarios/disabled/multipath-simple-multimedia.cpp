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
using ns3::ndn::FileConsumerLogTracer;

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
 *				              \                  /            
 *				               \    /------\    /             
 *				              	+-->| CR 3 |<--+              
 *				              	    \------/                  
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

	// Consumer
	AppHelper clientHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
	clientHelper.SetAttribute("AllowUpscale", BooleanValue(true));
 	clientHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  	clientHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  	clientHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  	clientHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  	clientHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
  	clientHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

	clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
  	clientHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/unibe/video1/vid1.mpd" )));
	//clientHelper.SetAttribute("WriteOutfile", StringValue("/mnt/c/Users/salta/Code/NetCodNDN/scenarios/networking2016/data/out/Johnny_1280x720_60.webm"));
	clientHelper.Install(client);
	//consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));

	// Producer
  	AppHelper producerHelper("ns3::ndn::FakeMultimediaServer");

  	// This fake multimedia producer will reply to all requests starting with /myprefix/FakeVid1
  	producerHelper.SetPrefix("/unibe/video1");
  	producerHelper.SetAttribute("MetaDataFile", StringValue("/mnt/c/Users/salta/Code/NetCodNDN/scenarios/networking2016/data/multimedia/representations/netflix_vid1.csv"));
  	// We just give the MPD file a name that makes it unique
  	producerHelper.SetAttribute("MPDFileName", StringValue("vid1.mpd"));

  	producerHelper.Install(source);

  	// We can install more then one fake multimedia producer on one node:

  	// This fake multimedia producer will reply to all requests starting with /myprefix/FakeVid2
  	//producerHelper.SetPrefix("/unibe/video2");
  	//producerHelper.SetAttribute("MetaDataFile", StringValue("/mnt/c/Users/salta/Code/NetCodNDN/scenarios/networking2016/data/multimedia/representations/netflix_vid2.csv"));
  	// We just give the MPD file a name that makes it unique
  	//producerHelper.SetAttribute("MPDFileName", StringValue("vid2.mpd"));

  	ndnGlobalRoutingHelper.AddOrigins("/unibe", source);

	// Calculate and install FIBs
	//GlobalRoutingHelper::CalculateAllPossibleRoutes();
	FibHelper::AddRoute("Client", "/unibe", "Inter2", 1);
	FibHelper::AddRoute("Client", "/unibe", "Inter3", 1);
	FibHelper::AddRoute("Inter2", "/unibe", "Inter1", 1);
	FibHelper::AddRoute("Inter3", "/unibe", "Inter1", 1);
	FibHelper::AddRoute("Inter1", "/unibe", "Source", 1);

	// Intalling Tracers
	//AppDelayTracer::InstallAll("results/app-delays-trace.txt");
	//L3RateTracer::InstallAll("results/l3-rate-trace.txt", Seconds(0.5));
	FileConsumerLogTracer::InstallAll("results/file-consumer-log-trace.txt");
		
	Simulator::Stop(Seconds(600.0));

	Simulator::Run();
	Simulator::Destroy();

	NS_LOG_UNCOND("Simulation Finished.");

	return 0;
}
