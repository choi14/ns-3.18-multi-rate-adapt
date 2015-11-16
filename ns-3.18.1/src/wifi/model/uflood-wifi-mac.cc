/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "uflood-wifi-mac.h"

#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"

#include "qos-tag.h"
#include "mac-low.h"
#include "dcf-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "msdu-aggregator.h"
#include "amsdu-subframe-header.h"
#include "wifi-net-device.h"

#include "ns3/llc-snap-header.h" // GJLEE
#include "mgt-headers.h"
#include <stdio.h>
#include <iostream>
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-variable.h"

//Newly defined headers
#include "tl-header.h"
#include "topology-result.h"
//#include "MNC_codec.h"
#include "rate-tag.h"
#include "coefficient-header.h"
#include "uflood-data-header.h"

#include "ns3/basic-energy-source.h"
#include "ns3/energy-source-container.h"
#include "ns3/energy-source.h"
#include "ns3/device-energy-model.h"
//#include "result.h"

NS_LOG_COMPONENT_DEFINE ("UfloodWifiMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (UfloodWifiMac);

TypeId
UfloodWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UfloodWifiMac")
    .SetParent<RegularWifiMac> ()
    .AddConstructor<UfloodWifiMac> ()
    .AddAttribute ("Source", "Whether it is source or not",
		    BooleanValue(false),
		    MakeBooleanAccessor (&UfloodWifiMac::m_src),
		    MakeBooleanChecker ())
	.AddAttribute ("NumTraining", "Number of training packets",
			UintegerValue(1000),
			MakeUintegerAccessor (&UfloodWifiMac::m_max),
			MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("UtilityType", "UtilityType",
			UintegerValue(1),
			MakeUintegerAccessor (&UfloodWifiMac::m_utility_type),
			MakeUintegerChecker<uint8_t> ())
	.AddAttribute ("Charging", "Whether it is charging or not",
			BooleanValue(false),
			MakeBooleanAccessor (&UfloodWifiMac::m_charging),
			MakeBooleanChecker ()) // gjlee
	.AddAttribute ("Alpha", "Alpha",
			DoubleValue(0.5),
			MakeDoubleAccessor (&UfloodWifiMac::m_alpha),
			MakeDoubleChecker<double> ())
	.AddAttribute ("Omega", "Omega",
			DoubleValue(2),
			MakeDoubleAccessor (&UfloodWifiMac::m_omega),
			MakeDoubleChecker<double> ())
  ;
  return tid;
}

UfloodWifiMac::UfloodWifiMac ()
{
  NS_LOG_FUNCTION (this);

  // Let the lower layers know that we are acting in an IBSS
  SetTypeOfStation (ADHOC_STA);
  m_topo = CreateObject<TopologyResult> ();
  m_nodeinfo = CreateObject<NodeInfo> ();
  l_topo = TopologyResult();
  l_nodeinfo = NodeInfo ();
  
  cur_tf_addr = Mac48Address();
  src_addr = Mac48Address("00:00:00:00:00:01");
  m_rate = 0;
  m_MNC_K = 16;
  m_MNC_P = 1;
  m_queue = CreateObject<WifiMacQueue> ();
  m_doing = false;
  m_succ = true;
  m_eid = 0;
  m_burst = 0;
  m_id = 0;
  m_cumm_ack = 0;
  m_data_rate = 0;
  init_time = 0;
  total_time = 0;
  training_energy = 0;
 // m_init = false;

}

UfloodWifiMac::~UfloodWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
UfloodWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  // In an IBSS, the BSSID is supposed to be generated per Section
  // 11.1.3 of IEEE 802.11. We don't currently do this - instead we
  // make an IBSS STA a bit like an AP, with the BSSID for frames
  // transmitted by each STA set to that STA's address.
  //
  // This is why we're overriding this method.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}




void
UfloodWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  //packet->Print(std::cout);
  //std::cout << std::endl;
  if (m_stationManager->IsBrandNew (to))
    {
      // In ad hoc mode, we assume that every destination supports all
      // the rates we support.
		for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
        {
          m_stationManager->AddSupportedMode (to, m_phy->GetMode (i));
  		  m_stationManager->AddBasicMode(m_phy->GetMode (i));
        }
      m_stationManager->RecordDisassociated (to);
    }

  WifiMacHeader hdr;

  // If we are not a QoS STA then we definitely want to use AC_BE to
  // transmit the packet. A TID of zero will map to AC_BE (through \c
  // QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  // For now, a STA that supports QoS does not support non-QoS
  // associations, and vice versa. In future the STA model should fall
  // back to non-QoS if talking to a peer that is also non-QoS. At
  // that point there will need to be per-station QoS state maintained
  // by the association state machine, and consulted here.
  if (m_qosSupported)
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      // Transmission of multiple frames in the same TXOP is not
      // supported for now
      hdr.SetQosTxopLimit (0);

      // Fill in the QoS control field in the MAC header
      tid = QosUtilsGetTidForPacket (packet);
      // Any value greater than 7 is invalid and likely indicates that
      // the packet had no QoS tag, so we revert to zero, which'll
      // mean that AC_BE is used.
      if (tid >= 7)
        {
	  NS_LOG_DEBUG("invalid tid" << (uint16_t)tid);
          tid = 0;
        }
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetTypeData ();
    }

  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  if(tid!=5 || !IsSrc()){ // if(0): not used - skip

	  if (m_qosSupported)
	  {
		  // Sanity check that the TID is valid
		  //std::cout << "qos packet" << '\n'; //130819-jhkoo
		  NS_ASSERT (tid < 8);
		  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet,hdr);
	  }
	  else
	  {
		  //std::cout << "nqos packet" << '\n'; //130819-jhkoo
		  m_dca->Queue (packet, hdr);
	  }
  }
  // MNC coded adhoc-wifi-mac
  if(tid==5 && IsSrc() ){ // if(1) : always used
	  m_succ = true;
	  m_queue->Enqueue(packet, hdr);
	  
	  if(!m_doing && m_queue->GetSize() >= m_MNC_K)
	  {	  
		NS_LOG_DEBUG ("start new batch Eid: " << m_eid);

		if (m_eid == 0)
			init_time = Simulator::Now().GetSeconds();
		

		for (uint8_t i=0; i < m_MNC_K; i++){
			WifiMacHeader tempheader;
			Ptr<const Packet> temppacket = m_queue->Dequeue(&tempheader);
			WifiMacHeader tempheader2 = tempheader;
			Ptr<Packet> temp = temppacket->Copy();
	    	m_MNC_Encoder.MNC_Enqueue(temp, tempheader2);
		}
		uint8_t burstsize = GetBurstSize(1);
		NS_LOG_INFO("Burst size: " << (uint16_t)burstsize);
				
		//NS_LOG_INFO("Size before encoding: " << packet->GetSize());
		m_MNC_Encoder.Encoding(m_eid, m_MNC_K, burstsize, m_MNC_P, 0);
					
		FeedbackInterpolation(m_low->GetAddress(), burstsize, 0xffff, 1 );
		NeighborInfo nbr(GenerateNbrInfo());
		//nbr.Print(std::cout);
		
		Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
		uint16_t tmpId = (uint16_t) rand->GetInteger(0,0xffff);
				m_burst = (m_burst != tmpId ? tmpId : ~tmpId);
		
		for(uint8_t i = 0; i < burstsize; i++){ 
				//m_stationManager->SetMulticastModeIndex(j->rate);
				WifiMacHeader temphdr;
				Ptr<const Packet> encoded_packet = m_MNC_Encoder.MNC_Dequeue(&temphdr);
				Ptr<Packet> send_packet = encoded_packet->Copy();  
				
				//NS_LOG_INFO("Size after encoding: " << send_packet->GetSize());
				
				UfloodDataHeader u_hdr;
				u_hdr.SetEid(m_eid);
				u_hdr.SetRank(m_MNC_K);
				u_hdr.SetBitmap(0xffff);
				u_hdr.SetSend(burstsize);
				u_hdr.SetRemain(burstsize-i-1);
				u_hdr.SetBurst(m_burst);
				
				u_hdr.SetNeighbor (nbr);

				send_packet->AddHeader(u_hdr);

				RateTag tag(m_rate);
				ConstCast<Packet> (send_packet)->AddPacketTag(tag);
				if(m_qosSupported){
						// Sanity check that the TID is valid
						NS_ASSERT (tid < 8);
						NS_LOG_DEBUG(temphdr.GetAddr1() << "edca Queue");
						temphdr.SetWep(1);
						m_edca[QosUtilsMapTidToAc(tid)]->Queue(send_packet, temphdr);
				}
				else{
						NS_LOG_DEBUG("dca Queue");
						temphdr.SetWep(1);
						m_dca->Queue(send_packet, temphdr);
				}
		}
		Ptr<NodeInfo> node_src = l_topo.Find(l_nodeinfo.GetAddr());
		node_src->SetRank(m_MNC_K);
		node_src->SetBitmap(0xffff);
		node_src->SetInter(0);
		
		l_nodeinfo.SetRank(m_MNC_K);
		l_nodeinfo.SetBitmap(0xffff);
		l_nodeinfo.SetInter(0);
		
		
		m_MNC_Encoder.FlushN();
		m_doing = true;
		
		
		if (m_utilityEvent.IsRunning())
				m_utilityEvent.Cancel();
		double next_fb = CalcTxTime(m_rate, burstsize)+2000;
	 	m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);

	  }

	  else
	  {
		return;
	  }
  }
}

