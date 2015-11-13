#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/sbra-wifi-manager.h"
#include "ns3/fb-headers.h"
#include "ns3/adhoc-wifi-mac.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MULTICAST_TEST");

uint32_t data = 0;
uint32_t rxdrop = 0;
uint32_t rxnum = 0;

double txtime = 0;
double rxtime = 0;
double idletime = 0;
double ccatime = 0;
double switchtime = 0;

static void
	StateLog (std::string context, Time start, Time duration,enum WifiPhy::State state ){

		if(Simulator::Now ().GetSeconds () >= 0){
			if(state==WifiPhy::TX)
				txtime+=duration.GetMicroSeconds();
			else if (state==WifiPhy::RX)
				rxtime+=duration.GetMicroSeconds();
			else if (state==WifiPhy::CCA_BUSY)
				ccatime+=duration.GetMicroSeconds();
			else if (state==WifiPhy::SWITCHING)
				switchtime+=duration.GetMicroSeconds();
			else if (state==WifiPhy::IDLE)
				idletime+=duration.GetMicroSeconds();
		}
}

static void 
RxNum(Ptr<const Packet> pkt)
{
	if(pkt->GetSize () >= 1000)
		rxnum++;
}
static void 
RxDrop(Ptr<const Packet> pkt )
{
	if(pkt->GetSize () >= 1000)
		rxdrop++;
}
static void 
RxData(Ptr <const Packet> pkt, const Address &a)
{
	if(pkt->GetSize () >= 1000)
		data += pkt->GetSize (); 
}

