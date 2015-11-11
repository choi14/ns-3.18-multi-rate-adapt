#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

uint32_t data=0;

static void
RxCnt (Ptr<const Packet> p, const Address &a)
{
  data = data + p->GetSize();
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << data);
}
	

int main (int argc, char *argv[])
{
		  
		int nSta = 1;
		
		CommandLine cmd;
	  cmd.AddValue("nSTA", "number of stations", nSta);
		cmd.Parse (argc, argv);

		NodeContainer ApNode;
		ApNode.Create (1);
		
		NodeContainer StaNodes;
		StaNodes.Create (nSta);
		NodeContainer c;
		c.Add (ApNode);
		c.Add (StaNodes);


									     
		WifiHelper wifi;
		wifi.SetStandard (WIFI_PHY_STANDARD_80211a);

		
		YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
		wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

     
		YansWifiChannelHelper wifiChannel ;
		wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
		wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
		wifiPhy.SetChannel (wifiChannel.Create ());

	
		//std::string rateControl("ns3::ArfWifiManager");
		std::string rateControl("ns3::ConstantRateWifiManager");



		NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
		//wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
		wifi.SetRemoteStationManager (rateControl);
					  
		
		Ssid ssid = Ssid ("wifi-default");

		wifiMac.SetType ("ns3::StaWifiMac",
						"Ssid", SsidValue (ssid),
						"ActiveProbing", BooleanValue (false));

		NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, StaNodes);
	 
		wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
		NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ApNode.Get(0));
			   
		
		NetDeviceContainer devices;
		devices.Add (apDevice);
		devices.Add (staDevice);
		
		MobilityHelper mobility;
		
		Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
		positionAlloc->Add (Vector (0.0, 0.0, 0.0));
		positionAlloc->Add (Vector (30.0, 0.0, 0.0));
		positionAlloc->Add (Vector (0.0, 30.0, 0.0));
		positionAlloc->Add (Vector (-30.0, 0.0, 0.0));
		positionAlloc->Add (Vector (0.0, -30.0, 0.0));
		positionAlloc->Add (Vector (0.0, 0.0, 30.0));

		mobility.SetPositionAllocator (positionAlloc);
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (c);

		   
		InternetStackHelper internet;
		internet.Install (c);
		Ipv4AddressHelper ipv4;

		
		ipv4.SetBase ("10.1.1.0", "255.255.255.0");
		ipv4.Assign (devices);

		
		uint16_t port = 9;
	
		//PacketSinkHelper sink ("ns3::UdpSocketFactory",
		//				Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
		Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(560));
		PacketSinkHelper sink ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
		
		ApplicationContainer app;
		
		app = sink.Install (ApNode.Get (0));
		app.Start (Seconds (0.0));
		app.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback(&RxCnt));

		
		OnOffHelper onoff ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address ("10.1.1.1"), port)));
						
		onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
		onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
		onoff.SetAttribute ("DataRate", StringValue ("5Mbps"));
		onoff.SetAttribute ("PacketSize", UintegerValue (1460));
										    

		for(int i=0; i<nSta; i++)
		{
				app = onoff.Install (StaNodes.Get (i));
				app.Start (Seconds (1.0));
				app.Stop (Seconds (10.0));
		}

		wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	 	wifiPhy.EnablePcap ("session6_ex1", devices);

		Simulator::Stop (Seconds (10.0));
		Simulator::Run ();
		Simulator::Destroy ();

		NS_LOG_UNCOND( rateControl << " >> Number of STAs= " << nSta<< " Aggregated Throughput: "<< (double)data*8/1000/1000/9 << " Mbps");
		return 0;
}