uint8_t
UfloodWifiMac::GetBurstSize(bool init){
		NS_LOG_FUNCTION(this);
		if (init){
				uint32_t max_rcv = 0;
				std::vector<Ptr<LinkInfo> > links = l_nodeinfo.GetInfo();						
				for (uint32_t i=0; i < links.size(); i++){
						uint32_t rcv = links[i]->m_rcv[m_rate];
						if ( rcv > max_rcv){	
								max_rcv= rcv;
						}
				}
				NS_ASSERT( max_rcv != 0);
				return std::ceil((double)m_MNC_K*(double)m_max/(double)max_rcv);
		}
		else{
			uint8_t min_l = m_MNC_K+1;
			bool zero_l = false;
			uint16_t my_bitmap = l_nodeinfo.GetBitmap();  
			uint8_t my_rank = l_nodeinfo.GetRank();

			std::vector<Ptr<LinkInfo> > links = l_nodeinfo.GetInfo();						
			for (uint32_t i=0; i < links.size(); i++){
				if (links[i]->m_neighbor){
						Ptr<NodeInfo> dest = l_topo.Find(links[i]->GetAddr());

						uint8_t l = CalculateL(my_bitmap, dest->GetBitmap(), my_rank, dest->GetRank());
						NS_LOG_INFO("Dest: " << links[i]->GetAddr() << "Bitmap1: " << my_bitmap << " Bitmap 2: " <<  dest->GetBitmap() << " Rank 1: " << (uint32_t)my_rank << " Rank 2: " << (uint32_t)dest->GetRank() << " L: " << (uint32_t)l);
						
						if (l == 0 && min_l == m_MNC_K+1 )
						{
							zero_l = true;	
						}
						else if (l > 0 && l < min_l)
						{
								min_l = l;
								zero_l = false;
						}
				}
			}
			if (zero_l)
				min_l = 0;

			NS_ASSERT(min_l <= m_MNC_K);
			return min_l;
		}
}

uint8_t 
UfloodWifiMac::CalculateL(uint16_t bitmap_a, uint16_t bitmap_b, uint8_t rank_a, uint8_t rank_b){
		uint16_t bit_sub = bitmap_a ^ (bitmap_a & bitmap_b);
		uint8_t bit_diff = 0;
		NS_ASSERT( rank_a <= m_MNC_K && rank_b <= m_MNC_K);
		
		for (bit_diff=0; bit_sub; bit_sub >>=1){
				bit_diff += bit_sub & 1;
		}
		//bit_sub = bitmap_a ^ (bitmap_a & bitmap_b);
		//NS_LOG_INFO("My bitmap " << bitmap_a << " bitmap " << bitmap_b << "bit sub" <<  bit_sub <<  " bit diff: " << (uint16_t)bit_diff);
	
		if (rank_b == m_MNC_K)
				return 0;
		else if (rank_a > rank_b)
				return (rank_a - rank_b);
		else if (bit_diff > 0)
				return bit_diff;
		else
				return 0;
		
		//uint8_t rank_diff = rank_a - rank_b > 0 ? (rank_a - rank_b) : 0;
		//return bit_diff > rank_diff ? bit_diff : rank_diff;
}


bool
UfloodWifiMac::IsAllReceived (void)
{
	bool ret = true;
	
	for(uint32_t i=0; i< l_topo.GetN(); i++)
	{
		if (l_topo.Get(i)->GetRank() < m_MNC_K || l_topo.Get(i)->GetInter() ==1 ){
			ret = false;
			return ret;
		}
	}
	
	return ret;
}

void
UfloodWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  // The approach taken here is that, from the point of view of a STA
  // in IBSS mode, the link is always up, so we immediately invoke the
  // callback if one is set
  linkUp ();
}

