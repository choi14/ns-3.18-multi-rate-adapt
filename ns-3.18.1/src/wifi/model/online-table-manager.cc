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
							DoubleValue(0.1),
							MakeDoubleAccessor(&OnlineTableManager::alpha),
							MakeDoubleChecker<double> ())
			.AddAttribute("SampleType", "0: Time-based 1: Number-based",
							UintegerValue(0),
							MakeUintegerAccessor(&OnlineTableManager::m_type),
							MakeUintegerChecker<uint8_t> ())
			.AddAttribute("SamplingNum", "Number of samples",
							UintegerValue(20),
							MakeUintegerAccessor(&OnlineTableManager::m_sampling_num),
							MakeUintegerChecker<uint16_t> ())
			;

	return tid;
}

OnlineTableManager::OnlineTableManager():
		m_rssi_cur(0),
		m_pdr_cur(0),
		m_mcs_cur(0),
		sampling_period(MilliSeconds(20))
{
		m_init = false;
	 	threshold_high = 0.95;
	 	threshold_low = 0.05;
		rssi_min = 0;
		m_rcv = 0;
		m_tot = 0;
		m_first_seq = 0;
		//m_last_seq = 0;
		m_training_max = 100;

		sampling_period = Time(MilliSeconds(20));
		
		for (uint32_t i = 0; i < 8; i++){
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
			m_tot = m_training_max - m_first_seq + 1;
	

			if (m_tot > 10)
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
				

				if (m_tot > 10)
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
		
			if (m_tot > 10)
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
	//double pdr = (double)m_rcv / (double)m_tot;
	//uint32_t rssi_avg = m_rssi_cur / m_rcv;
	
	//NS_LOG_UNCOND("Sampling Timeout Pdr: " << pdr << " Rssi: " << rssi_avg  << " rcv: " << m_rcv << " tot: " << m_tot << " mcs: " << (uint32_t)m_mcs_cur); 
	
	if (m_tot > 10)
		RecordSample(m_mcs_cur, m_rssi_cur, m_rcv, m_tot);
	
//	if (m_mcs_cur == 7)
		//PrintTable(m_table, std::cout);
}

void
OnlineTableManager::RecordSample (uint8_t mcs, uint32_t rssi_sum, uint16_t rcv, uint16_t tot){
	uint32_t rssi = rssi_sum / (uint32_t)rcv;
	double pdr = (double)rcv / (double)tot;
	
	uint32_t rssi_idx = rssi - rssi_min;
	uint32_t mcs_idx = (uint32_t)mcs;

	if (rssi_idx < 5)
			NS_LOG_UNCOND("MCS: " << mcs_idx << " IDX: " << rssi_idx <<  " RSSI_SUM: " << rssi_sum << " RCV: " << rcv << " TOT: " << tot);

	if (rssi_idx >= 50){
			NS_LOG_UNCOND("High RSSI MCS: " << mcs_idx << " IDX: " << rssi_idx <<  " RSSI_SUM: " << rssi_sum << " RCV: " << rcv << " TOT: " << tot);
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
/*	
	for (uint32_t i = 0; i < rssi_idx; i++){
		if((m_table[mcs][i]).p > pdr_cur.p && m_table[mcs][i].mark == true){
			pdr_cur.p = (m_table[mcs][i]).p;
			break;
		}
	}
*/
	(m_table[mcs_idx][rssi_idx]).p = pdr_cur.p;
	(m_table[mcs_idx][rssi_idx]).mark = pdr_cur.mark;
}

void
OnlineTableManager::PrintTable(Pdr table[][50], std::ostream &os){
	os.precision (2);
	for (uint32_t i = 0; i < 8; i++){
		os << "MCS " << i << " Rcv: (";	
		for (uint32_t j=0; j<50; j++){
				os << (m_table[i][j]).p << " ";
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
		
		//PrintTable(m_correct_table, std::cout);
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



}
