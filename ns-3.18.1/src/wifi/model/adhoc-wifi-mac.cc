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
#include "adhoc-wifi-mac.h"

#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"

#include "qos-tag.h"
#include "mac-low.h"
#include "dcf-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "msdu-aggregator.h"
#include "amsdu-subframe-header.h"
#include "mgt-headers.h"
#include "fb-headers.h"
#include "sbra-wifi-manager.h"

#include "wifi-preamble.h"
#include "tl-header.h"
#include "rate-tag.h"
#include "coefficient-header.h"
#include "ns3/random-variable-stream.h"
//#include "coefficient-header.h"

NS_LOG_COMPONENT_DEFINE ("AdhocWifiMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AdhocWifiMac);

TypeId
AdhocWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AdhocWifiMac")
    .SetParent<RegularWifiMac> ()
    .AddConstructor<AdhocWifiMac> ()
		.AddAttribute ("FeedbackType",
				"Type of Feedback",
				UintegerValue (0),
				MakeUintegerAccessor (&AdhocWifiMac::m_fbtype),
				MakeUintegerChecker<uint32_t> ())
		.AddAttribute ("FeedbackPeriod",
				"Period of Feedback",
				UintegerValue (100),
				MakeUintegerAccessor (&AdhocWifiMac::m_feedbackPeriod),
				MakeUintegerChecker<uint64_t> ())
		.AddAttribute ("RateAdaptType",
				"Type of rate adaptation",
				UintegerValue (0),
				MakeUintegerAccessor (&AdhocWifiMac::m_ratype),
				MakeUintegerChecker<uint32_t> ())
		.AddAttribute ("PerThreshold",
				"The maximum Packet Error Rate acceptable at any transmission mode",
				DoubleValue (0.001),
				MakeDoubleAccessor (&AdhocWifiMac::m_per),
				MakeDoubleChecker<double> ())
		.AddAttribute ("Alpha",
				"weighting factor of EWMA",
				DoubleValue (0.5),
				MakeDoubleAccessor (&AdhocWifiMac::m_alpha),
				MakeDoubleChecker<double> ())
		.AddAttribute ("Beta",
				"weighting factor of avg and stddev",
				DoubleValue (0.5),
				MakeDoubleAccessor (&AdhocWifiMac::m_beta),
				MakeDoubleChecker<double> ())
		.AddAttribute ("Percentile",
				"percentile",
				DoubleValue (0.9),
				MakeDoubleAccessor (&AdhocWifiMac::m_percentile),
				MakeDoubleChecker<double> ())
		.AddAttribute ("Eta",
				"eta",
				DoubleValue (1.0),
				MakeDoubleAccessor (&AdhocWifiMac::m_eta),
				MakeDoubleChecker<double> ())
		.AddAttribute ("Delta",
				"delta",
				DoubleValue (0.1),
				MakeDoubleAccessor (&AdhocWifiMac::m_delta),
				MakeDoubleChecker<double> ())
		.AddAttribute ("Rho",
				"rho",
				DoubleValue (0.1),
				MakeDoubleAccessor (&AdhocWifiMac::m_rho),
				MakeDoubleChecker<double> ())
  ;
  return tid;
}

AdhocWifiMac::AdhocWifiMac ()
{
	NS_LOG_FUNCTION (this);
	m_initialize = false;
	m_setMacLowValue = false;
	m_feedbackPeriod = 100;
	m_addBasicModes = 0;
	m_minSnr = 0.0;
	m_minSnrLinear = 0.0;
	m_minNT = 0.0;
	m_max = 100;
	m_tf_id = 0;
	m_nc_enabled = true;
	m_mcast_mcs = 0;
	m_burstsize = 20;
	m_src_eid = 0;
	m_last_eid = 0;
	m_MNC_K = 10;
	m_MNC_P = 1;
	m_k = 10;	

	m_tableManager = CreateObject<OnlineTableManager> ();

	// Let the lower layers know that we are acting in an IBSS
  SetTypeOfStation (ADHOC_STA);
}