void
UfloodWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();

  if (hdr->IsFeedback())
  {
		  if (hdr->IsWep()){
				  NS_LOG_DEBUG("Receive Feedback from" << from);
				  MgtFeedback feedback; 
				  packet->RemoveHeader (feedback);
				  uint8_t rank = feedback.GetRank();
				  uint16_t bitmap = feedback.GetBitmap();
				  NeighborInfo nbr = feedback.GetNeighbor();
				  std::vector<NbrItem> item = nbr.GetNbrs();

				  if (feedback.GetEid() != m_eid)
				  {
						  NS_LOG_INFO("Outdated Feedback");
						  return;
				  }

				  if (m_eid == feedback.GetEid())
				  {
			  		Ptr<NodeInfo> node = l_topo.Find(from);
					NS_LOG_DEBUG("Feedback Data Rank: " << (uint16_t)node->GetRank() <<  " -> " << (uint16_t)rank << " Bitmap: " << node->GetBitmap() << " -> " <<  bitmap);
			  		if (node != NULL)
					{
						node->SetRank(rank);
						node->SetBitmap(bitmap);
						node->SetInter(0);
					}
						
					for (uint32_t i=0;  i < item.size(); i++){
							Ptr<LinkInfo> link1 = l_nodeinfo.FindLinkById(item[i].id);
							if (link1 != NULL){
									Ptr<NodeInfo> node1 = l_topo.Find(link1->GetAddr());
									uint32_t rcv_from = l_topo.GetRcv(from, link1->GetAddr());
									uint32_t my_rcv = link1->m_rcv[m_rate];

									if ((((node1->GetInter() == 0 && item[i].inter == 0) ||  (node1->GetInter() == 1 && item[i].inter == 1) ) && (item[i].rank > node1->GetRank())) || (rcv_from > my_rcv)){
											node1->SetRank(item[i].rank);								
									}

							}
					}
				  }
				  return;
		  }
  }

  if (hdr->IsBatchAck())
  {
	  NS_LOG_DEBUG("Receive BatchAck from" << from);
	  MgtBatchAck batchack;
	  packet->RemoveHeader (batchack);
	  
	  uint64_t bitmap = batchack.GetBitmap(); 
	  NeighborInfo nbr = batchack.GetNeighbor();
	  std::vector<NbrItem> item = nbr.GetNbrs();

	  if (batchack.GetEid() != m_eid)
	  {
			  NS_LOG_INFO("Outdated BatchAck");
			  return;
	  }

	  if (m_eid == batchack.GetEid())
	  {
	  		m_cumm_ack |= bitmap; 
			NS_LOG_INFO("Cummulative ACK Bitmap: " << std::hex << bitmap << " ACK: " << m_cumm_ack << std::dec);  
			for (uint32_t i=0; i < l_topo.GetN(); i++){
				if (m_cumm_ack & (1 << i)){
					NS_LOG_INFO("Node " << i);
					l_topo.Get(i)->SetRank(m_MNC_K);	
					l_topo.Get(i)->SetBitmap(0xffff);
					l_topo.Get(i)->SetInter(0);
				}
			}
			
			/*
			  Ptr<NodeInfo> node = l_topo.Find(from);
			  if (node != NULL)
			  {
					  node->SetRank(m_MNC_K);
					  node->SetBitmap(0xffff);
					  node->SetInter(0);
			  }
			*/

			  for (uint32_t i=0;  i < item.size(); i++){
					  Ptr<LinkInfo> link1 = l_nodeinfo.FindLinkById(item[i].id);
					  if (link1 != NULL){
							  Ptr<NodeInfo> node1 = l_topo.Find(link1->GetAddr());
							  uint32_t rcv_from = l_topo.GetRcv(from, link1->GetAddr());
							  uint32_t my_rcv = link1->m_rcv[m_rate];

							  if ((((node1->GetInter() == 0 && item[i].inter == 0) ||  (node1->GetInter() == 1 && item[i].inter == 1) ) && (item[i].rank > node1->GetRank())) || (rcv_from > my_rcv)){
									  node1->SetRank(item[i].rank);								
							  }
					  }
			  }
	  }





	  if (!IsSrc())
	  {
			NS_LOG_INFO("Relay Packet");
			SendBatchAck(l_nodeinfo.GetParent()->GetAddr());
	  }
	
	/*
	  if (!IsSrc() && to == m_low->GetAddress())
	  {
			
	  }
	*/

	  if (IsSrc())
	  {
		if (IsAllReceived()){
		NS_LOG_INFO ("All node received " << m_eid  );
		m_doing = false;
		m_eid++;

		ResetTopo();
		m_MNC_Encoder.FlushAllBuffer();
		if (m_feedbackEvent.IsRunning())
			m_feedbackEvent.Cancel();
		if (m_relayEvent.IsRunning())
			m_relayEvent.Cancel();
		if (m_utilityEvent.IsRunning())
			m_utilityEvent.Cancel();
		if (m_childEvent.IsRunning())
				m_childEvent.Cancel();
		m_cumm_ack = 0;
		
		total_time = Simulator::Now().GetSeconds()-init_time;

		uint32_t nNode = NodeList::GetNNodes();
		for (uint32_t i = 0; i < nNode; i++)
		{
				Ptr<Node> node = NodeList::GetNode(i);
				for (uint32_t j=0; j < node->GetNDevices(); j++)
				{
						Ptr<WifiNetDevice> device = node->GetDevice(j)->GetObject<WifiNetDevice> ();
						if (device !=0)
						{
								Ptr<UfloodWifiMac> u_mac = device->GetMac()->GetObject<UfloodWifiMac> ();
								if (!u_mac->IsSrc())
										u_mac->EndBatch();
						}
				}
		}


		if (m_queue->GetSize() >= m_MNC_K)
		{
				uint8_t tid=5;
				NS_LOG_DEBUG ("start new batch Eid: " << m_eid);
				for (uint8_t i=0; i < m_MNC_K; i++){
						WifiMacHeader tempheader;
						Ptr<const Packet> temppacket = m_queue->Dequeue(&tempheader);
						WifiMacHeader tempheader2 = tempheader;
						Ptr<Packet> temp = temppacket->Copy();
						m_MNC_Encoder.MNC_Enqueue(temp, tempheader2);
				}
				uint8_t burstsize = GetBurstSize(1);
				NS_LOG_INFO("Burst size: " << (uint16_t)burstsize);

				//NS_LOG_INFO("Size before encoding: " << packet->GetSize());
				m_MNC_Encoder.Encoding(m_eid, m_MNC_K, burstsize, m_MNC_P, 0);

				FeedbackInterpolation(m_low->GetAddress(), burstsize, 0xffff, 1 );
				NeighborInfo nbr(GenerateNbrInfo());
				
				Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
				uint16_t tmpId = (uint16_t) rand->GetInteger(0,0xffff);
						m_burst = (m_burst != tmpId ? tmpId : ~tmpId);

				for(uint8_t i = 0; i < burstsize; i++){ 
						//m_stationManager->SetMulticastModeIndex(j->rate);
						WifiMacHeader temphdr;
						Ptr<const Packet> encoded_packet = m_MNC_Encoder.MNC_Dequeue(&temphdr);
						Ptr<Packet> send_packet = encoded_packet->Copy();  

						//NS_LOG_INFO("Size after encoding: " << send_packet->GetSize());

						UfloodDataHeader u_hdr;
						u_hdr.SetEid(m_eid);
						u_hdr.SetRank(m_MNC_K);
						u_hdr.SetBitmap(0xffff);
						u_hdr.SetSend(burstsize);
						u_hdr.SetRemain(burstsize-i-1);
						u_hdr.SetBurst(m_burst);
						
						u_hdr.SetNeighbor (nbr);

						send_packet->AddHeader(u_hdr);

						RateTag tag(m_rate);
						ConstCast<Packet> (send_packet)->AddPacketTag(tag);
						if(m_qosSupported){
								// Sanity check that the TID is valid
								NS_ASSERT (tid < 8);
								NS_LOG_DEBUG(temphdr.GetAddr1() << "edca Queue");
								temphdr.SetWep(1);
								m_edca[QosUtilsMapTidToAc(tid)]->Queue(send_packet, temphdr);
						}
						else{
								NS_LOG_DEBUG("dca Queue");
								temphdr.SetWep(1);
								m_dca->Queue(send_packet, temphdr);
						}
				}

				Ptr<NodeInfo> node_src = l_topo.Find(l_nodeinfo.GetAddr());
				node_src->SetRank(m_MNC_K);
				node_src->SetBitmap(0xffff);
				node_src->SetInter(0);
				
				l_nodeinfo.SetRank(m_MNC_K);
				l_nodeinfo.SetBitmap(0xffff);
				l_nodeinfo.SetInter(0);

				m_MNC_Encoder.FlushN();
				m_doing = true;
				
				
				if (m_utilityEvent.IsRunning())
						m_utilityEvent.Cancel();
				double next_fb = CalcTxTime(m_rate, burstsize)+2000;
				m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);
		}
		}
	  }
	  return;
  }
		if (hdr->IsData () && hdr->IsWep()){
				if (IsSrc()){
						UfloodDataHeader u_data;
						packet->RemoveHeader(u_data);

						uint16_t eid = u_data.GetEid();
						uint8_t rank = u_data.GetRank();
						uint16_t bitmap = u_data.GetBitmap();
						uint8_t send = u_data.GetSend();
						uint8_t remain = u_data.GetRemain();
						uint16_t burst = u_data.GetBurst(); 
						NeighborInfo info = u_data.GetNeighbor();
				  		std::vector<NbrItem> item = info.GetNbrs();
				
						if (eid != m_eid){
								NS_LOG_INFO("Outdated eid: " << eid);
								return;
						}

						if (burst != m_burst){
								FeedbackInterpolation(from, send, bitmap, (from==src_addr)); //warning timing might be incorrect
								m_burst = burst;

								if (m_relayEvent.IsRunning())
										m_relayEvent.Cancel();
								if (m_utilityEvent.IsRunning())
										m_utilityEvent.Cancel();
						}
						
						Ptr<NodeInfo> node = l_topo.Find(from);
						if (node != NULL)
						{
								node->SetRank(rank);
								node->SetBitmap(bitmap);
								node->SetInter(0);
						}

						for (uint32_t i=0;  i < item.size(); i++){
							Ptr<LinkInfo> link1 = l_nodeinfo.FindLinkById(item[i].id);
							if (link1 != NULL){
								Ptr<NodeInfo> node1 = l_topo.Find(link1->GetAddr());
								uint32_t rcv_from = l_topo.GetRcv(from, link1->GetAddr());
								uint32_t my_rcv = link1->m_rcv[m_rate];
								
								if ((((node1->GetInter() == 0 && item[i].inter == 0) ||  (node1->GetInter() == 1 && item[i].inter == 1) ) && (item[i].rank > node1->GetRank())) || (rcv_from > my_rcv)){
										node1->SetRank(item[i].rank);								
								}
							}
						}
						uint8_t send_rate = l_topo.Find(from)->GetRate();

						if (m_utilityEvent.IsRunning())
						{
								m_utilityEvent.Cancel();
						}
						double next_fb = CalcTxTime(send_rate, remain)+2000;
						NS_LOG_INFO("Rate: " << (uint32_t)send_rate << " Estimated Burst End Time : " << next_fb);
						m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);
						
						if (m_childEvent.IsRunning())
						{
								m_childEvent.Cancel();
						}
						next_fb = CalcTxTime(send_rate, remain) + CalcTxTime(m_data_rate, 3) ; 
						m_childEvent = Simulator::Schedule (MicroSeconds(next_fb), &UfloodWifiMac::CheckChild, this);
						
						NS_LOG_INFO ("UfloodData id: " << eid << " rank: " << (uint16_t)rank << " Bitmap: " << bitmap << " send: " << (uint16_t)send << " remain " <<(uint16_t)remain << " num info: " << (uint16_t)info.GetSize() << " eid: " << (uint16_t)m_eid << " m_succ " << m_succ  << " current rank: " << (uint32_t)l_nodeinfo.GetRank());
					}
				else{
						UfloodDataHeader u_data;
						packet->RemoveHeader(u_data);

						uint16_t eid = u_data.GetEid();
						uint8_t rank = u_data.GetRank();
						uint16_t bitmap = u_data.GetBitmap();
						uint8_t send = u_data.GetSend();
						uint8_t remain = u_data.GetRemain();
						uint16_t burst = u_data.GetBurst(); 
						NeighborInfo info = u_data.GetNeighbor();
				  		std::vector<NbrItem> item = info.GetNbrs();
						
						//info.Print(std::cout);

						//NS_LOG_INFO("Size: " << packet->GetSize());

						if (eid != m_eid){
								NS_LOG_INFO("Outdated eid: " << eid);
								return;
						}
						
						if (!m_doing){
								NS_ASSERT(m_succ == true);
								ResetTopo();
								m_MNC_Encoder.FlushAllBuffer();
								if (m_feedbackEvent.IsRunning())
										m_feedbackEvent.Cancel();
								if (m_relayEvent.IsRunning())
										m_relayEvent.Cancel();
								if (m_utilityEvent.IsRunning())
										m_utilityEvent.Cancel();
								if (m_childEvent.IsRunning())
										m_childEvent.Cancel();

								m_succ = false;
								m_cumm_ack = 0;
								m_doing = true;
						}
					
						if (burst != m_burst){
								FeedbackInterpolation(from, send, bitmap, (from==src_addr)); //warning timing might be incorrect
								m_burst = burst;
								
								if (m_relayEvent.IsRunning())
										m_relayEvent.Cancel();
								if (m_utilityEvent.IsRunning())
										m_utilityEvent.Cancel();
						}
					

						if (from == src_addr){
							uint16_t bit = l_nodeinfo.GetBitmap();
							bit = SetBit(bit, (send-remain-1));
							l_nodeinfo.SetBitmap(bit);
							uint8_t rank = l_nodeinfo.GetRank()+1 <= m_MNC_K ? l_nodeinfo.GetRank()+1 : m_MNC_K;
							l_nodeinfo.SetRank(rank);
							Ptr<NodeInfo> node = l_topo.Find(l_nodeinfo.GetAddr());
							node->SetRank(rank);
							node->SetBitmap(bit);
							node->SetInter(0);
						}
						else{
							uint16_t bit = l_nodeinfo.GetBitmap();
							bit |= bitmap;
							l_nodeinfo.SetBitmap(bit);
							uint8_t rank = l_nodeinfo.GetRank()+1 <= m_MNC_K ? l_nodeinfo.GetRank()+1 : m_MNC_K;
							l_nodeinfo.SetRank(rank);
							Ptr<NodeInfo> node = l_topo.Find(l_nodeinfo.GetAddr());
							node->SetRank(rank);
							node->SetBitmap(bit);
							node->SetInter(0);
						}					


						Ptr<NodeInfo> node = l_topo.Find(from);
						if (node != NULL)
						{
								node->SetRank(rank);
								node->SetBitmap(bitmap);
								node->SetInter(0);
						}

						for (uint32_t i=0;  i < item.size(); i++){
							Ptr<LinkInfo> link1 = l_nodeinfo.FindLinkById(item[i].id);
							if (link1 != NULL){
								Ptr<NodeInfo> node1 = l_topo.Find(link1->GetAddr());
								uint32_t rcv_from = l_topo.GetRcv(from, link1->GetAddr());
								uint32_t my_rcv = link1->m_rcv[m_rate];
								
								if ((((node1->GetInter() == 0 && item[i].inter == 0) ||  (node1->GetInter() == 1 && item[i].inter == 1) ) && (item[i].rank > node1->GetRank())) || (rcv_from > my_rcv)){
										node1->SetRank(item[i].rank);								
								}
							}
						}
					
						uint8_t send_rate = l_topo.Find(from)->GetRate();

						if (m_utilityEvent.IsRunning())
						{
								m_utilityEvent.Cancel();
						}
						double next_fb = CalcTxTime(send_rate, remain)+2000;
						NS_LOG_INFO("Rate: " << (uint32_t)send_rate << " Estimated Burst End Time : " << next_fb);
						m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);
						
						if (m_childEvent.IsRunning())
						{
								m_childEvent.Cancel();
						}
						next_fb = CalcTxTime(send_rate, remain) + CalcTxTime(m_data_rate, 3) ; 
						m_childEvent = Simulator::Schedule (MicroSeconds(next_fb), &UfloodWifiMac::CheckChild, this);
						
						if(m_succ == false){
								if (m_feedbackEvent.IsRunning())
								{
										m_feedbackEvent.Cancel();
								}
								double next_fb = CalcTxTime(send_rate, remain) + CalcTxTime(m_data_rate, 3) ; 
								m_feedbackEvent = Simulator::Schedule (MicroSeconds(next_fb), &UfloodWifiMac::SendFeedback, this);
						}
						
						Ptr<Packet> packet2 = packet->Copy();						

						NS_LOG_INFO ("UfloodData id: " << eid << " rank: " << (uint16_t)rank << " Bitmap: " << bitmap << " send: " << (uint16_t)send << " remain " <<(uint16_t)remain << " num info: " << (uint16_t)info.GetSize() << " eid: " << (uint16_t)m_eid << " m_succ " << m_succ  << " current rank: " << (uint32_t)l_nodeinfo.GetRank());
				

						if (eid == m_eid && m_succ == false){
								m_MNC_Decoder.MNC_Rx_Enqueue(packet2, hdr);

								if(m_MNC_Decoder.GetBufferSize() >= m_MNC_K){
										NS_LOG_INFO("Decoding Try: " << eid );

										if (!m_MNC_Decoder.Decoding(m_MNC_K, m_MNC_P, false)){
												NS_LOG_INFO("Decoding Failure: " << eid);
											
											if (!m_feedbackEvent.IsRunning())
											{
												double next_fb = CalcTxTime(send_rate, remain) + CalcTxTime(m_data_rate, 3) ; 
												m_feedbackEvent = Simulator::Schedule (MicroSeconds(next_fb), &UfloodWifiMac::SendFeedback, this);
											}
												l_nodeinfo.SetRank(m_MNC_K-1);
												l_nodeinfo.SetBitmap(0xfffe);

												return;
										}
										else{
												NS_LOG_INFO("Decoding Success: " << eid);
												m_succ = true;
												
												m_cumm_ack |= 1 << m_id; 
												if (m_feedbackEvent.IsRunning())
												{
														m_feedbackEvent.Cancel();
												}
												SendBatchAck(l_nodeinfo.GetParent()->GetAddr());
										}
										for(uint8_t i = 0; i < m_MNC_K; i++){
												WifiMacHeader temphdr;
												Ptr<const Packet> decoded_packet = m_MNC_Decoder.MNC_Rx_DDequeue(&temphdr);
												Ptr<Packet> forward_packet = decoded_packet->Copy();
												ForwardUp (forward_packet, from, to);	
												if(!IsSrc()){
														WifiMacHeader hdr;
														hdr.SetTypeData();
														hdr.SetAddr1(Mac48Address::GetMulticastPrefix());
														hdr.SetAddr2(m_low->GetAddress());
														hdr.SetAddr3(GetBssid());
														hdr.SetDsNotFrom();
														hdr.SetDsNotTo();
														hdr.SetNoMoreFragments();
														hdr.SetNoRetry();
														hdr.SetNoOrder();

														m_MNC_Encoder.MNC_Enqueue(decoded_packet, hdr);
												}
										}
								}
						}
				}

		return;
		}

  LlcSnapHeader llc;
  if (packet->RemoveHeader(llc))
  {
 	 if(llc.GetType() == 0x0811)
 	 {
		 //NS_LOG_INFO("Receive Tl packet");
		  ReceiveTl(packet, from);
		  return;
 	 }
     	
	 packet->AddHeader(llc);
  }
  
  if (hdr->IsData ())
    {
	  //NS_LOG_INFO("Receive data packet");
      if (hdr->IsQosData () && hdr->IsQosAmsdu ())
        {
          NS_LOG_DEBUG ("Received A-MSDU from" << from);
          DeaggregateAmsduAndForward (packet, hdr);
        }
      else
      {
	      // Default AdhocWifiMac Receiver
		      ForwardUp (packet, from, to);
      }
      //NS_LOG_DEBUG("End Receive");
      return;
    }


  // Invoke the receive handler of our parent class to deal with any
  // other frames. Specifically, this will handle Block Ack-related
  // Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

