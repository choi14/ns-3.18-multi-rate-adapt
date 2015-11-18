#include "online-table-manager.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "wifi-mode.h"

NS_LOG_COMPONENT_DEFINE ("OnlineTableManager");


namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (OnlineTableManager);


TypeId
OnlineTableManager::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::OnlineTableManager")
			.SetParent<Object> ()
			.AddConstructor<OnlineTableManager> ()
			.AddAttribute("Alpha", "alpha value for EWMA",
							DoubleValue(0.9),
							MakeDoubleAccessor(&OnlineTableManager::alpha),
							MakeDoubleChecker<double> ())
			.AddAttribute("SampleType", "0: Time-based 1: Number-based",
							UintegerValue(1),
							MakeUintegerAccessor(&OnlineTableManager::m_type),
							MakeUintegerChecker<uint8_t> ())
			.AddAttribute("SamplingNum", "Number of samples",
							UintegerValue(40),
							MakeUintegerAccessor(&OnlineTableManager::m_sampling_num),
							MakeUintegerChecker<uint16_t> ())
			;

	return tid;
}

OnlineTableManager::OnlineTableManager():
		m_rssi_cur(0),
		m_pdr_cur(0),
		m_mcs_cur(0)
{
		m_init = false;
	 	threshold_high = 0.95;
	 	threshold_low = 0.05;
		rssi_min = 0;
		m_rcv = 0;
		m_tot = 0;
		m_first_seq = 0;
		m_transition = 2;
		//m_last_seq = 0;
		m_training_max = 100;
		m_min_num_samples = 5;

		sampling_period = Time(MilliSeconds(20));
		
		for (uint32_t i = 0; i < 8; i++){
				for (uint32_t j=0; j < 30; j++){
						m_table[i][j].p = 0;
						m_table[i][j].mark = false;
						m_correct_table[i][j].p = 0;
						m_correct_table[i][j].mark = true;
				}
		}
}


OnlineTableManager::~OnlineTableManager()
{
}

void
OnlineTableManager::InitialTable (uint16_t seq, uint16_t id, uint8_t mcs, uint32_t rssi){
	if (m_init == false){
		//sample_init_time = Simulator::Now();
		m_rssi_cur = rssi;
		m_mcs_cur = mcs;
		m_first_seq = 0;
		m_id = id;
	
		m_rcv = 1;
		m_tot = seq;
		m_init = true;
			
		m_timeoutEvent = Simulator::Schedule (sampling_period, &OnlineTableManager::SamplingTimeout, this);
		return;
	}
	if (mcs != m_mcs_cur){
			NS_LOG_INFO("New mcs is comming " << (uint32_t)mcs);
			if (m_timeoutEvent.IsRunning())
					m_timeoutEvent.Cancel();
				
			// if (m_tot > theshold){} // to be
			m_tot = m_training_max - m_first_seq + 1;
	
			if (m_tot > m_min_num_samples)
				RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_tot);

			m_mcs_cur = mcs;
			m_rssi_cur = rssi;
			m_first_seq = 0;
			m_rcv = 1;
			m_tot = seq;
			m_id = id;

			m_timeoutEvent = Simulator::Schedule (sampling_period, &OnlineTableManager::SamplingTimeout, this);
			return;
	}

	if (id == m_id){
		if (!m_timeoutEvent.IsRunning()){
			m_rssi_cur = rssi;
			m_first_seq = seq;
			m_rcv = 1;
			m_tot = 1;
		
			m_timeoutEvent = Simulator::Schedule (sampling_period, &OnlineTableManager::SamplingTimeout, this);
		}
		else{
			if (m_type == 1 && seq - m_first_seq >= m_sampling_num ){
				NS_LOG_INFO("Seq: " << seq << " FirstSeq: " << m_first_seq << " Avg rssi: " << m_rssi_cur / m_rcv  );
				m_tot = m_sampling_num;
				

				if (m_tot > m_min_num_samples)
					RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_tot);
				
				m_rssi_cur = rssi;
				m_tot = seq - m_first_seq - m_sampling_num + 1;
				m_rcv = 1;
				m_first_seq = seq - m_tot + 1;
		
				if (m_timeoutEvent.IsRunning()){
					m_timeoutEvent.Cancel();
				}
				
				m_timeoutEvent = Simulator::Schedule (sampling_period, &OnlineTableManager::SamplingTimeout, this);
			}
			else{
				m_rssi_cur += rssi;	
				m_rcv++;
				//m_last_seq = seq;
				m_tot = seq - m_first_seq + 1;
			}
		}
	}
	else{
		if (m_timeoutEvent.IsRunning())
			m_timeoutEvent.Cancel();
		
				if (m_tot > m_min_num_samples)
				RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_tot);
			
			m_mcs_cur = mcs;
			m_rssi_cur = rssi;
			m_first_seq = 0;
			m_rcv = 1;
			m_id = id;
			m_tot = seq;
		
			m_timeoutEvent = Simulator::Schedule (sampling_period, &OnlineTableManager::SamplingTimeout, this);
	}
}
/*
void
OnlineTableManager::UpdateTable (uint32_t seq, uint32_t id, uint8_t mcs, uint32_t rssi){

}
*/
void
OnlineTableManager::SamplingTimeout (void){
	double pdr = (double)m_rcv / (double)m_tot;
	uint32_t rssi_avg = m_rssi_cur / m_rcv;
	
	NS_LOG_INFO("Sampling Timeout Pdr: " << pdr << " Rssi: " << rssi_avg  << " rcv: " << m_rcv << " tot: " << m_tot << " mcs: " << (uint32_t)m_mcs_cur); 
	
	if (m_tot > m_min_num_samples)
		RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_tot);
}

