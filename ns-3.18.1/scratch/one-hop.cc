#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

uint32_t data1=0;
uint32_t data2=0;

static void
RxCnt1 (Ptr<const Packet> p, const Address &a)
{
  data1 += p->GetSize();
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << data);
}
	
static void
RxCnt2 (Ptr<const Packet> p, const Address &a)
{
  data2 = data2 + p->GetSize();
}

int main (int argc, char *argv[])
{
		  
		int nSta = 4;
		
		CommandLine cmd;
	  cmd.AddValue("nSTA", "number of stations", nSta);
		cmd.Parse (argc, argv);
		
		NodeContainer StaNodes;
		StaNodes.Create (nSta);
									     
		WifiHelper wifi;
		wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
		
		YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
		wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
     
		YansWifiChannelHelper wifiChannel ;
		wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
		wifiChannel.AddPropagationLoss ("ns3::JakesPropagationLossModel");
		wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue(3.5));
		Config::SetDefault ("ns3::JakesProcess::DopplerFrequencyHz", DoubleValue (8.58)); // 0.5m/s
		//Config::SetDefault ("ns3::JakesProcess::DopplerFrequencyHz", DoubleValue (1.72)); // 0.1m/s

		wifiPhy.SetChannel (wifiChannel.Create ());
		
		//std::string rateControl("ns3::ArfWifiManager");
	  //std::string rateControl("ns3::IdealWifiManager");
	  //std::string rateControl("ns3::SbraWifiManager");
    std::string rateControl("ns3::ConstantRateWifiManager");



		NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
		//wifi.SetRemoteStationManager (rateControl, "DataMode", StringValue("OfdmRate9Mbps"));

					  
		
		Ssid ssid = Ssid ("wifi-default");

		wifiMac.SetType ("ns3::AdhocWifiMac",
						"Ssid", SsidValue (ssid));

		NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, StaNodes);
		
		MobilityHelper mobility;
		
		Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
		positionAlloc->Add (Vector (-10.0, 0.0, 0.0));
		positionAlloc->Add (Vector (45.0, 0.0, 0.0));
		positionAlloc->Add (Vector (-45.0, 0.0, 0.0));
		positionAlloc->Add (Vector (30.0, 0.0, 0.0));

		mobility.SetPositionAllocator (positionAlloc);
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (StaNodes);

		InternetStackHelper internet;
		internet.Install (StaNodes);
		
		Ipv4AddressHelper ipv4;
		ipv4.SetBase ("10.1.1.0", "255.255.255.0");
		ipv4.Assign (staDevice);

		// node0, node2
		uint16_t port = 9;
		
		PacketSinkHelper sink ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
		
		ApplicationContainer app;
		app = sink.Install (StaNodes.Get (0));
		app.Start (Seconds (0.0));
		app.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback(&RxCnt1));

		
		OnOffHelper onoff ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address ("10.1.1.1"), port)));
						
		onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
		onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
		onoff.SetAttribute ("DataRate", StringValue ("30Mbps"));
		onoff.SetAttribute ("PacketSize", UintegerValue (1460));
										    
		app = onoff.Install (StaNodes.Get (2));
		app.Start (Seconds (1.0));
		app.Stop (Seconds (2.0));

		// node1, node3
		uint16_t port2 = 8080;
		
		PacketSinkHelper sink2 ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address::GetAny (), port2)));
		
		app = sink2.Install (StaNodes.Get (1));
		app.Start (Seconds (0.0));
		app.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback(&RxCnt2));

		OnOffHelper onoff2 ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address ("10.1.1.2"), port2)));
						
		onoff2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
		onoff2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
		onoff2.SetAttribute ("DataRate", StringValue ("30Mbps"));
		onoff2.SetAttribute ("PacketSize", UintegerValue (1460));
										    
		app = onoff2.Install (StaNodes.Get (3));
		app.Start (Seconds (1.0));
		app.Stop (Seconds (2.0));
		
		wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	 	//wifiPhy.EnablePcap ("one-hop", staDevice);

		Simulator::Stop (Seconds (2.0));
		Simulator::Run ();
		Simulator::Destroy ();

		NS_LOG_UNCOND( rateControl << " >> Node0 Throughput: "<< (double)data1*8/1000/1000/9 << " Mbps");
		NS_LOG_UNCOND( rateControl << " >> Node1 Throughput: "<< (double)data2*8/1000/1000/9 << " Mbps");
		return 0;
}