bool
UfloodWifiMac::IsSrc (void)
{
	return m_src;
}

#if 0
/*
TopologyResult
UfloodWifiMac::GetResults(void)
{
	return m_topos;
}
*/



double
UfloodWifiMac::GetUtility (void)
{
	return m_utility;
}
#endif

void
UfloodWifiMac::SendFeedback(void)
{
	NS_LOG_FUNCTION(this);

	if (m_succ){
			if (m_feedbackEvent.IsRunning())
					m_feedbackEvent.Cancel();
			return;
	}

	WifiMacHeader hdr;
	hdr.SetFeedback();
 	hdr.SetAddr1(Mac48Address::GetMulticastPrefix());
 	hdr.SetAddr2(m_low->GetAddress());
 	hdr.SetAddr3(GetBssid());
	hdr.SetDsNotFrom();
	hdr.SetDsNotTo();
	hdr.SetWep(1);

	Ptr<Packet> packet = Create<Packet>();

	MgtFeedback feedback;
	feedback.SetEid(m_eid);
	feedback.SetRank(l_nodeinfo.GetRank());
	feedback.SetBitmap(l_nodeinfo.GetBitmap());
	NeighborInfo nbr(GenerateNbrInfo());
	feedback.SetNeighbor(nbr);

	packet->AddHeader(feedback);
	m_dca->Queue(packet, hdr);

	NS_LOG_INFO("Send Feedback " << m_low->GetAddress() << " Rank: " << (uint16_t)l_nodeinfo.GetRank() << " Bitmap: " << l_nodeinfo.GetBitmap() );
	
	if (m_feedbackEvent.IsRunning())
	{
			m_feedbackEvent.Cancel();
	}
	if (!m_succ){
		double next_fb = CalcTxTime(m_data_rate, 3);
		m_feedbackEvent = Simulator::Schedule (MicroSeconds(next_fb), &UfloodWifiMac::SendFeedback, this);
	}
}

