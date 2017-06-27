#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

// Custom Strategies
//#include "random-load-balancer-strategy.hpp"
#include "weighted-load-balancer-strategy.hpp"
#include "random-load-balancer-strategy_no-aggregation.hpp"
#include <boost/filesystem.hpp>

using namespace ns3;
using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::FibHelper;
using ns3::ndn::StrategyChoiceHelper;
using ns3::ndn::L3RateTracer;
using ns3::ndn::FileConsumerLogTracer;
using ns3::ndn::DASHPlayerTracer;

NS_LOG_COMPONENT_DEFINE ("MultimediaNetcodGenerated");

#define _LOG_INFO(x) NS_LOG_INFO(x)

int
main(int argc, char* argv[])
{
	int VIDEOS = 3;
	srand (time(NULL));

	CommandLine cmd;
	int runId = 1;
	cmd.AddValue ("runid", "ID for this run", runId);
	cmd.Parse(argc, argv);

	AnnotatedTopologyReader topologyReader("", 25);
	topologyReader.SetFileName("topologies/layer-generated.txt");
	topologyReader.Read();

	// Getting containers for the consumer/producer
	NodeContainer sources;
	NodeContainer routers;
	NodeContainer clients;

    // Fill the containers and set the routing (FIB)
    #include "../topologies/layer-generated-cpp.txt"

	/*

	// Install NDN stack
	StackHelper ndnSources;
	ndnSources.setCsSize(100);
	ndnSources.Install(sources);

	StackHelper ndnRouters;
	ndnRouters.setCsSize(100);
	ndnRouters.Install(routers);

	StackHelper ndnClients;
	ndnClients.setCsSize(1);
	ndnClients.Install(clients);

	*/

	// Choosing forwarding strategy
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(sources, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(routers, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(clients, "/unibe");

	// Consumer(s)
	ns3::ndn::AppHelper clientHelper("ns3::ndn::FileConsumerNetworkCodingWindow::MultimediaConsumer");
	clientHelper.SetAttribute("AllowUpscale", BooleanValue(true));
	clientHelper.SetAttribute("AllowDownscale", BooleanValue(false));
	clientHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
	clientHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
	clientHelper.SetAttribute("StartRepresentationId", StringValue("lowest"));
	clientHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
	clientHelper.SetAttribute("StartUpDelay", StringValue("2.0"));
	clientHelper.SetAttribute("LifeTime", StringValue("1000ms"));

  	//clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
	clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::DASHJSAdaptationLogic"));

	//clientHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/unibe/videos/video1.mpd" )));

	// Distribution of the videos requested by the clients
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, VIDEOS);

	// Install consumers with random start times to randomize the seeds
	for (NodeContainer::Iterator it = clients.Begin (); it != clients.End (); ++it)
	{
		std::ostringstream prefix;
		prefix << "/unibe/videos/video" << dis(gen) << ".mpd";

		clientHelper.SetAttribute("MpdFileToRequest", StringValue(prefix.str()));
		ApplicationContainer app = clientHelper.Install(*it);
		uint64_t startTime = 500 + (rand() % 100);
		NS_LOG_UNCOND("Delay time for client " << (*it)->GetId() << " is " << startTime);
		app.Start(MilliSeconds(startTime));
	}

	// Producer(s)
	AppHelper producerHelper("ns3::ndn::FakeMultimediaServerNetworkCoding");
	
	producerHelper.SetAttribute("MetaDataFile", StringValue("data/multimedia/representations/netflix.csv"));
	producerHelper.SetPrefix("/unibe/videos");
	producerHelper.SetAttribute("VideoName", StringValue("video"));
	producerHelper.SetAttribute("NumberOfVideos", UintegerValue(VIDEOS));

	producerHelper.Install(sources);

    // Installing global routing interface on all nodes
	GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();
  	ndnGlobalRoutingHelper.AddOrigins("/unibe", sources);

	// Intalling Tracers
	std::string tracer_path = std::string("results/generated/" + std::to_string(runId) + "/netcodndn/");

    boost::filesystem::path dir(tracer_path.c_str());
    if(boost::filesystem::create_directories(dir))
    {
		NS_LOG_UNCOND("Directory Created: " + tracer_path);
    }
	L3RateTracer::InstallAll(tracer_path + "l3-rate-trace.txt", Seconds(1.0));
	//FileConsumerLogTracer::Install(Names::Find<Node>("SE-C002"), "results/star/netcod/file-consumer-log-trace.txt");
	DASHPlayerTracer::InstallAll(tracer_path + "dash-trace.txt");

	Simulator::Stop(Seconds(200.0));

	Simulator::Run();
	Simulator::Destroy();

	NS_LOG_UNCOND("Simulation Finished.");

	return 0;
}