AdhocWifiMac::~AdhocWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
AdhocWifiMac::SetAddress (Mac48Address address)
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
AdhocWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (m_stationManager->IsBrandNew (to))
    {
      // In ad hoc mode, we assume that every destination supports all
      // the rates we support.
      for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
        {
          m_stationManager->AddSupportedMode (to, m_phy->GetMode (i));
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

	if(m_initialize == false)
	{
		m_low->SetAlpha(m_alpha);
		m_low->SetEDR(m_eta, m_delta, m_rho);
		m_initialize = true;
	}
  if (m_nc_enabled){
		  bool sys = true;
		  m_MNC_Encoder.MNC_Enqueue(packet, hdr);

		  if(m_MNC_Encoder.GetBufferSize() == m_MNC_K){
				  m_src_eid++;
				  NS_LOG_ERROR("Source send, eid: " << m_src_eid << " m_burstsize: " << (uint16_t)m_burstsize << " MCS: " << (uint16_t)m_mcast_mcs);
						  m_MNC_Encoder.Encoding(m_src_eid, m_MNC_K, m_burstsize, m_MNC_P, sys);
						  for(uint8_t i = 0; i < m_burstsize; i++){ 
								  WifiMacHeader temphdr;
								  Ptr<const Packet> encoded_packet = m_MNC_Encoder.MNC_Dequeue(&temphdr);

								  RateTag tag(m_mcast_mcs); // mcs
								  ConstCast<Packet> (encoded_packet)->AddPacketTag(tag);
								  if(m_qosSupported){
										  NS_ASSERT (tid < 8);
										  NS_LOG_DEBUG(temphdr.GetAddr1() << "edca Queue");
										  temphdr.SetWep(1);
										  m_edca[QosUtilsMapTidToAc(tid)]->Queue(encoded_packet, temphdr);
								  }
								  else{
										  NS_LOG_DEBUG("dca Queue");
										  temphdr.SetWep(1);
										  m_dca->Queue(encoded_packet, temphdr);
								  }
						  }
				  m_MNC_Encoder.FlushAllBuffer();
		  }
  }
  else{
		  if (m_qosSupported)
		  {
				  // Sanity check that the TID is valid
				  NS_ASSERT (tid < 8);
				  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
		  }
		  else
		  {
				  m_dca->Queue (packet, hdr);
		  }
  }
}

void
AdhocWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  // The approach taken here is that, from the point of view of a STA
  // in IBSS mode, the link is always up, so we immediately invoke the
  // callback if one is set
  linkUp ();
}

void
AdhocWifiMac::UpdateInfo (Mac48Address addr, struct rxInfo info)
{
	bool exist = false;
	for (uint32_t i = 0; i < m_infos.size(); i++)
	{
		if (m_infos[i].addr == addr){
			exist = true;
			m_infos[i].info = info;
			break;
		}
	}
	if (exist == false)
	{
		StaInfo stainfo;
		stainfo.addr = addr;
		stainfo.info = info;
		m_infos.push_back(stainfo);
	}
	/*
	for (uint32_t i = 0; i < m_infos.size(); i++){
		NS_LOG_ERROR("Addr " << m_infos[i].addr); 
	}
	*/
}

