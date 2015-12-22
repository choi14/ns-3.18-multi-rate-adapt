#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/sbra-wifi-manager.h"
#include "ns3/fb-headers.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/mobility-model.h"
#include <vector>

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

double start_time = 30;

static void StartTl(Ptr<AdhocWifiMac> mac)
{
	 mac->SetBasicModes();
	 mac->SendTraining();
}

static void
	StateLog (std::string context, Time start, Time duration,enum WifiPhy::State state ){

		if(Simulator::Now ().GetSeconds () >= start_time + 1){
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
	uint32_t seed = 0; // 1:1:100 
	uint32_t rateAdaptType = 0; // 0, 1, 2 (0 = n*t) 
	uint32_t feedbackType = 4; // 0, 1, 2, 3, 4
	//uint32_t nc_k = 10;	
	uint64_t feedbackPeriod = 1000; // MilliSeconds
	double dopplerVelocity = 0.1; // 0.5:0.5:2
	double bound = 10.0; 
	double endTime = 100;
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
	//EDR type
	uint32_t edrType = 0;
	uint32_t linearTime = 2000;

	//blockSize
	uint16_t blockSize = 5;

  //double getTotalTx = 0.0;
	double sumTotalRx = 0.0;
	std::vector<double> getTotalRx;
	getTotalRx.clear ();
	
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
	cmd.AddValue ("EDRtype", "Type of EDR Calculation", edrType);
	cmd.AddValue ("LinearTime", "Time of Linear Decreasing/Increasing", linearTime);
	cmd.AddValue ("BlockSize", "Size of interleaving block", blockSize);
	cmd.Parse (argc, argv);
	
	SeedManager::SetRun(seed);

	/*
	Config::SetDefault ("ns3::YansWifiPhy::CcaMode1Threshold", DoubleValue (-200));
	Config::SetDefault ("ns3::YansWifiPhy::EnergyDetectionThreshold", DoubleValue (-200));
	Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (4000));
	*/

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
	// Adhoc-wifi-mac.cc
	Config::SetDefault ("ns3::AdhocWifiMac::RateAdaptType", UintegerValue (rateAdaptType));
	Config::SetDefault ("ns3::AdhocWifiMac::PerThreshold", DoubleValue (perThreshold));
	Config::SetDefault ("ns3::AdhocWifiMac::FeedbackType", UintegerValue (feedbackType));
	Config::SetDefault ("ns3::AdhocWifiMac::FeedbackPeriod", UintegerValue (feedbackPeriod));
	Config::SetDefault ("ns3::AdhocWifiMac::Alpha", DoubleValue (alpha));
	Config::SetDefault ("ns3::AdhocWifiMac::Beta", DoubleValue (beta));
	Config::SetDefault ("ns3::AdhocWifiMac::Percentile", DoubleValue (percentile));
	Config::SetDefault ("ns3::AdhocWifiMac::Eta", DoubleValue (eta));
	Config::SetDefault ("ns3::AdhocWifiMac::Delta", DoubleValue (delta));
	Config::SetDefault ("ns3::AdhocWifiMac::Rho", DoubleValue (rho));
	Config::SetDefault ("ns3::AdhocWifiMac::BlockSize", UintegerValue (blockSize));
	// Mac-low.cc
	Config::SetDefault ("ns3::MacLow::EDRtype", UintegerValue (edrType));
	Config::SetDefault ("ns3::MacLow::LinearTime", UintegerValue (linearTime));
	
	wifiPhy.SetChannel (wifiChannel.Create ());
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default(); 

	Ssid ssid = Ssid ("wifi-default"); 
	wifiMac.SetType ("ns3::AdhocWifiMac", "Ssid", SsidValue (ssid)); 
	NetDeviceContainer txDevice = wifi.Install (wifiPhy, wifiMac, txNodes);

	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
	NetDeviceContainer rxDevice = wifi.Install (wifiPhy, wifiMac, rxNodes);

	NetDeviceContainer allDevice(txDevice, rxDevice);
	MobilityHelper txMobility, rxMobility; 
	txMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	txMobility.SetPositionAllocator (positionAlloc);
  txMobility.Install (txNodes.Get (0));

  /*	
  std::stringstream DiscRho;
	DiscRho << "ns3::UniformRandomVariable[Min=" << bound << "|Max=" << bound << "]";	
	rxMobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
			"X", StringValue ("0.0"),
			"Y", StringValue ("0.0"),
			"Rho", StringValue (DiscRho.str() ));
	rxMobility.Install (rxNodes);
	*/

	std::stringstream Rectangle;
	Rectangle << "ns3::UniformRandomVariable[Min=" << -0.5*bound << "|Max=" << 0.5*bound << "]";
	rxMobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
			"X", StringValue (Rectangle.str ()),
			"Y", StringValue (Rectangle.str ()));
	rxMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  rxMobility.Install (rxNodes);
	
	/*
	for (uint32_t i=0; i < rxNodes.GetN(); i++)
	{
		std::cout <<	 rxNodes.Get(i)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
	}
*/
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

	txApp.Start (Seconds (start_time));
	txApp.Stop (Seconds (start_time+endTime));

	PacketSinkHelper sink ("ns3::UdpSocketFactory",	InetSocketAddress (Ipv4Address::GetAny (), multicastPort));
	
	ApplicationContainer rxApp = sink.Install (rxNodes);
	rxApp.Start (Seconds (start_time));
	rxApp.Stop (Seconds (start_time+endTime+0.1));
	rxApp.Get (0)->TraceConnectWithoutContext ("Rx", MakeCallback (&RxData));
	rxDevice.Get(0)->GetObject<WifiNetDevice>()->GetPhy()->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));
	rxDevice.Get(0)->GetObject<WifiNetDevice>()->GetPhy()->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&RxNum));
		
	for (uint32_t i=0; i<allDevice.GetN(); i++){
  		Ptr<WifiNetDevice> dev = allDevice.Get(i)->GetObject<WifiNetDevice> ();
		Ptr<AdhocWifiMac> mac = dev->GetMac()->GetObject<AdhocWifiMac> ();
		mac->SetAid(i);
		Simulator::Schedule (Seconds (i*2), &StartTl, mac);
	}

	//wifiPhy.EnablePcapAll ("multicast-test");

	std::ostringstream path1;
	path1 << "/NodeList/" << txNodes.Get (0) -> GetId () << "/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/State/$ns3::WifiPhyStateHelper/State";
	Config::Connect (path1.str (), MakeCallback (&StateLog));
	
	Simulator::Stop (Seconds (start_time+endTime+0.1));
  Simulator::Run ();
	
	std::ofstream fout;
	std::ostringstream out_filename;
	
	switch (feedbackType)
	{
		case 0:
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " percentile: " << percentile << " bound: " << bound << " rxNodeNum: " << rxNodeNum);
			out_filename << "storage_results/result_151201/per_" << percentile << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
			break;
		case 1:
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " alpha: " << alpha << " bound: " << bound << " rxNodeNum: " << rxNodeNum);
			out_filename << "storage_results/result_151201/alp_" <<   alpha   << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
			break;
		case 2:
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " beta: " << beta << " bound: " << bound << " rxNodeNum: " << rxNodeNum);
			out_filename << "storage_results/result_151201/bet_" <<   beta   << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << ".txt";
			break;
		case 3: 
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " eta: " << eta << " delta: " << delta << " rho: " << rho <<  " bound: " << bound << " rxNodeNum: " << rxNodeNum << " edrType: " << edrType);
			out_filename << "storage_results/result_edr3_151222/edr_" << eta << "_" << delta << "_" << rho  << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << "_" << edrType << "_" << linearTime << "_" << blockSize << ".txt";
			break;
		case 4: 
			NS_LOG_UNCOND ("seed: " << seed << " feedbackPeriod: " << feedbackPeriod <<	" dopplerVelocity: " << dopplerVelocity <<
					" feedbackType: " << feedbackType << " eta: " << eta << " delta: " << delta << " rho: " << rho <<  " bound: " << bound << " rxNodeNum: " << rxNodeNum << " edrType: " << edrType);
			out_filename << "storage_results/result_edr4_151222/edr_" << eta << "_" << delta << "_" << rho  << "_" << seed << "_" << rxNodeNum <<  "_" << feedbackPeriod << "_" << dopplerVelocity  << "_" << bound << "_" << edrType << "_" << linearTime << "_" << blockSize << ".txt";
			break;
		default:
			NS_LOG_UNCOND("Invalid type");
	}

	fout.open(out_filename.str().c_str(), std::ostream::out);
	if(!fout.good())
		NS_LOG_UNCOND("File open failed " << out_filename.str());
	
	//fout << "multicastrateadapt_seed" << seed << "_rateAdaptType" << rateAdaptType << "_feedbackPeriod" << feedbackPeriod <<  "_doppler" << dopplerVelocity << "_bound" << bound << 
	//	" feedbackType: " << feedbackType << " percentile: " << percentile << " alpha: " << alpha << " beta: " << beta << std::endl;
	
	//Ptr<WifiNetDevice> txNetDevice = txDevice.Get(0)->GetObject<WifiNetDevice> ();
	//Ptr<WifiMac> txMac = txNetDevice->GetMac ();
	//Ptr<RegularWifiMac> txRegMac = DynamicCast<RegularWifiMac> (txMac);
	//Ptr<SbraWifiManager> sbraWifi = DynamicCast<SbraWifiManager>(txRegMac->GetWifiRemoteStationManager());

	// Number of tx packet from source node before network condig
	Ptr<WifiNetDevice> txNetDevice = txDevice.Get(0)->GetObject<WifiNetDevice> ();
	Ptr<WifiMac> txMac = txNetDevice->GetMac ();
	Ptr<AdhocWifiMac> txAdhocMac = DynamicCast<AdhocWifiMac> (txMac);
	uint32_t txNum = txAdhocMac->GetTxNum ();
	NS_LOG_UNCOND("Source node sent (AdhocWifiMac): " << txNum);
	
	Ptr<WifiNetDevice> rxNetDevice = rxDevice.Get(0)->GetObject<WifiNetDevice> ();
	Ptr<WifiMac> rxMac = rxNetDevice->GetMac ();
	Ptr<AdhocWifiMac> rxAdhocMac = DynamicCast<AdhocWifiMac> (rxMac);
	//uint32_t rxNum = rxAdhocMac->GetRxNum ();
	//NS_LOG_UNCOND("Node1 received (AdhocWifiMac): " << rxNum);
	//NS_LOG_UNCOND("AvgPer: " << 1 - (double)rxNum/(double)txNum);
	
	fout << "Source node sent: " << txNum << std::endl;
	
	/*
	Ptr<OnOffApplication> onof = txApp.Get(0)->GetObject<OnOffApplication> ();
	getTotalTx = nc_k*(onof->GetTotalTx ()/1000/nc_k);
	NS_LOG_UNCOND("Source node sent: " << getTotalTx);
	*/

	for (uint32_t i = 0; i < rxApp.GetN(); i++)
	{
		Ptr<PacketSink> sink2 = rxApp.Get(i)->GetObject<PacketSink> ();
		getTotalRx.push_back(sink2->GetTotalRx ()/1000);
		NS_LOG_UNCOND("Node " << i+1 << " received: " << getTotalRx[i]);
		fout << "Node " << i+1 << " received: " << getTotalRx[i] << std::endl;
		sumTotalRx += getTotalRx[i];
	}
	double totaltime = txtime + rxtime + idletime + ccatime + switchtime;
	double airtime = (totaltime - idletime)/totaltime;
	fout << "AirTime: " << airtime << std::endl;
	fout.close();
	NS_LOG_UNCOND("AirTime: " << airtime);
	NS_LOG_UNCOND("AvgPer: " << 1 - sumTotalRx/(txNum*rxNodeNum));

	Simulator::Destroy ();
	//NS_LOG_INFO("Throughput: "<< (double)data*8/1000/1000/endTime << " Mbps");
	//NS_LOG_INFO("rxdrop: "<<rxdrop);
	//NS_LOG_INFO("rxnum: "<<rxnum);
	//NS_LOG_INFO("PER: "<<(double)(rxdrop)/(double)(rxnum+rxdrop));
}
