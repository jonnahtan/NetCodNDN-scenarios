#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

// Custom Strategies
#include "random-load-balancer-strategy.hpp"
#include "weighted-load-balancer-strategy.hpp"
#include "random-load-balancer-strategy_no-aggregation.hpp"
#include <boost/filesystem.hpp>

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

NS_LOG_COMPONENT_DEFINE ("File_Traditional_L");

#define _LOG_INFO(x) NS_LOG_INFO(x)

int
main(int argc, char* argv[])
{
	int runId = 1;
	CommandLine cmd;
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

	// Choosing forwarding strategy
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(sources, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(routers, "/unibe");
	StrategyChoiceHelper::Install<nfd::fw::WeightedLoadBalancerStrategy>(clients, "/unibe");

	// Consumer(s)
	ns3::ndn::AppHelper clientHelper("ns3::ndn::FileConsumerWindow::MultimediaConsumer");
	clientHelper.SetAttribute("AllowUpscale", BooleanValue(true));
	clientHelper.SetAttribute("AllowDownscale", BooleanValue(false));
	clientHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
	clientHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
	clientHelper.SetAttribute("StartRepresentationId", StringValue("lowest"));
	clientHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
	clientHelper.SetAttribute("StartUpDelay", StringValue("2.0"));

  	//clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
	clientHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::DASHJSAdaptationLogic"));
  	clientHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/unibe/videos/video1.mpd" )));

	clientHelper.SetAttribute("LifeTime", StringValue("1000ms"));

  	//consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));

	// Install consumers with random start times to randomize the seeds
	srand (time(NULL));
	for (NodeContainer::Iterator it = clients.Begin (); it != clients.End (); ++it)
	{
		ApplicationContainer app = clientHelper.Install(*it);
		uint64_t startTime = 500 + (rand() % 100);
		NS_LOG_UNCOND("Delay time for client " << (*it)->GetId() << " is " << startTime);
		app.Start(MilliSeconds(startTime));
	}

	// Producer(s)
 	AppHelper producerHelper("ns3::ndn::FakeMultimediaServer");
 	producerHelper.SetPrefix("/unibe/videos");
	producerHelper.SetAttribute("MetaDataFile", StringValue("data/csv/netflix.csv"));
	producerHelper.SetAttribute("MPDFileName", StringValue("video1.mpd"));
  	producerHelper.Install(sources); // install to some node from nodelist
	  
    // Installing global routing interface on all nodes
	GlobalRoutingHelper ndnGlobalRoutingHelper;
	ndnGlobalRoutingHelper.InstallAll();
  	ndnGlobalRoutingHelper.AddOrigins("/unibe", sources);

	// Intalling Tracers
	std::string tracer_path = std::string("results/generated/" + std::to_string(runId) + "/ndn/");

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
