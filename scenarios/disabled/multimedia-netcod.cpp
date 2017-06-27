#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

// Custom Strategies
//#include "random-load-balancer-strategy.hpp"
#include "weighted-load-balancer-strategy.hpp"
#include "random-load-balancer-strategy_no-aggregation.hpp"

using namespace ns3;
using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::FibHelper;
using ns3::ndn::StrategyChoiceHelper;
using ns3::ndn::AppDelayTracer;
using ns3::ndn::L3RateTracer;
using ns3::ndn::FileConsumerLogTracer;

NS_LOG_COMPONENT_DEFINE ("File_NetworkCoding");

#define _LOG_INFO(x) NS_LOG_INFO(x)

int
main(int argc, char* argv[])
{
	CommandLine cmd;
	cmd.Parse(argc, argv);

	AnnotatedTopologyReader topologyReader("", 25);
	topologyReader.SetFileName("topologies/topo-butterfly.txt");
	topologyReader.Read();
	
	// Getting containers for the consumer/producer
	NodeContainer sources;
	NodeContainer clients;
	NodeContainer routers;

	// Adding sources
	sources.Add("Source1");
	sources.Add("Source2");

	// Adding clients
	clients.Add("Client1");
	clients.Add("Client2");

	// Adding clients
	routers.Add("Interm1");
	routers.Add("Interm2");

	// Install NDN stack on all nodes
	StackHelper ndnHelper;
	ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
	ndnHelper.InstallAll();

	// Choosing forwarding strategy
	StrategyChoiceHelper::Install<nfd::fw::RandomLoadBalancerStrategy_NA>(sources, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(routers, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(clients, "/unibe");
	
	// Installing global routing interface on all nodes
	GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();

	// Consumer(s)
	ns3::ndn::AppHelper clientHelper("ns3::ndn::FileConsumerNetworkCodingCbr::MultimediaConsumer");
	clientHelper.SetAttribute("AllowUpscale", BooleanValue(true));
	clientHelper.SetAttribute("AllowDownscale", BooleanValue(false));
	clientHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
	clientHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
	clientHelper.SetAttribute("StartRepresentationId", StringValue("auto"));//auto
	clientHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
	clientHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  	clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
  	clientHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/unibe/videos/vid1.mpd" )));

	clientHelper.SetAttribute("LifeTime", StringValue("300ms"));

  	//consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));

	// Install consumers with random start times to randomize the seeds
	srand (time(NULL));
	for (NodeContainer::Iterator it = clients.Begin (); it != clients.End (); ++it)
	{
		ApplicationContainer app = clientHelper.Install(*it);
		double st = 0.5 + 0.001 * (rand() % 50);
		NS_LOG_UNCOND("Delay time for client " << (*it)->GetId() << " is " << st);
		app.Start(Seconds(st));
	}

	// Producer(s)
 	AppHelper producerHelper("ns3::ndn::FakeMultimediaServerNetworkCoding");
 	producerHelper.SetPrefix("/unibe/videos");
	producerHelper.SetAttribute("MetaDataFile", StringValue("data/csv/netflix_vid1.csv"));
	producerHelper.SetAttribute("MPDFileName", StringValue("vid1.mpd"));
  	producerHelper.Install(sources); // install to some node from nodelist
	  
  	ndnGlobalRoutingHelper.AddOrigins("/unibe", sources);

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
	//AppDelayTracer::InstallAll("results/app-delays-trace.txt");
	//L3RateTracer::InstallAll("results/l3-rate-trace.txt", Seconds(0.5));
	FileConsumerLogTracer::InstallAll("results/file-consumer-log-trace.txt");
		
	Simulator::Stop(Seconds(60.0));

	Simulator::Run();
	Simulator::Destroy();

	NS_LOG_UNCOND("Simulation Finished.");

	return 0;
}