void
AdhocWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();


  if (hdr->IsFeedback ())
  {
		  FeedbackHeader fbhdr;
		  packet->RemoveHeader (fbhdr);
		  m_rxInfoSet.Rssi = fbhdr.GetRssi();
		  m_rxInfoSet.Snr = fbhdr.GetSnr();
		  m_rxInfoSet.LossPacket = fbhdr.GetLossPacket();
		  m_rxInfoSet.TotalPacket = fbhdr.GetTotalPacket();

		  NS_LOG_INFO ("[rx feedback packet]" << " Address: " << to << " RSSI: " << m_rxInfoSet.Rssi
					<< " Snr: " << m_rxInfoSet.Snr << " LossPacket: " 
					<< m_rxInfoSet.LossPacket << " TotalPacket: " << m_rxInfoSet.TotalPacket);
			
			// GroupRate
		  UpdateInfo(from, m_rxInfoSet);
			m_mcast_mcs = GroupRateAdaptation ();

		  return;
  }
  
  if (hdr->IsData ())
  {
		  if (hdr->IsMoreData()){
		  		ReceiveTl(packet, from);
		  		return;
		  }

		  if (hdr->IsQosData () && hdr->IsQosAmsdu ())
		  {
				  NS_LOG_DEBUG ("Received A-MSDU from" << from);
				  DeaggregateAmsduAndForward (packet, hdr);
		  }
		  else if (to.IsGroup ())
		  {
				  if (m_initialize == false)
				  {
						  m_srcAddress = from; // jychoi source address
						  m_initialize = true; 
						  SendFeedback ();
				  }

				  if (hdr->IsWep()){
						  ReceiveNC(packet, hdr);
			    	}
				  else{
						  ForwardUp (packet, from, to);
				  }
		  }
		  else
		  {
				  ForwardUp (packet, from, to);
		  }
		  return;
  }

	// Invoke the receive handler of our parent class to deal with any
  // other frames. Specifically, this will handle Block Ack-related
  // Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

// jychoi
void
AdhocWifiMac::SendFeedback ()
{
  NS_LOG_FUNCTION (this);
  
	WifiMacHeader hdr;
  hdr.SetFeedback ();
  hdr.SetAddr1 (m_srcAddress);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
	
	m_rxInfoGet = m_low->GetRxInfo (m_fbtype, m_percentile, m_alpha, m_beta, m_eta, m_delta, m_rho);
  Ptr<Packet> packet = Create<Packet> ();
  FeedbackHeader FeedbackHdr; // Set RSSI, SNR, txPacket, TotalPacket
	FeedbackHdr.SetRssi (m_rxInfoGet.Rssi);
  FeedbackHdr.SetSnr (m_rxInfoGet.Snr);
  FeedbackHdr.SetLossPacket (m_rxInfoGet.LossPacket);
  FeedbackHdr.SetTotalPacket (m_rxInfoGet.TotalPacket);
	packet->AddHeader (FeedbackHdr);

	NS_LOG_INFO ("[tx feedback packet]" << " Address " << m_low->GetAddress () << " RSSI: " << m_rxInfoGet.Rssi << " Snr: " << m_rxInfoGet.Snr <<
			" LossPacket: " << m_rxInfoGet.LossPacket << " TotalPacket: " << m_rxInfoGet.TotalPacket);
  m_dca->Queue (packet, hdr);
  Simulator::Schedule (MilliSeconds(m_feedbackPeriod), &AdhocWifiMac::SendFeedback, this);
}