void
OnlineTableManager::RecordSample (uint8_t mcs, uint32_t rssi_sum, uint16_t rcv, uint16_t tot){
	uint32_t rssi = rssi_sum / (uint32_t)rcv;
	double pdr = (double)rcv / (double)tot;
	
	uint32_t rssi_idx = rssi - rssi_min;
	uint32_t mcs_idx = (uint32_t)mcs;

	if (rssi_idx >= 30){
			NS_LOG_INFO("High RSSI MCS: " << mcs_idx << " IDX: " << rssi_idx <<  " RSSI_SUM: " << rssi_sum << " RCV: " << rcv << " TOT: " << tot);
			return;
	}

	Pdr pdr_cur = m_table[mcs_idx][rssi_idx];

	if (pdr_cur.mark == false){
			pdr_cur.p = pdr;
			pdr_cur.mark = true;
	}
	else
			pdr_cur.p = alpha*pdr_cur.p + (1-alpha)*pdr;

	if (pdr_cur.p > threshold_high)
			pdr_cur.p = threshold_high;
	else if (pdr_cur.p < threshold_low)
			pdr_cur.p = 0; // To be
	
	for (uint32_t i = 0; i < rssi_idx; i++){
		if((m_table[mcs][i]).p > pdr_cur.p && m_table[mcs][i].mark == true){
			pdr_cur.p = (m_table[mcs][i]).p;
			break;
		}
	}
	(m_table[mcs_idx][rssi_idx]).p = pdr_cur.p;
	(m_table[mcs_idx][rssi_idx]).mark = pdr_cur.mark;
}

void
OnlineTableManager::PrintTable(Pdr table[][30], std::ostream &os){
	os.precision (2);
	for (uint32_t i = 0; i < 8; i++){
		os << "MCS " << i << " Rcv: (";	
		for (uint32_t j=0; j<30; j++){
				os << (table[i][j]).p << " ";
		}

		os << ")" << std::endl;
	}
	os << std::endl;
}

void
OnlineTableManager::SetTrainingMax(uint16_t max){
		m_training_max = max;
}

void
OnlineTableManager::GenerateCorrectTable(void)
{
		for (uint32_t i = 0; i < 8; i++){
			WifiMode mode = m_manager->GetBasicMode(i);
			
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

			uint32_t databytes = 1400;
			double nSymbols = ((databytes+64)*8+22)/coderate/ofdmbits;
			uint32_t nbits = ((uint32_t)nSymbols +1)*ofdmbits;

			for (uint32_t j = 0; j < 30; j++){
					double snr = (double)j + (double)rssi_min;
					snr = std::pow(10.0, snr/10.0);
					double pdr = m_phy->CalculatePdr (mode, snr, nbits);
					
					if (pdr < 0.001)
							pdr = 0;
					
					m_correct_table[i][j].p = pdr;
			}
		}
		
//		PrintTable(m_correct_table, std::cout);
}

void 
OnlineTableManager::SetRemoteStation(Ptr<WifiRemoteStationManager> manager)
{
	m_manager = manager;
}
void 
OnlineTableManager::SetPhy(Ptr<WifiPhy> phy)
{
	m_phy = phy;
}

void
OnlineTableManager::PrintOnlineTable(std::ostream &os){
	PrintTable(m_table, os);
}

