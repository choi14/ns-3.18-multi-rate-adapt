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
#ifndef UFLOOD_WIFI_MAC_H
#define UFLOOD_WIFI_MAC_H

#include "regular-wifi-mac.h"
#include "topology-result.h"
//#include "topology-learning.h"
#include "amsdu-subframe-header.h"
#include "MNC_codec.h"
#include "wifi-mac-queue.h"
#include "neighbor-info.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 *
 */
class UfloodWifiMac : public RegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  UfloodWifiMac ();
  virtual ~UfloodWifiMac ();

  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);

  /**
   * \param linkUp the callback to invoke when the link becomes up.
   */
  virtual void SetLinkUpCallback (Callback<void> linkUp);

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to);

  bool IsSrc(void);
  void SendFeedback(void);
  void SendBatchAck (Mac48Address to);
  bool IsAllReceived (void);
  //std::vector<Ptr<NodeInfo> > GetResults(void);
/*
  uint16_t CalculateUtility(void);
  double GetUtility(void);
  void IsBest(void);

  void StartNewBatch (void); 
 // void FinishBatch (void);
  void SendBurst (uint16_t burst);
  void FeedbackHandling (void);

 //topology-learning
 */
  void SetTopo (Ptr<TopologyResult> result);
  Ptr<NodeInfo> GetNodeInfo(void);
  Ptr<TopologyResult> GetTopo(void);
  void SendTraining (void);
  void RunDijkstra(void);
  uint8_t GetBurstSize(bool init);
  uint8_t CalculateL (uint16_t bitmap_a, uint16_t bitmap_b, uint8_t rank_a, uint8_t rank_b);
  void SetBasicModes(void);

  Ptr<TopologyResult> GetResults(void);
  void ReceiveTl (Ptr<Packet> packet, Mac48Address from);
  void CalculateRate(void);
  uint8_t GetRate(void);
  std::vector<NbrItem> GenerateNbrInfo(void);
  double CalcTxTime(uint8_t rate, uint8_t num);
  void ResetTopo(void);
  uint16_t SetBit(uint16_t bit, uint8_t seq);
  void FeedbackInterpolation (Mac48Address from, uint8_t send, uint16_t bitmap, bool src);
  void CalculateUtility(void);
  double GetMbps(uint8_t rate);
  void RelayIfBest(void);
  void SendBurst (void);
  uint8_t GetFeedbackRate(void);
  void CheckChild(void);
  void EndBatch(void);
  double GetNodeEnergy(Mac48Address addr);
  void BatchAck(uint64_t bitmap); // to be
	void SetCharging(bool charging);
	bool GetCharging(void);
	double GetTotalTime(void);  //in second;

	double GetTrainingEnergy(void);
	void SetTrainingEnergy(double energy);

private:
  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr);

  bool m_src;
  Ptr<TopologyResult> m_topo;
  Ptr<NodeInfo> m_nodeinfo;  // m topo is connected with m_nodeinfo // my mistake
  
  TopologyResult l_topo; //local topology info
  NodeInfo l_nodeinfo; //local nodeinfo
  
  Mac48Address cur_tf_addr;
  Mac48Address src_addr;
  Ptr<WifiMacQueue> m_queue;
  MNC_Encoder m_MNC_Encoder;
  MNC_Decoder m_MNC_Decoder;
  EventId m_feedbackEvent;
  EventId m_relayEvent;
  EventId m_utilityEvent;
  EventId m_childEvent;

  //uint8_t m_rank;
  //uint16_t m_bitmap;
  uint32_t m_max;
  uint8_t m_rate;
  uint8_t m_MNC_K;
  uint8_t m_MNC_P;
  bool m_doing;
  uint16_t m_eid;
  bool m_succ;
  uint16_t m_burst;
  
  uint64_t m_cumm_ack;
  uint8_t m_id;
  uint8_t m_data_rate;
  uint8_t m_utility_type;
  
  double m_alpha;
  double m_omega;
  bool m_charging; // gjlee

  double init_time;
  double total_time;

  double training_energy;

};

} // namespace ns3

#endif /* ADHOC_WIFI_MAC_H */