// jychoi
uint8_t
AdhocWifiMac::GroupRateAdaptation ()
{
	// Add Basic Mode
	if (m_addBasicModes == 0)
	{
		SetBasicModes ();
		m_addBasicModes = 1;
	}

	uint32_t vsize = m_infos.size();
	NS_LOG_INFO("vsize: " << vsize);

	if(vsize == 0)
		m_GroupTxMcs = 0;

	else
	{
		for (uint32_t m = 0; m < vsize; m++)
		{
			if((double)m_infos[m].info.Snr > 3.0)
			{
				m_minSnr = (double)m_infos[m].info.Snr;
				break;
			}
		}

		for (uint32_t m = 0; m < vsize; m++)
		{
			if( (m_minSnr > (double)m_infos[m].info.Snr) && ((double)m_infos[m].info.Snr > 3.0) )
			{
				m_minSnr = (double)m_infos[m].info.Snr;
			}
		}

		NS_LOG_INFO("m_minSnr(dB): " << m_minSnr <<
				" m_minSnr(ratio): " << std::pow(10.0, m_minSnr/10.0)); 

		if(m_minSnr > 3.0)
		{
			m_firstnt = true;
			m_minSnrLinear = std::pow (10.0, m_minSnr/10.0); // m_minSnrLinear: log->linear
			uint32_t NBasicMode = m_stationManager->GetNBasicModes ();
			double Per = 1.0;
			double Pdr = 0.0;			
			double maxPdrRate = 0.0;
			double PdrRate = 0.0;
			double Rate = 0.0;

			for (uint32_t k = 0; k < NBasicMode; k++)
			{
				WifiMode mode = m_stationManager->GetBasicMode(k);

				double coderate = 1.0;
				if(mode.GetCodeRate () == WIFI_CODE_RATE_3_4)
					coderate = 3.0/4.0;
				else if(mode.GetCodeRate () == WIFI_CODE_RATE_2_3)
					coderate = 2.0/3.0;
				else if(mode.GetCodeRate () == WIFI_CODE_RATE_1_2)
					coderate = 1.0/2.0;
				else if(mode.GetCodeRate () == WIFI_CODE_RATE_5_6)
					coderate = 5.0/6.0;

				uint32_t ofdmbits = 48; // Coded bits per OFDM symbols
				if(mode.GetPhyRate () == 12000000)
					ofdmbits = 48;
				else if(mode.GetPhyRate () == 24000000)
					ofdmbits = 96;
				else if(mode.GetPhyRate () == 48000000)
					ofdmbits = 192;
				else if(mode.GetPhyRate () == 72000000)
					ofdmbits = 288;

				uint32_t databits = 1000;
				double nSymbols = ((databits+80)*8+22)/coderate/ofdmbits;
				uint32_t nbits = ((uint32_t)nSymbols +1)*ofdmbits;

				// N*Duration Rate Adaptation 
				if(m_ratype == 0)
				{
					WifiPreamble preamble = WIFI_PREAMBLE_LONG;
					WifiTxVector v;
					v.SetMode (mode);
					v.SetTxPowerLevel (0);
					v.SetShortGuardInterval (false);
					v.SetNss (1);
					v.SetNess (0);
					v.SetStbc (false);

					Pdr = m_phy->CalculatePdr (mode, m_minSnrLinear, nbits);
					uint8_t tempBurstsize = KtoNTable[m_k-1][(int)(Pdr*100)];
					if((tempBurstsize < 13) && (tempBurstsize > 0))
					{
						tempBurstsize = 13;
					}
					NS_LOG_INFO("m_k: " << (uint16_t)m_k << " Pdr: " << (int)(Pdr*100) << " Pdr: " << Pdr);
					duration = m_phy->CalculateTxDuration(databits+80, v, preamble); 
					double nt = tempBurstsize * duration.GetMicroSeconds ();
					NS_LOG_INFO("Duration: " << duration << " m_burstsize: " << (uint32_t)m_burstsize << " nt: " << nt);
					
					if(m_firstnt == true)
					{
						m_GroupTxMode = mode;
						m_minNT = nt;
						m_burstsize = tempBurstsize;
						m_firstnt = false;
					}
					else
					{
						if ((nt < m_minNT) && (nt != 0))
						{
							m_GroupTxMode = mode;
							m_minNT = nt;
							m_burstsize = tempBurstsize;
						}
					}
				}//m_ratype == 0
				
				// PER-SNR Rate Adaptation
				else if(m_ratype == 1)
				{
					Pdr = m_phy->CalculatePdr (mode, m_minSnrLinear, nbits);
					double tempPer = 1-Pdr;
					NS_LOG_INFO("mode: " << mode.GetDataRate ()*0.000001 << " tempPer: " << tempPer);
					if(tempPer < m_per)
					{
						m_GroupTxMode = mode;
						Per = tempPer;
					}

					if (Per == 1)
						m_GroupTxMcs = 0;
					else if (Per > 1)
						NS_LOG_ERROR("Never happen");
					m_burstsize = KtoNTable[m_k-1][(uint32_t)(1-Per)*100];
				}//m_ratype == 1

				// Throughput-SNR Rate Adaptation
				else if (m_ratype == 2)
				{
					Pdr = m_phy->CalculatePdr (mode, m_minSnrLinear, nbits);
					Rate = mode.GetDataRate();
					PdrRate = Pdr*Rate*0.000001;

					if (PdrRate > maxPdrRate)
					{
						m_GroupTxMode = mode;
						maxPdrRate = PdrRate;
						Per = 1-Pdr;
					}
					if (maxPdrRate == 0)
					{
						m_GroupTxMcs = 0;
						Per = 1;
					}
					else if (maxPdrRate < 0)
						NS_LOG_ERROR("Never happen");
					m_burstsize = KtoNTable[m_k-1][(uint32_t)(1-Per)*100];
				}//m_ratype==2

			} // for - NBasicMod
		
			int tmpGroupTxMode = m_GroupTxMode.GetDataRate()*0.000001;
			switch (tmpGroupTxMode)
			{
				case 6:
					m_GroupTxMcs = 0;	break;
				case 9:
					m_GroupTxMcs = 1;	break;
				case 12:
					m_GroupTxMcs = 2;	break;
				case 18:
					m_GroupTxMcs = 3;	break;
				case 24:
					m_GroupTxMcs = 4;	break;
				case 36:
					m_GroupTxMcs = 5;	break;
				case 48:
					m_GroupTxMcs = 6; break;
				case 54:
					m_GroupTxMcs = 7;	break;
			}

			if(m_minNT == 0)
			{
				m_GroupTxMcs = 0;
				m_minNT = 30;
			}

			NS_LOG_INFO ("m_minSnr: " << m_minSnr
					<< " GroupTxDataRate: " <<  m_GroupTxMode.GetDataRate ()*0.000001<<" Mb/s"
					<< " GroupTxMcs: " << m_GroupTxMcs);
		} // if m_minSnr > 1
		else
		{
			NS_LOG_INFO("m_minSnr: " << m_minSnr << " < 1 and selectBasicMode");
			m_GroupTxMcs = 0;
			m_minNT = 30;
		}
	} // vsize != 0
	return m_GroupTxMcs;
}