void
OnlineTableManager::FillingBlank(void){
		uint32_t snr_edge = 0;

		PrintOnlineTable(std::cout);
		
		std::cout << " After Filling " << std::endl;
		for (uint32_t i = 0; i < 8; i++){
				uint32_t mcs_idx = 7-i;

				uint32_t snr_low_id = 0;
				double snr_low_value = 1;
				uint32_t snr_high_id = 0;
				double snr_high_value =0;

				bool no_low = true;
				bool no_high = true;

				for (uint32_t j=0; j < 30; j++){
						double pdr = m_table[mcs_idx][j].p;
						bool mark = m_table[mcs_idx][j].mark;

						if (mark == true && pdr == threshold_low){
								no_low = false;
						}

						if (mark == true && pdr <= snr_low_value){
								snr_low_id = j;
								snr_low_value = pdr;
						}
						
						if (mark == true && pdr > snr_high_value){
								if (snr_high_value != 0){
										for (uint32_t k = snr_high_id + 1; k < j; k++){
												m_table[mcs_idx][k].p = snr_high_value +(double)(k-snr_high_id)*(pdr - snr_high_value)/(double)(j-snr_high_id);
										}
								}
								snr_high_value = pdr;
								snr_high_id = j;
						}
						if (mark == true && pdr == threshold_high){
							//NS_LOG_UNCOND("ID: " << j);
							for (uint32_t k = j+1; k < 30; k++){
								m_table[mcs_idx][k].p = threshold_high;
							}
							snr_high_value = pdr;
							snr_high_id = j;
							
							no_high = false;
							break;
						}
				}
				if (no_low == true && no_high == true){
						NS_LOG_UNCOND("MCS: " << mcs_idx << " High ID: " << snr_high_id << " Low ID: " << snr_low_id); 
						if (snr_low_id == snr_high_id){
							if (snr_high_id == 0)
									continue;

							NS_LOG_INFO("Single sample");
								
							for (int k = (int)snr_high_id; k >= 0; k--){
								m_table[mcs_idx][k].p = snr_high_value - (double)(snr_high_id-k)*(threshold_high /(double)m_transition);
								if (m_table[mcs_idx][k].p < threshold_low){
										m_table[mcs_idx][k].p = 0;
										break;
								}
							}
							
							for (int k = (int)snr_high_id; k < 30; k++){
								m_table[mcs_idx][k].p = snr_high_value + (double)(k - snr_high_id)*(threshold_high /(double)m_transition);
								if (m_table[mcs_idx][k].p >= threshold_high){
										m_table[mcs_idx][k].p = threshold_high;
								}
							}
						}

						else{
							double slope = (snr_high_value - snr_low_value) / (snr_high_id - snr_low_id);

							for (uint32_t k = 0; k < snr_low_id; k++){
									if ( k + m_transition < snr_low_id )
											m_table[mcs_idx][k].p = 0;
									else{
											m_table[mcs_idx][k].p = snr_low_value - (double)(snr_low_id-k)*slope;
											if (m_table[mcs_idx][k].p < threshold_low){
													m_table[mcs_idx][k].p = 0;
											}
									}
							}
							
							for (uint32_t k = snr_high_id+1; k < 30; k++){
								m_table[mcs_idx][k].p = snr_high_value + (double)(k - snr_high_id)*slope;
								if (m_table[mcs_idx][k].p >= threshold_high){
										m_table[mcs_idx][k].p = threshold_high;
								}
							}
						}
				}
				else if (no_low == true){
						//NS_LOG_UNCOND("MCS: " << mcs_idx << " High ID: " << snr_high_id << " Low ID: " << snr_low_id); 
						double slope = 0;
						if (snr_high_id == snr_low_id){
							if (snr_edge == 0){
									snr_edge = snr_high_id;
							}
							else{
									if (snr_high_id >= snr_edge){
										for (uint32_t k=snr_edge-1; k < snr_high_id;	k++){
											m_table[mcs_idx][k].p = threshold_high;
										}
										snr_high_id = snr_edge-1;
										snr_low_id = snr_edge-1;
										snr_edge = snr_high_id;
									}
							}

							slope = threshold_high / (double)m_transition;
						}
						else{
							slope = (snr_high_value - snr_low_value) / (double)(snr_high_id - snr_low_id);
						}
						
						for (uint32_t k = snr_low_id - m_transition; k < snr_low_id; k++){
								m_table[mcs_idx][k].p = snr_low_value - (double)(snr_low_id-k)*slope;
								if (m_table[mcs_idx][k].p < threshold_low){
										m_table[mcs_idx][k].p = 0;
								}
						}
				}
				else if (no_high == true){

				}

		}

		PrintOnlineTable(std::cout);
}






}
