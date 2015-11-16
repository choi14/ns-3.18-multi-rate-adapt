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
				DoubleValue (0.1),
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
	m_initialize = false;
	m_setMacLowValue = false;
	m_feedbackPeriod = 100;
  NS_LOG_FUNCTION (this);

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
AdhocWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();
	if (hdr->IsData ())
	{
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
			ForwardUp (packet, from, to);
		}
		else
		{
			ForwardUp (packet, from, to);
		}
		return;
	}
	else if (hdr->IsFeedback ())
	{
		FeedbackHeader fbhdr;
		packet->RemoveHeader (fbhdr);
		m_rxInfoSet.Rssi = (int32_t)fbhdr.GetRssi();
		m_rxInfoSet.Snr = (int32_t)fbhdr.GetSnr();
		m_rxInfoSet.LossPacket = fbhdr.GetLossPacket();
		m_rxInfoSet.TotalPacket = fbhdr.GetTotalPacket();
		
		Ptr<SbraWifiManager> sbra = DynamicCast<SbraWifiManager> (GetWifiRemoteStationManager());
		sbra->UpdateInfo(from, m_rxInfoSet);

		NS_LOG_INFO ("[rx feedback packet]" << " Address: " << from << " RSSI: " << m_rxInfoSet.Rssi << " Snr: " << 
				m_rxInfoSet.Snr << " LossPacket: " << m_rxInfoSet.LossPacket << " TotalPacket: " << m_rxInfoSet.TotalPacket);
	 	return;
		//ForwardUp (originPacket, from, to);
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
  // Set alpha
	// m_low -> SetAlpha (m_alpha);
	m_rxInfoGet = m_low->GetRxInfo (m_fbtype, m_percentile, m_alpha, m_beta, m_eta, m_delta, m_rho);
  Ptr<Packet> packet = Create<Packet> ();
  FeedbackHeader FeedbackHdr; // Set RSSI, SNR, txPacket, TotalPacket
	FeedbackHdr.SetRssi (m_rxInfoGet.Rssi);
  FeedbackHdr.SetSnr (m_rxInfoGet.Snr);
  FeedbackHdr.SetLossPacket (m_rxInfoGet.LossPacket);
  FeedbackHdr.SetTotalPacket (m_rxInfoGet.TotalPacket);
	packet->AddHeader (FeedbackHdr);

	NS_LOG_INFO ("[tx feedback packet]" << " RSSI: " << m_rxInfoGet.Rssi << " Snr: " << m_rxInfoGet.Snr <<
			" LossPacket: " << m_rxInfoGet.LossPacket << " TotalPacket: " << m_rxInfoGet.TotalPacket);
  m_dca->Queue (packet, hdr);
  Simulator::Schedule (MilliSeconds(m_feedbackPeriod), &AdhocWifiMac::SendFeedback, this);
}

} // namespace ns3