void
AdhocWifiMac::SendTraining(void)
{
	NS_LOG_FUNCTION (this << m_max);
	NS_LOG_INFO("Send Training " << m_low->GetAddress());
	WifiMacHeader hdr;
	hdr.SetTypeData();
	hdr.SetAddr1(Mac48Address::GetMulticastPrefix());
	hdr.SetAddr2(m_low->GetAddress());
	hdr.SetAddr3(m_low->GetBssid());
	hdr.SetDsNotFrom();
	hdr.SetDsNotTo();
	hdr.SetMoreData(1);

	TlHeader tl;
	tl.SetType(TL_TF1);
	tl.SetK(m_max);
	
	Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
	uint16_t tmpId = (uint16_t) rand->GetInteger(0,0xffff);
	m_tf_id = (m_tf_id != tmpId ? tmpId : ~tmpId);
	
	for (uint8_t j=0; j<8; j++){
		for (uint32_t i = 0; i < m_max; i++)
		{
					Ptr<Packet> packet = Create<Packet>(1440);
					
					tl.SetSeq(i);
					tl.SetRate(j);
					tl.SetId(m_tf_id);
					RateTag rtag(j);

					packet->AddHeader(tl);
					packet->AddPacketTag(rtag);
					m_dca->Queue(packet, hdr);
		}
	}
}

void
AdhocWifiMac::ReceiveTl (Ptr<Packet> packet, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << from);
		
  TlHeader tl;
  packet->RemoveHeader(tl);

  switch(tl.GetType())
  {
		  case TL_TF1:
				 {
					uint8_t rate = tl.GetRate();
					uint32_t id = tl.GetId();
					uint32_t seq = tl.GetSeq();
					double snr =  m_low->GetRxSnr();
					uint32_t snr_db =(uint32_t)(10*std::log10(snr));

					NS_LOG_INFO("Rate: " << rate << " Id: " << id << " Seq: " << seq << " Snr: " << snr_db);
					m_tableManager->InitialTable(seq, id, rate, snr_db); 	
			
					break;
				 }
		default:
			  NS_ASSERT(false);
			  return;
  }
}