void
UfloodWifiMac::SendBatchAck(Mac48Address to)
{
	NS_LOG_FUNCTION(this);
	
	NS_LOG_INFO("Send BatchAck to " << to);
	WifiMacHeader hdr;
	hdr.SetBatchAck();
 	hdr.SetAddr1(to);
 	hdr.SetAddr2(m_low->GetAddress());
 	hdr.SetAddr3(GetBssid());
	hdr.SetDsNotFrom();
	hdr.SetDsNotTo();

	//batchack.SetRank(8);
	// to be implemented with unicast routing 

	Ptr<Packet> packet = Create<Packet>();
	
	MgtBatchAck batchack;
	batchack.SetEid(m_eid);
	batchack.SetBitmap(m_cumm_ack); 
	batchack.SetParent(l_nodeinfo.GetParent()->GetAddr());
	NeighborInfo nbr(GenerateNbrInfo());
	batchack.SetNeighbor(nbr);
	
	packet->AddHeader(batchack);
	
	if(m_qosSupported){
			uint8_t tid =7;
			NS_ASSERT (tid < 8);
		//	NS_LOG_DEBUG(temphdr.GetAddr1() << "edca Queue");
			m_edca[QosUtilsMapTidToAc(tid)]->Queue(packet, hdr);
	}
	else{
		//	NS_LOG_DEBUG("dca Queue");
			m_dca->Queue(packet, hdr);
	}

	//YAMAE

	uint32_t nNode = NodeList::GetNNodes();
	for (uint32_t i = 0; i < nNode; i++)
	{
			Ptr<Node> node = NodeList::GetNode(i);
			for (uint32_t j=0; j < node->GetNDevices(); j++)
			{
					Ptr<WifiNetDevice> device = node->GetDevice(j)->GetObject<WifiNetDevice> ();
					if (device !=0)
					{
							Ptr<UfloodWifiMac> u_mac = device->GetMac()->GetObject<UfloodWifiMac> ();
							u_mac->BatchAck(m_cumm_ack);
					}
			}
	}

}



void
UfloodWifiMac::ReceiveTl (Ptr<Packet> packet, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << from);
  	
  TlHeader tl;
  packet->RemoveHeader(tl);
		//tl.Print(std::cout);

  switch(tl.GetType())
  {
		  case TL_TF1:
				 {
					uint8_t rate = tl.GetRate();
					if (cur_tf_addr == Mac48Address())
					{
						m_nodeinfo->SetAddr(m_low->GetAddress());
						Ptr<LinkInfo> link = CreateObject<LinkInfo> ();
						link->SetAddr(from);
						link->m_rcv[rate]++;
						m_nodeinfo->AddLink(link);
						m_nodeinfo->SetCharging(m_charging);
						m_nodeinfo->SetRate(0);
						cur_tf_addr = from;
					}
					else if (cur_tf_addr != from){
						Ptr<LinkInfo> link = m_nodeinfo->FindLink(from);
						if(link == NULL){
								link = CreateObject<LinkInfo> ();
								link->SetAddr(from);
								link->m_rcv[rate]=1;
								m_nodeinfo->AddLink(link);
						}
						else{
								link->ResetRcv();
								link->m_rcv[rate]++;
						}
						cur_tf_addr = from;
					}
					else{
						Ptr<LinkInfo> link = m_nodeinfo->FindLink(from);
						if(link == NULL){
								link = CreateObject<LinkInfo> ();
								link->SetAddr(from);
								link->m_rcv[rate]++;
								m_nodeinfo->AddLink(link);
						}
						else{
								link->m_rcv[rate]++;
						}
					}
					break;
				 }
		default:
			  NS_ASSERT(false);
			  return;
  }
}
Ptr<TopologyResult>
UfloodWifiMac::GetResults (void)
{
	return m_topo;
}
/*
void
UfloodWifiMac::NoNbr(void)
{
	std::cout << "NoNbr in Rate " << uint16_t(m_t_rate) << std::endl;
	NextRate();
}
*/

	
void
UfloodWifiMac::SetTopo (Ptr<TopologyResult> result)
{
	NS_LOG_FUNCTION(this);
	m_topo = result->Copy();

	for (uint32_t i=0; i < m_topo->GetN(); i++){
			Ptr<NodeInfo> node = m_topo->Get(i);
			std::vector<Ptr<LinkInfo> > links = node->GetInfo();
			Ptr<NodeInfo> node_info = CreateObject<NodeInfo> ();

			node_info->SetAddr(node->GetAddr());	
			node_info->SetRate(node->GetRate());
			node_info->SetCharging(node->IsCharging());

			uint8_t num_nbr = 0;
			for (uint32_t j=0; j < links.size(); j++){
					Ptr<LinkInfo> link = links[j]->Copy();
					node_info->AddLink(link);
					if (links[j]->m_neighbor)
					{
						num_nbr++;
					}
			}
			
			node_info->SetNumNeighbor(num_nbr);

			if (node->GetAddr() == src_addr)
			{
				node_info->SetRank(m_MNC_K);
				node_info->SetUtility(0);
				node_info->SetBitmap(0xffff);
				node_info->SetInter(0);
			}

			l_topo.Add (node_info);
			
			//Copy to l_nodeinfo
			if (node->GetAddr() == m_low->GetAddress() )
			{
					l_nodeinfo.SetAddr(node->GetAddr());
					l_nodeinfo.SetRate(node->GetRate());
					l_nodeinfo.SetParent(node->GetParent());
					l_nodeinfo.SetCharging(m_charging);
					l_nodeinfo.SetNumNeighbor(num_nbr);
					
					for (uint32_t j=0; j < links.size(); j++){
							Ptr<LinkInfo> link = links[j]->Copy();
							l_nodeinfo.AddLink(link);
					}
					uint32_t n_child = (uint32_t)node->GetChilds().size();
					//NS_LOG_UNCOND("Copy Child size: " << node->GetChilds().size() << " " << n_child);
					for (uint32_t j=0; j < n_child; j++){
							Ptr<NodeInfo> child = node->GetChilds().at(j);
							l_nodeinfo.AddChild(child);
					}

					m_rate = node->GetRate();
					m_id = (uint8_t)i;
			}
	}

	std::vector<Ptr<LinkInfo> > links = l_nodeinfo.GetInfo(); 
	uint8_t min_r = 8;
	for (uint32_t i=0; i < links.size(); i++){
			if (links[i]->m_neighbor){
					Ptr<NodeInfo> node = l_topo.Find(links[i]->GetAddr());
					uint8_t rate = node->GetRate(); 
					if (rate < min_r)
							min_r = rate;
			}
	}
	if (min_r == 8)
		m_data_rate = m_rate;
	else
		m_data_rate = min_r;
}

