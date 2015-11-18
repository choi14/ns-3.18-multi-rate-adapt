#ifndef ONLINE_TABLE_MANAGER_H
#define ONLINE_TABLE_MANAGER_H

#include "ns3/mac48-address.h"
#include <stdint.h>
#include <vector>
#include "wifi-mode.h"
#include "wifi-remote-station-manager.h"
#include "fb-headers.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/object.h"
#include "wifi-remote-station-manager.h"
#include "wifi-phy.h"

namespace ns3 {

typedef struct{
	double p;
 	bool mark;	
}Pdr;

class OnlineTableManager : public Object
{
public:
  static TypeId GetTypeId (void);
  OnlineTableManager ();
  virtual ~OnlineTableManager ();
  void InitialTable (uint16_t seq, uint16_t id, uint8_t mcs, uint32_t rssi);
//  void UpdateTable (uint32_t seq, uint32_t id, uint8_t mcs, uint32_t rssi); 
  void RecordSample (uint8_t mcs, uint32_t rssi_sum, uint16_t m_rcv, uint16_t m_tot);
  void SamplingTimeout(void);
  void PrintTable(Pdr table[][30], std::ostream &os);
  void SetTrainingMax(uint16_t max);
  void GenerateCorrectTable(void);
  void SetRemoteStation(Ptr<WifiRemoteStationManager> manager);
  void SetPhy(Ptr<WifiPhy> phy);
  void PrintOnlineTable(std::ostream &os);
  //double GetPdr(uint8_t mcs, uint32_t rssi);
  void FillingBlank(void);

private:
	Pdr m_table[8][30];
	Pdr m_correct_table[8][30];
	uint32_t m_rssi_cur;
	double m_pdr_cur;
	uint16_t m_rcv;
	uint16_t m_tot;
	uint16_t m_first_seq;
	//uint32_t m_last_seq;
	uint16_t m_id;
	uint16_t m_training_max;
	double alpha;

	uint8_t m_mcs_cur;
	uint32_t rssi_min;
	uint32_t m_transition; // in db


	double threshold_high;
	double threshold_low;
	bool m_init;
	uint8_t m_type;
	uint16_t m_sampling_num;
	uint16_t m_min_num_samples;
//	Time sample_init_time;
	Time sampling_period;
	EventId m_timeoutEvent;

	Ptr<WifiRemoteStationManager> m_manager;
	Ptr<WifiPhy> m_phy;

};

} // namespace ns3

#endif /* ONLINE_TABLE_MANAGER_H */
