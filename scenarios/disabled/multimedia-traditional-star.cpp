#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

// Custom Strategies
#include "random-load-balancer-strategy.hpp"
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
using ns3::ndn::DASHPlayerTracer;

NS_LOG_COMPONENT_DEFINE ("File_Traditional");

#define _LOG_INFO(x) NS_LOG_INFO(x)

int
main(int argc, char* argv[])
{
	CommandLine cmd;
	cmd.Parse(argc, argv);

	AnnotatedTopologyReader topologyReader("", 25);
	topologyReader.SetFileName("topologies/star-multipath.txt");
	topologyReader.Read();

	// Install NDN stack on all nodes
	StackHelper ndnHelper;
	ndnHelper.setCsSize(1000000);
	ndnHelper.InstallAll();
	
	// Getting containers for the consumer/producer
	NodeContainer sources;
	NodeContainer routers;
	NodeContainer clients;

	// Adding sources
	sources.Add("l1-1");

	// Adding routers
	routers.Add("l2-1");
	routers.Add("l2-2");
	routers.Add("l2-3");
	routers.Add("l2-4");

	// Adding clients
	clients.Add("l3-1");
	clients.Add("l3-2");
	clients.Add("l3-3");
	clients.Add("l3-4");
	clients.Add("l3-5");
	clients.Add("l3-6");
	clients.Add("l3-7");
	clients.Add("l3-8");

	// Calculate and install FIBs
	
	// L2 to L1
	FibHelper::AddRoute("l2-1", "/unibe", "l1-1", 1);
	FibHelper::AddRoute("l2-2", "/unibe", "l1-1", 1);
	FibHelper::AddRoute("l2-3", "/unibe", "l1-1", 1);
	FibHelper::AddRoute("l2-4", "/unibe", "l1-1", 1);

	// L3 to L2
	FibHelper::AddRoute("l3-1", "/unibe", "l2-1", 1);
	FibHelper::AddRoute("l3-2", "/unibe", "l2-1", 1);
	FibHelper::AddRoute("l3-7", "/unibe", "l2-1", 1);
	FibHelper::AddRoute("l3-8", "/unibe", "l2-1", 1);
	FibHelper::AddRoute("l3-2", "/unibe", "l2-2", 1);
	FibHelper::AddRoute("l3-3", "/unibe", "l2-2", 1);
	FibHelper::AddRoute("l3-4", "/unibe", "l2-2", 1);
	FibHelper::AddRoute("l3-5", "/unibe", "l2-2", 1);
	FibHelper::AddRoute("l3-3", "/unibe", "l2-3", 1);
	FibHelper::AddRoute("l3-4", "/unibe", "l2-3", 1);
	FibHelper::AddRoute("l3-5", "/unibe", "l2-3", 1);
	FibHelper::AddRoute("l3-6", "/unibe", "l2-3", 1);
	FibHelper::AddRoute("l3-1", "/unibe", "l2-4", 1);
	FibHelper::AddRoute("l3-6", "/unibe", "l2-4", 1);
	FibHelper::AddRoute("l3-7", "/unibe", "l2-4", 1);
	FibHelper::AddRoute("l3-8", "/unibe", "l2-4", 1);

	// Choosing forwarding strategy
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(sources, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(routers, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(clients, "/unibe");
	
	// Installing global routing interface on all nodes
	GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();

	// Consumer(s)
	ns3::ndn::AppHelper clientHelper("ns3::ndn::FileConsumerWindow::MultimediaConsumer");
	clientHelper.SetAttribute("AllowUpscale", BooleanValue(true));
	clientHelper.SetAttribute("AllowDownscale", BooleanValue(false));
	clientHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
	clientHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
	clientHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
	clientHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(60));
	clientHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  	clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
  	clientHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/unibe/videos/vid1.mpd" )));

	clientHelper.SetAttribute("LifeTime", StringValue("500ms"));

  	//consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));

	// Install consumers with random start times to randomize the seeds
	srand (time(NULL));
	for (NodeContainer::Iterator it = clients.Begin (); it != clients.End (); ++it)
	{
		ApplicationContainer app = clientHelper.Install(*it);
		uint64_t startTime = 100 + (rand() % 100);
		NS_LOG_UNCOND("Delay time for client " << (*it)->GetId() << " is " << startTime);
		app.Start(MilliSeconds(startTime));
	}

	// Producer(s)
 	AppHelper producerHelper("ns3::ndn::FakeMultimediaServer");
 	producerHelper.SetPrefix("/unibe/videos");
	producerHelper.SetAttribute("MetaDataFile", StringValue("data/csv/netflix_vid1.csv"));
	producerHelper.SetAttribute("MPDFileName", StringValue("vid1.mpd"));
  	producerHelper.Install(sources); // install to some node from nodelist
	  
  	ndnGlobalRoutingHelper.AddOrigins("/unibe", sources);

	// Intalling Tracers
	//L3RateTracer::Install(sources, "results/star/traditional/l3-rate-trace.txt", Seconds(1.0));
	L3RateTracer::InstallAll("results/star/traditional/l3-rate-trace.txt", Seconds(0.25));
	FileConsumerLogTracer::InstallAll("results/star/traditional/file-consumer-log-trace.txt");
	DASHPlayerTracer::InstallAll("results/star/traditional/dash-trace.txt");
		
	Simulator::Stop(Seconds(200.0));

	Simulator::Run();
	Simulator::Destroy();

	NS_LOG_UNCOND("Simulation Finished.");

	return 0;
}