void
UfloodWifiMac::SendTraining (void){
	NS_LOG_FUNCTION (this << m_max);

	NS_LOG_INFO("Send Training " << m_low->GetAddress());
	WifiMacHeader hdr;
	hdr.SetTypeData();
	hdr.SetAddr1(Mac48Address::GetMulticastPrefix());
	hdr.SetAddr2(m_low->GetAddress());
	hdr.SetAddr3(m_low->GetBssid());
	hdr.SetDsNotFrom();
	hdr.SetDsNotTo();

	TlHeader tl;
	tl.SetType(TL_TF1);

	LlcSnapHeader llc;
	llc.SetType(0x0811);
	
	for (uint8_t j=0; j<8; j++){
		for (uint32_t i = 0; i < m_max; i++)
		{
					Ptr<Packet> packet = Create<Packet>(1470);
					
					tl.SetSeq(i);
					tl.SetRate(j);
					RateTag rtag(j);

					packet->AddHeader(tl);
					packet->AddHeader(llc);
					packet->AddPacketTag(rtag);
					m_dca->Queue(packet, hdr);
		}
	}
}

Ptr<NodeInfo>
UfloodWifiMac::GetNodeInfo(void){
	NS_LOG_FUNCTION(this);
	return m_nodeinfo;
}

Ptr<TopologyResult> 
UfloodWifiMac::GetTopo(void){
	return m_topo;
}

void
UfloodWifiMac::CalculateRate(void){
	Ptr<NodeInfo> node=NULL;

	NS_LOG_UNCOND("Num node: " << m_topo->GetN());
	for (uint32_t i=0; i<m_topo->GetN(); i++){
		Ptr<NodeInfo> node2 = m_topo->Get(i);
		if (node2->GetAddr() == m_nodeinfo->GetAddr())
		{
				node = node2;
				break;
		}
	}
	
	if (node == NULL){
			m_rate=0;
			return;
	}
	else{
			m_nodeinfo->SetParent(node->GetParent());
			if (node->GetChilds().size() == 0)
			{
					m_rate = 0;
			}
			else{
					uint32_t n_child = (uint32_t)node->GetChilds().size();
					NS_LOG_UNCOND("Find node info child size: " << node->GetChilds().size() << " " << n_child);
					uint8_t rate_min = 8;
					for (uint32_t j=0; j < n_child; j++){
							Ptr<NodeInfo> child = node->GetChilds().at(j);
							m_nodeinfo->AddChild(child);
							Ptr<LinkInfo> link = m_nodeinfo->FindLink(child->GetAddr());
							NS_ASSERT(link != NULL);
							NS_ASSERT(link->GetRate() < 8);
							if (link->GetRate() < rate_min)
							{
									rate_min = link->GetRate();
							}
					}
					m_rate = rate_min;
					NS_ASSERT(m_rate < 8);
			}
	}
}

uint8_t
UfloodWifiMac::GetRate(void){
		return m_rate;
}

std::vector<NbrItem>
UfloodWifiMac::GenerateNbrInfo(void)
{
	std::vector<NbrItem> ret;
	std::vector<Ptr<LinkInfo> > links = l_nodeinfo.GetInfo();						
	
	for (uint32_t i=0; i <links.size(); i++){
		if (links[i]->m_neighbor == true)
		{
			Ptr<NodeInfo> node = l_topo.Find(links[i]->GetAddr());
			NbrItem item;
			item.id = links[i]->m_id;
			item.rank = node->GetRank();
			item.inter = node->GetInter();
			ret.push_back(item);
		}
	}

	return ret;
}

double 
UfloodWifiMac::CalcTxTime(uint8_t rate, uint8_t num)
{
		WifiTxVector txvector;
		uint32_t packet_size =1427; 

		switch(rate){
			case 0:
				txvector.SetMode(WifiMode("OfdmRate6Mbps"));
				break;
			case 1:
				txvector.SetMode(WifiMode("OfdmRate9Mbps")); 
				break;
			case 2:
				txvector.SetMode(WifiMode("OfdmRate12Mbps")); 
				break;
			case 3:
				txvector.SetMode(WifiMode("OfdmRate18Mbps")); 
				break;
			case 4:
				txvector.SetMode(WifiMode("OfdmRate24Mbps")); 
				break;
			case 5:
				txvector.SetMode(WifiMode("OfdmRate36Mbps")); 
				break;
			case 6:
				txvector.SetMode(WifiMode("OfdmRate48Mbps")); 
				break;
			case 7:
				txvector.SetMode(WifiMode("OfdmRate54Mbps"));
				break;
			default:
				NS_ASSERT(rate < 8);
		}

		double usec =  (m_phy->CalculateTxDuration(packet_size, txvector, WIFI_PREAMBLE_LONG)).GetMicroSeconds();
		return (usec+103)*(double)num;
}

void
UfloodWifiMac::ResetTopo()
{
	for (uint32_t i=0; i< l_topo.GetN(); i++){
		if (l_topo.Get(i)->GetAddr() == src_addr)
		{
			l_topo.Get(i)->SetRank(m_MNC_K);
			l_topo.Get(i)->SetUtility(0);
			l_topo.Get(i)->SetBitmap(0xffff);
			l_topo.Get(i)->SetInter(0);
		}
		else{
				l_topo.Get(i)->SetRank(0);
				l_topo.Get(i)->SetUtility(0);
				l_topo.Get(i)->SetBitmap(0);
				l_topo.Get(i)->SetInter(1);
		}
	}
	
	if (!IsSrc()){
			l_nodeinfo.SetRank(0);
			l_nodeinfo.SetUtility(0);
			l_nodeinfo.SetBitmap(0);
			l_nodeinfo.SetInter(1);
	}
	else{
			l_nodeinfo.SetRank(m_MNC_K);
			l_nodeinfo.SetUtility(0);
			l_nodeinfo.SetBitmap(0xffff);
			l_nodeinfo.SetInter(0);
	}
}

uint16_t
UfloodWifiMac::SetBit(uint16_t bit, uint8_t seq)
{
	uint16_t ret = bit;
	ret |= 1 << seq;
	return ret;
}

void
UfloodWifiMac::FeedbackInterpolation(Mac48Address from, uint8_t send, uint16_t bitmap, bool src)
{
	NS_LOG_FUNCTION(this);
	
	Ptr<NodeInfo> node = l_topo.Find(from);
	
	if (node == NULL)
	{
		NS_LOG_DEBUG("Invalid address");
		return;
	}

	else
	{
			std::vector<Ptr<LinkInfo> > info = node->GetInfo();
			NS_LOG_INFO("Feedback Interpolation Result");
			for (uint32_t j=0; j< info.size(); j++)
			{
				if (info[j]->GetAddr () == l_nodeinfo.GetAddr())
					continue;

				Ptr<NodeInfo> dest = l_topo.Find(info[j]->GetAddr());
				UniformVariable a(0,1);
				uint32_t rcv = info[j]->m_rcv[node->GetRate()];
				uint8_t rank = dest->GetRank();
				uint16_t my_bitmap = dest->GetBitmap();
				uint8_t inter = dest->GetInter();
				
				if (rank < m_MNC_K || inter == 1){
						for (uint8_t k=0; k<send; k++){
								if(a.GetValue() < (double)rcv/(double)m_max)
								{	
										if (src){
												my_bitmap = SetBit(my_bitmap, k);	
										}
										else{
												my_bitmap |= bitmap; 
										}
										if (rank < m_MNC_K) 
												rank++;
										inter = 1;
								}
						}
				
					dest->SetRank(rank);
					dest->SetBitmap(my_bitmap);
					dest->SetInter(inter);
					
					NS_LOG_INFO("From " << from << " To " << info[j]->GetAddr() << " Rank: " << (uint16_t) rank << " Bitmap: " << my_bitmap << " Inter: " << (uint32_t) inter); 
				}
				else 
					NS_LOG_INFO("Skip interpolation due to succ " << dest->GetAddr());
			}
	}
}

