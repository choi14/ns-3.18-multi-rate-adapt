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
		m_nc_n = 20;
		//m_last_seq = 0;
		m_training_max = 100;
		m_min_num_samples = 5;

		sampling_period = Time(MilliSeconds(20));
		
		for (uint32_t i = 0; i < 8; i++){
				m_rssi_high[i] = 0;
				m_rssi_low[i] = 0;
				m_no_low[i] = true;
				m_no_middle[i] = true;
				m_no_high[i] = true;
				for (uint32_t j=0; j < 50; j++){
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
			m_tot = m_training_max - m_first_seq;
	
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
				//NS_LOG_INFO("Seq: " << seq << " FirstSeq: " << m_first_seq << " Avg rssi: " << m_rssi_cur / m_rcv  );
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

void
OnlineTableManager::UpdateTable (uint16_t seq, uint16_t id, uint8_t mcs, uint32_t rssi){
	NS_ASSERT(seq <= m_nc_n);

	if (id != m_id || mcs != m_mcs_cur){
			if (m_tot > m_min_num_samples){
					RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_nc_n);
					FillingBlank(m_mcs_cur);
					Monotonicity();
			}

			m_mcs_cur = mcs;
			m_rssi_cur = rssi;
			m_first_seq = 0;
			m_rcv = 1;
			m_id = id;
	}
	else{
		m_rssi_cur += rssi;
		m_rcv++;
	}
}


void
OnlineTableManager::SamplingTimeout (void){
	double pdr = (double)m_rcv / (double)m_tot;
	uint32_t rssi_avg = m_rssi_cur / m_rcv;
	
	NS_LOG_LOGIC("Sampling Timeout Pdr: " << pdr << " Rssi: " << rssi_avg  << " rcv: " << m_rcv << " tot: " << m_tot << " mcs: " << (uint32_t)m_mcs_cur); 
	
	if (m_tot > m_min_num_samples)
		RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_tot);
}