int
main (int argc, char *argv[])
{
	uint32_t txNodeNum = 1;
	uint32_t rxNodeNum = 1;
	uint32_t seed = 1; // 1:1:100 
	uint32_t rateAdaptType = 0; // 0 or 1 0-> per over 0.001 1-> maximun throughput 
	uint32_t feedbackType = 0; // 0, 1, 2
	uint64_t feedbackPeriod = 200; // MilliSeconds
	double dopplerVelocity = 0.1; // 0.5:0.5:2
	double bound = 20.0; 
	double endTime = 20;
	double perThreshold = 0.001;
	
	// feedbackType 0
	double percentile = 0.9; // [0, 1]
	// feedbackType 1
	double alpha = 0.5; // 0.1 0.3 0.5 0.7 0.9
	// feedbackType 2
	double beta = 0.5; // 0 0.5 1.0
	// feedbackType 3
	double eta = 1.0;
	double delta = 0.1;
	double rho = 0.1;
	
	CommandLine cmd;
	cmd.AddValue ("TxNodeNum", "Number of tx nodes", txNodeNum);
	cmd.AddValue ("RxNodeNum", "Number of rx nodes", rxNodeNum);
	cmd.AddValue ("Seed", "Seed of simulation", seed);
	cmd.AddValue ("RateAdaptType", "Type of rate adaptation", rateAdaptType);
	cmd.AddValue ("FeedbackPeriod", "Period of feedback", feedbackPeriod);
	cmd.AddValue ("Doppler", "Doppler Velocity", dopplerVelocity);
	cmd.AddValue ("Bound", "Rectangular bound of topology", bound);
	cmd.AddValue ("EndTime", "Simulator runtime", endTime);
	cmd.AddValue ("PerThreshold", "threshold of per", perThreshold);
	cmd.AddValue ("FeedbackType", "Type of rssi feedback", feedbackType);
	cmd.AddValue ("Percentile", "percentile of rssi", percentile);
	cmd.AddValue ("Alpha", "Exponential Weighting Moving Average factor", alpha);
	cmd.AddValue ("Beta", "Weighting factor of stddev", beta);
	cmd.AddValue ("Eta", "Weighting factor of SNR", eta);
	cmd.AddValue ("Delta", "Weighting factor of SNR and deviation", delta);
	cmd.AddValue ("Rho", "weighting factor of deviation", rho);
	cmd.Parse (argc, argv);
	
	SeedManager::SetRun(seed);

	/*NS_LOG_UNCOND
		("seed: " << seed 
		 << " rateAdaptType: " << rateAdaptType << " feedbackPeriod: " << feedbackPeriod <<	" doppler: " << dopplerVelocity << " bound: " << bound
		 <<	" feedbackType: " << feedbackType << " percentile: " << percentile << " alpha: " << alpha << " beta: " << beta);*/
	NodeContainer txNodes, rxNodes;
	txNodes.Create (txNodeNum);
	rxNodes.Create (rxNodeNum);

	WifiHelper wifi = WifiHelper::Default();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	std::string rateControl("ns3::SbraWifiManager");
	wifi.SetRemoteStationManager (rateControl);
	
	YansWifiChannelHelper wifiChannel; 
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel"); 
	wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue(3.5));

	double dopplerFrq = dopplerVelocity*50/3; 
	wifiChannel.AddPropagationLoss("ns3::JakesPropagationLossModel");
	Config::SetDefault ("ns3::JakesProcess::DopplerFrequencyHz", DoubleValue (dopplerFrq));
	// SbraWifiManger
	Config::SetDefault ("ns3::SbraWifiManager::Type", UintegerValue (rateAdaptType));
	Config::SetDefault ("ns3::SbraWifiManager::PerThreshold", DoubleValue (perThreshold));
	// AdhocWifiMac
	Config::SetDefault ("ns3::AdhocWifiMac::FeedbackType", UintegerValue (feedbackType));
	Config::SetDefault ("ns3::AdhocWifiMac::FeedbackPeriod", UintegerValue (feedbackPeriod));
	Config::SetDefault ("ns3::AdhocWifiMac::Alpha", DoubleValue (alpha));
	Config::SetDefault ("ns3::AdhocWifiMac::Beta", DoubleValue (beta));
	Config::SetDefault ("ns3::AdhocWifiMac::Percentile", DoubleValue (percentile));
	Config::SetDefault ("ns3::AdhocWifiMac::Eta", DoubleValue (eta));
	Config::SetDefault ("ns3::AdhocWifiMac::Delta", DoubleValue (delta));
	Config::SetDefault ("ns3::AdhocWifiMac::Rho", DoubleValue (rho));
	
	wifiPhy.SetChannel (wifiChannel.Create ());
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default(); 

	Ssid ssid = Ssid ("wifi-default"); 
	wifiMac.SetType ("ns3::AdhocWifiMac", "Ssid", SsidValue (ssid)); 
	NetDeviceContainer txDevice = wifi.Install (wifiPhy, wifiMac, txNodes);

	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
	NetDeviceContainer rxDevice = wifi.Install (wifiPhy, wifiMac, rxNodes);

	MobilityHelper txMobility, rxMobility; 
	txMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	txMobility.SetPositionAllocator (positionAlloc);
  txMobility.Install (txNodes.Get (0));

  std::stringstream DiscRho;
	DiscRho << "ns3::UniformRandomVariable[Min=" << bound << "|Max=" << bound << "]";	
	rxMobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
			"X", StringValue ("0.0"),
			"Y", StringValue ("0.0"),
			"Rho", StringValue (DiscRho.str() ));
	rxMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rxMobility.Install (rxNodes);
	
	InternetStackHelper stack;
	stack.Install (txNodes);
	stack.Install (rxNodes);
  
  Ipv4AddressHelper ipv4Addr;
	ipv4Addr.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4Addr.Assign (txDevice);
  ipv4Addr.Assign (rxDevice);

  Ipv4Address multicastSource ("10.1.1.1");
  Ipv4Address multicastGroup ("224.100.100.1");

  Ipv4StaticRoutingHelper multicast;
	
	Ptr<Node> sender = txNodes.Get (0);
	Ptr<NetDevice> senderIf = txDevice.Get (0);
	multicast.SetDefaultMulticastRoute (sender, senderIf);
		
	uint16_t multicastPort = 9;

	OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (multicastGroup, multicastPort)));
	onoff.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	onoff.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	onoff.SetAttribute ("PacketSize", UintegerValue(1000));
	onoff.SetAttribute ("DataRate", StringValue ("2Mbps"));

	ApplicationContainer txApp = onoff.Install (txNodes.Get (0));

	txApp.Start (Seconds (0.0));
	txApp.Stop (Seconds (endTime));

	PacketSinkHelper sink ("ns3::UdpSocketFactory",	InetSocketAddress (Ipv4Address::GetAny (), multicastPort));
	
	ApplicationContainer rxApp = sink.Install (rxNodes);
	rxApp.Start (Seconds (0.0));
	rxApp.Stop (Seconds (endTime));
	rxApp.Get (0)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxData));
	rxDevice.Get(0)->GetObject<WifiNetDevice>()->GetPhy()->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));
	rxDevice.Get(0)->GetObject<WifiNetDevice>()->GetPhy()->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&RxNum));
	
	//wifiPhy.EnablePcapAll ("multicast-test");

	std::ostringstream path1;
	path1 << "/NodeList/" << txNodes.Get (0) -> GetId () << "/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/State/$ns3::WifiPhyStateHelper/State";
	Config::Connect (path1.str (), MakeCallback (&StateLog));
	
	Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
	
	std::ofstream fout;
	std::ostringstream out_filename;
	
	switch (feedbackType)
	{
		case 0:
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " percentile: " << percentile << " bound: " << bound);
			out_filename << "storage_results/result_151111/per_" << percentile << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
			break;
		case 1:
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " alpha: " << alpha << " bound: " << bound);
			out_filename << "storage_results/result_151111/alp_" <<   alpha   << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
			break;
		case 2:
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " beta: " << beta << " bound: " << bound);
			out_filename << "storage_results/result_151111/bet_" <<   beta   << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
			break;
		case 3: 
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " eta: " << eta << " delta: " << delta << " rho: " << rho <<  " bound: " << bound);
			out_filename << "storage_results/result_edr_151111/edr_" << eta << "_" << delta << "_" << rho  << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
	}

	fout.open(out_filename.str().c_str(), std::ostream::out);
	if(!fout.good())
		NS_LOG_UNCOND("File open failed");
	
	//fout << "multicastrateadapt_seed" << seed << "_rateAdaptType" << rateAdaptType << "_feedbackPeriod" << feedbackPeriod <<  "_doppler" << dopplerVelocity << "_bound" << bound << 
	//	" feedbackType: " << feedbackType << " percentile: " << percentile << " alpha: " << alpha << " beta: " << beta << std::endl;
	
	Ptr<OnOffApplication> onof = txApp.Get(0)->GetObject<OnOffApplication> ();
	NS_LOG_UNCOND("Source node sent: " << (onof->GetTotalTx()/1000));
	fout << "Source node sent: " << (onof->GetTotalTx()/1000) << std::endl;

	for (uint32_t i=0; i<rxApp.GetN(); i++)
	{
		Ptr<PacketSink> sink2 = rxApp.Get(i)->GetObject<PacketSink> ();
		NS_LOG_UNCOND("Node " << i+1 << " received: " << sink2->GetTotalRx()/1000);
		fout << "Node " << i+1 << " received: " << sink2->GetTotalRx()/1000 << std::endl;
	}

	double totaltime=txtime + rxtime + idletime + ccatime + switchtime;
	double airtime = (totaltime - idletime)/totaltime;

	Ptr<WifiNetDevice> txNetDevice = txDevice.Get(0)->GetObject<WifiNetDevice> ();
	Ptr<WifiMac> txMac = txNetDevice->GetMac ();
	Ptr<RegularWifiMac> txRegMac = DynamicCast<RegularWifiMac> (txMac);
	Ptr<SbraWifiManager> sbraWifi = DynamicCast<SbraWifiManager>(txRegMac->GetWifiRemoteStationManager());

	NS_LOG_UNCOND("AirTime: " << airtime);
	NS_LOG_UNCOND("AvgMinSnr(dB): " <<sbraWifi->GetAvgMinSnrDb ());
	NS_LOG_UNCOND("AvgTxMode(Mb/s): " <<sbraWifi->GetAvgTxMode ());
	NS_LOG_UNCOND("AvgTxMcs: " <<sbraWifi->GetAvgTxMcs ());
	
	fout << "AirTime: " << airtime << std::endl;
	fout << "AvgMinSnr: " << sbraWifi->GetAvgMinSnrDb () << std::endl;
	fout.close();

	Simulator::Destroy ();
	NS_LOG_INFO("Throughput: "<< (double)data*8/1000/1000/endTime << " Mbps");
	NS_LOG_INFO("rxdrop: "<<rxdrop);
	NS_LOG_INFO("rxnum: "<<rxnum);
	NS_LOG_INFO("PER: "<<(double)(rxdrop)/(double)(rxnum+rxdrop));
}