void
UfloodWifiMac::CalculateUtility(void)
{
	NS_LOG_DEBUG("CalculateUtility Type: " << (uint32_t)m_utility_type);

	for (uint32_t i=0; i < l_topo.GetN(); i++)
	{
		double utility=0;
		Ptr<NodeInfo> node = l_topo.Get(i);
		
		double energy = 1;
		if (m_utility_type == 2 || m_utility_type == 4){
			energy = GetNodeEnergy(node->GetAddr());
			if (node->IsCharging())
			{
				energy = energy*m_omega;
			}
		}

		double tx_time = 1/GetMbps(node->GetRate());
		if (m_utility_type == 2 || m_utility_type == 3 || m_utility_type == 4)
				tx_time = CalcTxTime(node->GetRate(), 1);
		
		uint16_t bit_a = node->GetBitmap();
				
		std::vector<Ptr<LinkInfo> > info = node->GetInfo();

		for (uint32_t j = 0; j < info.size(); j++){
			if (info[j]->m_neighbor == true)
			{
				uint8_t rcv = info[j]->m_rcv[node->GetRate()];
				
				Ptr<NodeInfo> node2 = l_topo.Find(info[j]->GetAddr());
				std::vector<Ptr<LinkInfo> > hop2 = node2->GetInfo();
				uint8_t num_2hop = 0;
				for (uint32_t k = 0; k < hop2.size(); k++){
					Ptr<NodeInfo> node_2hop = l_topo.Find(hop2[k]->GetAddr());
					if (node_2hop->GetRank() < m_MNC_K || node_2hop->GetInter() == 1 )
					{
							num_2hop++;
					}
				}

				uint8_t num_nbr = 0;
				if (m_utility_type == 3 || m_utility_type == 4)
				{
					num_nbr = num_2hop;
				}

				uint16_t bit_b = node2->GetBitmap();

				if (CalculateL(bit_a, bit_b, node->GetRank(), node2->GetRank()) > 0 ){ 
					utility += energy*(1+m_alpha*(double)num_nbr)*(double)rcv/(double)m_max/tx_time;
					//NS_LOG_INFO (node->GetAddr() << " Can give benefit to " << node2->GetAddr() << " Bit_a: " << (uint16_t) bit_a << " Bit_b: " << bit_b << " Rank_a: " << (uint16_t)node->GetRank() << " Rank_b: " << (uint16_t)node2->GetRank() );
				}
			}
		}

		node->SetUtility(utility);
//		NS_LOG_INFO ("Node" << i << " Utility: " << utility);

		if(node->GetAddr() == m_low->GetAddress())
		{
				l_nodeinfo.SetUtility(utility);
				NS_LOG_INFO ("Utility of this node " << utility);
		}
	}
}

double 
UfloodWifiMac::GetMbps(uint8_t rate)
{
		switch(rate){
			case 0:
				return 6;
			case 1:
				return 9;
			case 2:
				return 12;
			case 3:
				return 18;
			case 4:
				return 24;
			case 5:
				return 36;
			case 6:
				return 48;
			case 7:
				return 54; 
			default:
				NS_ASSERT(rate < 8);
				return 0;
		}
}

void
UfloodWifiMac::RelayIfBest(void)
{
	CalculateUtility();
	uint16_t rank = 1;
	double utility = l_nodeinfo.GetUtility();

	for (uint32_t i=0; i < l_topo.GetN(); i++){
			if (utility < l_topo.Get(i)->GetUtility())  // to be implemented for calculating the rank by comparing neighbor of neighbor
					rank++;
	}
	
	if(m_utilityEvent.IsRunning()) 
		m_utilityEvent.Cancel();
	
	if (utility > 0){
			NS_LOG_INFO("My order : " << rank);
			if (rank == 1)
					SendBurst();

			else {
					double next_fb = CalcTxTime(m_data_rate, rank+1); 
					m_relayEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::SendBurst, this);
			}
	}
	else{
		double next_fb = CalcTxTime(m_data_rate, 3); 
		m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);
	}
}

void
UfloodWifiMac::SendBurst()
{
	uint8_t burstsize = GetBurstSize(0);
	if (burstsize==0){
		NS_LOG_INFO("Cancel Sending due to zero benefit");
		if (m_relayEvent.IsRunning())
		{
			m_relayEvent.Cancel();
		}
		
		double next_fb = CalcTxTime(m_data_rate, 3); 
		m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);
		return;
	}
	
	WifiMacHeader temp_header;
	Ptr<WifiMacQueue> temp_buffer = CreateObject<WifiMacQueue> ();
	
	if (!m_succ)
	{
		NS_ASSERT(m_MNC_Encoder.GetBufferSize() == 0);
		
		while(m_MNC_Decoder.GetBufferSize() != 0){
			Ptr<const Packet> temp_packet =	m_MNC_Decoder.MNC_Rx_Dequeue(&temp_header);
			Ptr<Packet> packet2 = Create<Packet>();
			packet2 = temp_packet->Copy();
			temp_buffer->Enqueue(packet2, temp_header);
			
			Ptr<Packet> packet = Create<Packet>();
			packet = temp_packet->Copy();
			
			WifiMacHeader hdr;
			hdr.SetTypeData();
			hdr.SetAddr1(Mac48Address::GetMulticastPrefix());
			hdr.SetAddr2(m_low->GetAddress());
			hdr.SetAddr3(GetBssid());
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetNoMoreFragments();
			hdr.SetNoRetry();
			hdr.SetNoOrder();

			m_MNC_Encoder.MNC_Enqueue(packet, hdr);
		}

		while(temp_buffer->GetSize() > 0){
				Ptr<const Packet> temp_packet = temp_buffer->Dequeue(&temp_header);
				Ptr<Packet> packet3 = Create<Packet>();
				packet3 = temp_packet->Copy();
				m_MNC_Decoder.MNC_Rx_Enqueue(packet3, &temp_header);
		}
	}	

	NS_ASSERT(m_MNC_Encoder.GetBufferSize() > 0);
	
	
	if (m_succ){
			m_MNC_Encoder.Encoding(m_eid, m_MNC_K, burstsize, m_MNC_P, 0);
	}
	else
			m_MNC_Encoder.WithCoeffEncoding(m_eid, (uint8_t)m_MNC_Encoder.GetBufferSize(), burstsize, m_MNC_P, 0);
	
	FeedbackInterpolation(m_low->GetAddress(), burstsize, l_nodeinfo.GetBitmap(), IsSrc() );
	NeighborInfo nbr(GenerateNbrInfo());
	//nbr.Print(std::cout);

	Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
	uint16_t tmpId = (uint16_t) rand->GetInteger(0,0xffff);
	m_burst = (m_burst != tmpId ? tmpId : ~tmpId);

	for (uint8_t i=0; i<burstsize; i++){
			WifiMacHeader temphdr;
			//Ptr<const Packet> encoded_packet = m_MNC_Decoder.MNC_Rx_EDequeue(&temphdr);
			Ptr<const Packet> encoded_packet = m_MNC_Encoder.MNC_Dequeue(&temphdr);
			Ptr<Packet> send_packet = encoded_packet->Copy();  

			//NS_LOG_INFO("Size after encoding: " << send_packet->GetSize());

			UfloodDataHeader u_hdr;
			u_hdr.SetEid(m_eid);
			u_hdr.SetRank(l_nodeinfo.GetRank());
			u_hdr.SetBitmap(l_nodeinfo.GetBitmap());
			u_hdr.SetSend(burstsize);
			u_hdr.SetRemain(burstsize-i-1);
			u_hdr.SetBurst(m_burst);

			u_hdr.SetNeighbor (nbr);

			send_packet->AddHeader(u_hdr);
			
			//Ptr<Packet> send_packet2 = send_packet->Copy();  

			RateTag tag(m_rate);
			ConstCast<Packet> (send_packet)->AddPacketTag(tag);
			
			if(m_qosSupported){
					uint8_t tid =5;
					// Sanity check that the TID is valid
					NS_ASSERT (tid < 8);
					NS_LOG_DEBUG(temphdr.GetAddr1() << "edca Queue");
					temphdr.SetWep(1);
					m_edca[QosUtilsMapTidToAc(tid)]->Queue(send_packet, temphdr);
			}
			else{
					NS_LOG_DEBUG("dca Queue");
					temphdr.SetWep(1);
					m_dca->Queue(send_packet, temphdr);
			}
	}
	if (!m_succ)
		m_MNC_Encoder.FlushAllBuffer(); 
	else
		m_MNC_Encoder.FlushN();


	if (m_relayEvent.IsRunning())
	{
			m_relayEvent.Cancel();
	}
	
	if (m_utilityEvent.IsRunning())
			m_utilityEvent.Cancel();
	
	if (m_feedbackEvent.IsRunning())
			m_feedbackEvent.Cancel();

	if (m_childEvent.IsRunning())
			m_childEvent.Cancel();
	
	double next_fb = CalcTxTime(m_rate, burstsize)+2000;
	m_utilityEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::RelayIfBest, this);

	if (!m_succ){
		next_fb = CalcTxTime(m_rate, burstsize)+8000;
		m_feedbackEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::SendFeedback, this);
	}

	next_fb = CalcTxTime(m_rate, burstsize)+CalcTxTime(m_data_rate, 3);
	m_childEvent = Simulator::Schedule(MicroSeconds(next_fb), &UfloodWifiMac::CheckChild, this);

}