void
OnlineTableManager::RecordSample (uint8_t mcs, uint32_t rssi_sum, uint16_t rcv, uint16_t tot){
	uint32_t rssi = rssi_sum / (uint32_t)rcv;
	double pdr = (double)rcv / (double)tot;
	
	uint32_t rssi_idx = rssi - rssi_min;
	uint32_t mcs_idx = (uint32_t)mcs;

	NS_LOG_LOGIC("New Sample MCS: " << mcs_idx << " IDX: " << rssi_idx <<  " RSSI_SUM: " << rssi_sum << " RCV: " << rcv << " TOT: " << tot << "PDR: " << pdr);
	
	if (rssi_idx >= 50){
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
OnlineTableManager::PrintTable(Pdr table[][50], std::ostream &os){
	os.precision (2);
	for (uint32_t i = 0; i < 8; i++){
		os << "MCS " << i << " Rcv: (";	
		for (uint32_t j=0; j<50; j++){
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

			for (uint32_t j = 0; j < 50; j++){
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

double
OnlineTableManager::GetPdr (uint8_t mcs, uint32_t rssi){
	if (rssi < rssi_min)
			return 0;
	else if (rssi > rssi_min + 50)
			return threshold_high;
	else
			return m_table[mcs][rssi].p;
}

void
OnlineTableManager::FillingBlank(uint8_t mcs){
		//NS_LOG_INFO( " Before Filling ");
		//PrintOnlineTable(std::cout);

		double rssi_low_value = 1;
		double rssi_high_value = 0;

		for (uint32_t j=0; j < 50; j++){
				double pdr = m_table[mcs][j].p;
				bool mark = m_table[mcs][j].mark;

				if (mark == true && pdr < threshold_low){
						m_no_low[mcs] = false;
				}
				if (mark == true && pdr <= rssi_low_value){
						m_rssi_low[mcs] = j;
						rssi_low_value = pdr;
				}
				if (mark == true && pdr > threshold_low && pdr < threshold_high){
						m_no_middle[mcs] = false;
				}
				if (mark == true && pdr > rssi_high_value){
						if (rssi_high_value != 0){
								for (uint32_t k = m_rssi_high[mcs] + 1; k < j; k++){
										m_table[mcs][k].p = rssi_high_value +(double)(k-m_rssi_high[mcs])*(pdr - rssi_high_value)/(double)(j-m_rssi_high[mcs]);
								}
						}
						rssi_high_value = pdr;
						m_rssi_high[mcs] = j;
				}
				if (mark == true && pdr == threshold_high){
						for (uint32_t k = j+1; k < 50; k++){
								m_table[mcs][k].p = threshold_high;
						}
						rssi_high_value = pdr;
						m_rssi_high[mcs] = j;

						m_no_high[mcs] = false;
						break;
				}
		}
}

void
OnlineTableManager::Monotonicity(void){
	uint8_t ref_mcs = 8;

//	NS_LOG_UNCOND("Before Monotonicity");
//	PrintOnlineTable(std::cout);
	
	for (uint8_t i=0; i < 8; i++){
		bool no_low = m_no_low[i];
		bool no_high = m_no_high[i];
		bool no_middle = m_no_middle[i];

		if (!no_high && !no_low){
				ref_mcs = i;
				break;
		}
				
		if ((!no_high && !no_middle) && i < ref_mcs){
			ref_mcs = i;
		}
		NS_LOG_INFO("MCS :  "<< (uint32_t)i <<  " High: " << m_rssi_high[i] << " Low: " << m_rssi_low[i] << " RefMCS: " << (uint32_t)ref_mcs);
	}
	if (ref_mcs == 8){
			uint32_t id_mcs = 50;
			for (uint8_t i = 0; i < 8; i++){
					uint32_t temp_id = m_rssi_high[i] > i ? m_rssi_high[i] - (uint32_t)i : 0;

					if (temp_id < id_mcs){
							ref_mcs = i;
							id_mcs = temp_id;
					}
			}
	}
		
	NS_LOG_INFO("REF MCS :  "<< (uint32_t)ref_mcs <<  " High: " << m_rssi_high[ref_mcs] << " Low: " << m_rssi_low[ref_mcs]);

	if (m_no_low[ref_mcs]){
		double rssi_high_value = (m_table[ref_mcs][m_rssi_high[ref_mcs]]).p;
		double rssi_low_value = (m_table[ref_mcs][m_rssi_low[ref_mcs]]).p;
		double slope = 0;

		if (m_no_middle[ref_mcs]){
			slope = threshold_high / (double)m_transition;
		}
		else{
			slope = (rssi_high_value - rssi_low_value) / (double)(m_rssi_high[ref_mcs] - m_rssi_low[ref_mcs]);
		}

		if (m_rssi_low[ref_mcs] > m_rssi_high[ref_mcs]-m_transition){
				for (uint32_t k = m_rssi_low[ref_mcs]; k >= m_rssi_high[ref_mcs]-m_transition; k--){
						m_table[ref_mcs][k].p = rssi_high_value - (double)(m_rssi_high[ref_mcs]-k)*slope;
						m_rssi_low[ref_mcs] = k;
						if (m_table[ref_mcs][k].p < threshold_low){
								m_table[ref_mcs][k].p = 0;
								break;
						}
				}
		}

		if ((m_table[ref_mcs][m_rssi_low[ref_mcs]]).p > 0){
				m_rssi_low[ref_mcs] -= 1;
		}
	}

	for (uint8_t i = 0; i < 8; i++){
		bool no_low = m_no_low[i];
		bool no_high = m_no_high[i];
		bool no_middle = m_no_middle[i];
	
		NS_LOG_INFO("MCS :  "<< (uint32_t)i <<  " High: " << m_rssi_high[i] << " Low: " << m_rssi_low[i] << " ( " << no_high << " " << no_low << " " << no_middle << " )");

		if (i == ref_mcs)
				continue;
		if (!no_high && !no_low)
				continue;
		else if (!no_high && !no_middle)
				continue;
		else if (!no_high && no_middle){
				if(m_rssi_high[i] > m_rssi_high[ref_mcs] + i - ref_mcs ){
						m_rssi_high[i] = m_rssi_high[ref_mcs] + i - ref_mcs;
				}
				if (m_rssi_low[i] > m_rssi_low[ref_mcs] + i -ref_mcs){
						m_rssi_low[i] = m_rssi_low[ref_mcs] + i -ref_mcs;
				}

				double slope = 0;

				if (m_rssi_high[i] == m_rssi_low[i])
					slope = threshold_high / (double)m_transition;
				else
					slope = threshold_high / (double)(m_rssi_high[i] - m_rssi_low[i]);
				
				NS_LOG_INFO("MCS: "<< (uint32_t)i <<  " High: " << m_rssi_high[i] << " Low: " << m_rssi_low[i] << " ( " << no_high << " " << no_low << " " << no_middle << " )" << " Slope: " << slope );
				
				for (uint32_t k = m_rssi_high[i]; k < 50; k++){
						m_table[i][k].p = threshold_high;
				}
				for (uint32_t k = 0; k <= m_rssi_low[i]; k++){
						m_table[i][k].p = 0;

				}

				for (uint32_t k = m_rssi_low[i]+1; k < m_rssi_high[i]; k++){
						m_table[i][k].p = threshold_high - (double)(m_rssi_high[i]-k)*slope;
						if (m_table[i][k].p < threshold_low){
								m_table[i][k].p = 0;
						}
				}
		}
		else if (no_high && no_low){
				double rssi_high_value = (m_table[i][m_rssi_high[i]]).p;
				double rssi_low_value = (m_table[i][m_rssi_low[i]]).p;

				if (m_rssi_low[i] == m_rssi_high[i]){
						if (m_rssi_high[i] == 0)
								continue;

						NS_LOG_INFO("Single sample");

						for (uint32_t k = 0; k < m_rssi_high[i]; k++){
								m_table[i][k].p = rssi_high_value - (double)(m_rssi_high[i]-k)*(threshold_high /(double)m_transition);
								if (m_table[i][k].p < threshold_low){
										m_table[i][k].p = 0;
										break;
								}
						}

						for (uint32_t k = m_rssi_high[i]; k < 50; k++){
								m_table[i][k].p = rssi_high_value + (double)(k - m_rssi_high[i])*(threshold_high /(double)m_transition);
								if (m_table[i][k].p >= threshold_high){
										m_table[i][k].p = threshold_high;
								}
						}
				}

				else{
						double slope = (rssi_high_value - rssi_low_value) / (m_rssi_high[i] - m_rssi_low[i]);

						for (uint32_t k = 0; k < m_rssi_low[i]; k++){
								if ( k + m_transition < m_rssi_low[i] )
										m_table[i][k].p = 0;
								else{
										m_table[i][k].p = rssi_low_value - (double)(m_rssi_low[i]-k)*slope;
										if (m_table[i][k].p < threshold_low){
												m_table[i][k].p = 0;
										}
								}
						}

						for (uint32_t k = m_rssi_high[i]+1; k < 50; k++){
								m_table[i][k].p = rssi_high_value + (double)(k - m_rssi_high[i])*slope;
								if (m_table[i][k].p >= threshold_high){
										m_table[i][k].p = threshold_high;
								}
						}
				}
		}
	}

//	NS_LOG_UNCOND("After Monotonicity");
//	PrintOnlineTable(std::cout);
}


void
OnlineTableManager::PrintCorrectTable(std::ostream &os){
	PrintTable(m_correct_table, os);
}


void
OnlineTableManager::SetN(uint16_t nc_n){
		m_nc_n = nc_n;
}



}
