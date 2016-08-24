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

NS_LOG_COMPONENT_DEFINE ("File_NetworkCoding");

#define _LOG_INFO(x) NS_LOG_INFO(x)

int
main(int argc, char* argv[])
{
	CommandLine cmd;
	cmd.Parse(argc, argv);

	AnnotatedTopologyReader topologyReader("", 25);
	topologyReader.SetFileName("topologies/star-multipath.txt");
	topologyReader.Read();
	
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
	ns3::ndn::AppHelper clientHelper("ns3::ndn::FileConsumerWindow::MultimediaConsumer");
	clientHelper.SetAttribute("AllowUpscale", BooleanValue(true));
	clientHelper.SetAttribute("AllowDownscale", BooleanValue(false));
	clientHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
	clientHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
	clientHelper.SetAttribute("StartRepresentationId", StringValue("auto"));//auto
	clientHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
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
		uint64_t startTime = 100 + (rand() % 10);
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

	// Intalling Tracers
	//AppDelayTracer::InstallAll("results/app-delays-trace.txt");
	//L3RateTracer::InstallAll("results/l3-rate-trace.txt", Seconds(0.5));
	//FileConsumerLogTracer::InstallAll("results/file-consumer-log-trace.txt");
	DASHPlayerTracer::InstallAll("results/dash-tracer-traditional-star.txt");
		
	Simulator::Stop(Seconds(60.0));

	Simulator::Run();
	Simulator::Destroy();

	NS_LOG_UNCOND("Simulation Finished.");

	return 0;
}