void
UfloodWifiMac::SetBasicModes(void)
{
/*
	m_stationManager->AddBasicMode(WifiMode("DsssRate1Mbps"));	// 0
	m_stationManager->AddBasicMode(WifiMode("DsssRate2Mbps"));	// 1
	m_stationManager->AddBasicMode(WifiMode("DsssRate5_5Mbps"));// 2
	m_stationManager->AddBasicMode(WifiMode("DsssRate11Mbps"));	// 3
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate6Mbps"));// 4
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate9Mbps"));// 5
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate12Mbps"));// 6
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate18Mbps"));// 7
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate24Mbps"));// 8
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate36Mbps"));// 9
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate48Mbps"));// 10
	m_stationManager->AddBasicMode(WifiMode("ErpOfdmRate54Mbps"));// 11
*/
	m_stationManager->AddBasicMode(WifiMode("OfdmRate6Mbps"));// 4
	m_stationManager->AddBasicMode(WifiMode("OfdmRate9Mbps"));// 5
	m_stationManager->AddBasicMode(WifiMode("OfdmRate12Mbps"));// 6
	m_stationManager->AddBasicMode(WifiMode("OfdmRate18Mbps"));// 7
	m_stationManager->AddBasicMode(WifiMode("OfdmRate24Mbps"));// 8
	m_stationManager->AddBasicMode(WifiMode("OfdmRate36Mbps"));// 9
	m_stationManager->AddBasicMode(WifiMode("OfdmRate48Mbps"));// 10
	m_stationManager->AddBasicMode(WifiMode("OfdmRate54Mbps"));// 11

}

uint8_t
UfloodWifiMac::GetFeedbackRate(void)
{
	std::vector<Ptr<LinkInfo> > links = l_nodeinfo.GetInfo(); 
	uint8_t min_r = 8;
	for (uint32_t i=0; i < links.size(); i++){
			if (links[i]->m_neighbor){
					Ptr<NodeInfo> node = l_topo.Find(links[i]->GetAddr());
					if (node->GetRank() < m_MNC_K){
							uint8_t rate = node->GetRate(); 
							if (rate < min_r)
									min_r = rate;
					}
			}
	}
	if (min_r == 8)
		return m_rate;
	else
		return min_r;
}

void
UfloodWifiMac::CheckChild(void)
{
	std::vector<Ptr<NodeInfo> > childs = l_nodeinfo.GetChilds();
	
	for (uint32_t i=0; i < childs.size(); i++){
		for (uint32_t j=0; j < l_topo.GetN(); j++){
			if (childs[i]->GetAddr() == l_topo.Get(j)->GetAddr()){
				if (l_topo.Get(j)->GetRank() == m_MNC_K && l_topo.Get(j)->GetInter()==1){
					if (m_cumm_ack & (1 << j))
					{
							NS_LOG_INFO("Without ACK from " << l_topo.Get(j)->GetAddr() << " cumm_ACK: " << std::hex <<  m_cumm_ack << std::dec );
							l_topo.Get(j)->SetRank(m_MNC_K-1);
					}
				}
			}		
		}
	}
}

void
UfloodWifiMac::EndBatch(void)
{
		NS_LOG_INFO("End Batch " << m_eid);
		ResetTopo();
		m_MNC_Encoder.FlushAllBuffer();
		if (m_feedbackEvent.IsRunning())
				m_feedbackEvent.Cancel();
		if (m_relayEvent.IsRunning())
				m_relayEvent.Cancel();
		if (m_utilityEvent.IsRunning())
				m_utilityEvent.Cancel();
		if (m_childEvent.IsRunning())
				m_childEvent.Cancel();
		m_succ = true;
		m_cumm_ack = 0;
		m_eid++;
		m_doing = false;
}


double 
UfloodWifiMac::GetNodeEnergy(Mac48Address addr)
{
	uint32_t nNode = NodeList::GetNNodes();
	for (uint32_t i = 0; i < nNode; i++)
	{
		Ptr<Node> node = NodeList::GetNode(i);
		for (uint32_t j = 0; j < node->GetNDevices(); j++)
		{
			Ptr<WifiNetDevice> device = node->GetDevice(j)->GetObject<WifiNetDevice> ();
			if (device!=0) 
			{
				Ptr<UfloodWifiMac> u_mac = device->GetMac()->GetObject<UfloodWifiMac> ();
				if ((u_mac->IsSrc() && addr==Mac48Address()) || addr == u_mac->GetAddress() )
				{
					Ptr<EnergySource> es = node->GetObject<EnergySourceContainer> ()->Get(0) ;
					if (es== NULL)
						return 0; 
					Ptr<BasicEnergySource> bes = es->GetObject<BasicEnergySource> ();

					if (bes== NULL)
						return 0; 
					Ptr<DeviceEnergyModel> dem = 
						bes->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
					double init_e = bes->GetInitialEnergy();
					double total_e = dem->GetTotalEnergyConsumption();
					//double remain_e = bes->GetRemainingEnergy();

					if (init_e >= total_e)
						return init_e - total_e;
					else
						return 0;
				}
			}
		}
		// End topology learning
	}
	return 0;
}

void
UfloodWifiMac::BatchAck(uint64_t bitmap)
{
		m_cumm_ack |= bitmap; 
		//NS_LOG_INFO("Cummulative ACK Bitmap: " << std::hex << bitmap << " ACK: " << m_cumm_ack << std::dec);  
		for (uint32_t i=0; i < l_topo.GetN(); i++){
				if (m_cumm_ack & (1 << i)){
						//NS_LOG_INFO("Node " << i);
						l_topo.Get(i)->SetRank(m_MNC_K);	
						l_topo.Get(i)->SetBitmap(0xffff);
						l_topo.Get(i)->SetInter(0);
				}
		}
}
//gjlee
void
UfloodWifiMac::SetCharging(bool charging)
{
	m_charging = charging;
}
bool
UfloodWifiMac::GetCharging(void)
{
	return m_charging;
}

double
UfloodWifiMac::GetTotalTime(void)
{
		return total_time;
}

double 
UfloodWifiMac::GetTrainingEnergy(void)
{
		return training_energy;
}
void
UfloodWifiMac::SetTrainingEnergy(double energy)
{
		training_energy = energy;
}


// TO DO : Packet Size 

} // namespace ns3