void
AdhocWifiMac::ReceiveNC (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  		Mac48Address from = hdr->GetAddr2 ();
  		Mac48Address to = hdr->GetAddr1 ();
		
		CoefficientHeader coeffi; 
		packet->PeekHeader(coeffi);

		uint16_t eid = coeffi.GetEid();

		if (eid <= m_last_eid)
				return;

		for(MNC_Decoder_QueueI iter = m_decoder_queue.begin(); iter != m_decoder_queue.end(); iter++){
				if(iter->GetEid() < eid){
						if (iter->GetBufferSize() > 0)
								NS_LOG_ERROR("New Eid coming. " << (uint16_t)iter->GetBufferSize() << " Packets are in Old buffer." );
						iter->Systematicing();

						uint8_t size = iter->GetSBufferSize();
						for(uint8_t i = 0; i < size; i++){
								WifiMacHeader remain_hdr;
								Ptr<const Packet> remain_packet = iter->MNC_Rx_SDequeue(&remain_hdr);
								Ptr<Packet> forward_packet = remain_packet->Copy();
								ForwardUp (forward_packet, from, to);
						} //if
						m_last_eid = iter->GetEid();
				} 
		}//for

		if (eid <= m_last_eid)
				return;
		MNC_Decoder_QueueI iter;
		for(iter = m_decoder_queue.begin(); iter != m_decoder_queue.end(); iter++){
				if(iter->GetEid() == eid){
						break;
				}
		}
		if(iter == m_decoder_queue.end()){
				MNC_Decoder decoder;
				decoder.SetEid(eid);
				decoder.MNC_Rx_Enqueue(packet, hdr);
				m_decoder_queue.push_back(decoder);
				return;
		}

		iter->MNC_Rx_Enqueue(packet, hdr);

		if(iter->GetBufferSize() >= m_MNC_K){
				NS_LOG_ERROR("Decoding Try: " << eid );

				if(!(iter->Decoding(m_MNC_K, m_MNC_P, false))){
						NS_LOG_ERROR("Decoding Failure: " << eid);
						return;
				}
				else{
						NS_LOG_ERROR("Decoding Success: " << eid);
				}

				for(uint8_t i = 0; i < m_MNC_K; i++){
						WifiMacHeader temphdr;
						Ptr<const Packet> decoded_packet = iter->MNC_Rx_DDequeue(&temphdr);
						Ptr<Packet> forward_packet = decoded_packet->Copy();
						ForwardUp (forward_packet, from, to);
				} //for
			
				m_last_eid = iter->GetEid();
		}
}

void
AdhocWifiMac::SetBasicModes(void)
{
	m_stationManager->AddBasicMode(WifiMode("OfdmRate6Mbps"));// 4
	m_stationManager->AddBasicMode(WifiMode("OfdmRate9Mbps"));// 5
	m_stationManager->AddBasicMode(WifiMode("OfdmRate12Mbps"));// 6
	m_stationManager->AddBasicMode(WifiMode("OfdmRate18Mbps"));// 7
	m_stationManager->AddBasicMode(WifiMode("OfdmRate24Mbps"));// 8
	m_stationManager->AddBasicMode(WifiMode("OfdmRate36Mbps"));// 9
	m_stationManager->AddBasicMode(WifiMode("OfdmRate48Mbps"));// 10
	m_stationManager->AddBasicMode(WifiMode("OfdmRate54Mbps"));// 11

	NS_LOG_UNCOND ("Number of Basic Modes: " << m_stationManager->GetNBasicModes());
	
	m_tableManager->SetRemoteStation(m_stationManager);
	m_tableManager->SetPhy(m_phy);
	m_tableManager->GenerateCorrectTable();
}

} // namespace ns3
