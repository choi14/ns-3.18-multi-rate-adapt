#include "sbra-wifi-manager.h"
#include "yans-error-rate-model.h"
#include "wifi-phy.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include <cmath>
#include "ns3/log.h"
#define Min(a,b) ((a < b) ? a : b)

//jychoi
#include <ostream>
#include <vector>
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/net-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/regular-wifi-mac.h"

NS_LOG_COMPONENT_DEFINE("SbraWifiManager");

namespace ns3 {

struct SbraWifiRemoteStation : public WifiRemoteStation
{
	double m_lastSnr;
};

NS_OBJECT_ENSURE_REGISTERED (SbraWifiManager);

TypeId
SbraWifiManager::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::SbraWifiManager")
				.SetParent<WifiRemoteStationManager> ()
				.AddConstructor<SbraWifiManager> ()
				.AddAttribute ("BerThreshold",
						"The maximum Bit Error Rate acceptable at any transmission mode",
						DoubleValue (10e-6),
						MakeDoubleAccessor (&SbraWifiManager::m_ber),
						MakeDoubleChecker<double> ())
				.AddAttribute ("Type",
						"Type of rate adaptation",
						UintegerValue (0),
						MakeUintegerAccessor (&SbraWifiManager::m_type),
						MakeUintegerChecker<uint32_t> ())
				.AddAttribute ("PerThreshold",
						"The maximum Bit Error Rate acceptable at any transmission mode",
						DoubleValue (0.001),
						MakeDoubleAccessor (&SbraWifiManager::m_per),
						MakeDoubleChecker<double> ())
				;
			return tid;
}

SbraWifiManager::SbraWifiManager ()
{
	m_addBasicMode = false;
	m_num = 0;
	m_sum_min_snr = 0;
}
SbraWifiManager::~SbraWifiManager ()
{
}

	void
SbraWifiManager::SetupPhy (Ptr<WifiPhy> phy)
{
  uint32_t nModes = phy->GetNModes ();
  for (uint32_t i = 0; i < nModes; i++)
    {
      WifiMode mode = phy->GetMode (i);
      AddModeSnrThreshold (mode, phy->CalculateSnr (mode, m_ber));
    }
	m_phy = phy;
  WifiRemoteStationManager::SetupPhy (phy);
}

double
SbraWifiManager::GetSnrThreshold (WifiMode mode) const
{
  for (Thresholds::const_iterator i = m_thresholds.begin (); i != m_thresholds.end (); i++)
    {
      if (mode == i->second)
        {
          return i->first;
        }
    }
  NS_ASSERT (false);
  return 0.0;
}

void
SbraWifiManager::AddModeSnrThreshold (WifiMode mode, double snr)
{
  m_thresholds.push_back (std::make_pair (snr,mode));
}

WifiRemoteStation *
SbraWifiManager::DoCreateStation (void) const
{
  SbraWifiRemoteStation *station = new SbraWifiRemoteStation ();
  station->m_lastSnr = 0.0;
  return station;
}


void
SbraWifiManager::DoReportRxOk (WifiRemoteStation *station,
                                double rxSnr, WifiMode txMode)
{
}
void
SbraWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
}
void
SbraWifiManager::DoReportDataFailed (WifiRemoteStation *station)
{
}
void
SbraWifiManager::DoReportRtsOk (WifiRemoteStation *st,
                                 double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  SbraWifiRemoteStation *station = (SbraWifiRemoteStation *)st;
  station->m_lastSnr = rtsSnr;
}
void
SbraWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                  double ackSnr, WifiMode ackMode, double dataSnr)
{
  SbraWifiRemoteStation *station = (SbraWifiRemoteStation *)st;
  station->m_lastSnr = dataSnr;
}
void
SbraWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
}
void
SbraWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
}
void
SbraWifiManager::AddOfdmRate (void)
{
	AddBasicMode(WifiMode("OfdmRate6Mbps"));
	AddBasicMode(WifiMode("OfdmRate9Mbps"));
	AddBasicMode(WifiMode("OfdmRate12Mbps"));
	AddBasicMode(WifiMode("OfdmRate18Mbps"));
	AddBasicMode(WifiMode("OfdmRate24Mbps"));
	AddBasicMode(WifiMode("OfdmRate36Mbps"));
	AddBasicMode(WifiMode("OfdmRate48Mbps"));
	AddBasicMode(WifiMode("OfdmRate54Mbps"));
	m_addBasicMode = true;
}
WifiMode
SbraWifiManager::DoGroupRateAdaptation ()
{
		return GroupRateAdaptation();
}
WifiMode
SbraWifiManager::GroupRateAdaptation ()
{
	if (m_addBasicMode == false)
		AddOfdmRate ();
	
	uint32_t vsize = m_infos.size();
	if(vsize == 0)
	{
		m_GroupTxMode = GetBasicMode (0);
	}
	else
	{
		m_minSnr = (double)m_infos[0].info.Rssi;
		for (uint32_t m = 0; m < vsize; m++)
		{
			if(m_minSnr > (double)m_infos[m].info.Rssi)
			{
				m_minSnr = (double)m_infos[m].info.Rssi;
			}
		}
	
		m_sum_min_snr += m_minSnr;
		m_num++;

		m_minSnr = std::pow (10.0, m_minSnr/10.0); 
		//NS_LOG_INFO ("m_minSnr: " << m_minSnr);

		uint32_t NBasicMode = GetNBasicModes ();
		double Pdr = 0.0;

		if(m_minSnr > 1.0)
		{
			// PER-SNR Rate Adaptation
			if(m_type == 0)
			{
				double Per = 1;
				for (uint32_t k = 0; k < NBasicMode; k++)
				{
					WifiMode mode = GetBasicMode(k);
					
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
					double nSymbols = ((databits+64)*8+22)/coderate/ofdmbits;
					uint32_t nbits = ((uint32_t)nSymbols +1)*ofdmbits;

					Pdr = m_phy->CalculatePdr (mode, m_minSnr, nbits);
					double tempPer = 1-Pdr;
					if(tempPer < m_per)
					{
						m_GroupTxMode = mode;
						Per=tempPer;
					}
				}
				if (Per > 1)
					NS_ASSERT("Never happen");
				else if (Per == 1)
					m_GroupTxMode = GetBasicMode (0);
				
				NS_LOG_INFO ("m_minSnr: " << m_minSnr << " GroupTxDataRate: " <<  m_GroupTxMode.GetDataRate ()*0.000001<<" Mb/s");
				//NS_LOG_INFO("PER: "<< Per << " GroupTxDataRate: " 
				//		<< m_GroupTxMode.GetDataRate ()*0.000001<<" Mb/s");
			}

			// Throughput-SNR Rate Adaptation
			else if (m_type == 1)
			{
				double maxPdrRate = 0.0;
				double PdrRate = 0.0;
				double Rate = 0.0;
				for (uint32_t k = 0; k < NBasicMode; k++)
				{
					WifiMode mode = GetBasicMode(k);
					NS_LOG_INFO("mode = "<<GetBasicMode(k).GetDataRate() );
					Pdr = m_phy->CalculatePdr (mode, m_minSnr, 1086*8);
					Rate = mode.GetDataRate();
					PdrRate = Pdr*Rate*0.000001;

					if (PdrRate > maxPdrRate)
					{
						maxPdrRate = PdrRate;
						m_GroupTxMode = mode;
					}
				}
				if (maxPdrRate == 0)
					m_GroupTxMode = GetBasicMode (0);
				
				NS_LOG_INFO("SNR: "<< m_minSnr <<" GroupTxDataRate: "<< 
						m_GroupTxMode.GetDataRate ()*0.000001<<" Mb/s");
			}
		}
		else
			m_GroupTxMode = GetBasicMode (0);
	}
	return m_GroupTxMode;
}
WifiTxVector
SbraWifiManager::DoGetDataTxVector (WifiRemoteStation *st, uint32_t size)
{
	SbraWifiRemoteStation *station = (SbraWifiRemoteStation *)st;
	// We search within the Supported rate set the mode with the
	// highest snr threshold possible which is smaller than m_lastSnr
  // to ensure correct packet delivery.
	double maxFdrRate = 0.0;
	double FdrRate = 0.0;
	double Fdr = 0.0;
	double Rate = 0.0;
	double lastSnr = station->m_lastSnr;
	
	WifiMode maxMode = GetDefaultMode ();
  for (uint32_t i = 0; i < GetNSupported (station); i++)
    {
      WifiMode mode = GetSupported (station, i);
  		Fdr = m_phy->CalculatePdr (mode, lastSnr, 1086*8);
			
			Rate = mode.GetDataRate();
			FdrRate = Fdr*Rate;
			// jychoi
			if (FdrRate > maxFdrRate)
        {
					maxFdrRate = FdrRate;
          maxMode = mode;
        }
    }
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetLongRetryCount (station), GetShortGuardInterval (station), Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()), GetNumberOfTransmitAntennas (station), GetStbc (station));
}

WifiTxVector
SbraWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  SbraWifiRemoteStation *station = (SbraWifiRemoteStation *)st;
  // We search within the Basic rate set the mode with the highest
  // snr threshold possible which is smaller than m_lastSnr to
  // ensure correct packet delivery.
  double maxThreshold = 0.0;
  WifiMode maxMode = GetDefaultMode ();
  for (uint32_t i = 0; i < GetNBasicModes (); i++)
    {
      WifiMode mode = GetBasicMode (i);
      double threshold = GetSnrThreshold (mode);
      if (threshold > maxThreshold
          && threshold < station->m_lastSnr)
        {
          maxThreshold = threshold;
          maxMode = mode;
        }
    }
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetShortRetryCount (station), GetShortGuardInterval (station), Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()), GetNumberOfTransmitAntennas (station), GetStbc (station));
}

void
SbraWifiManager::UpdateInfo (Mac48Address addr, struct rxInfo info)
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
	if (exist == false){
			StaInfo stainfo;
			stainfo.addr = addr;
			/*
			m_infos[i].rxInfo.Rssi = info.Rssi;
			m_infos[i].rxInfo.Snr = info.Snr;
			m_infos[i].rxInfo.LossPacket = info.LossPacket;
			m_infos[i].rxInfo.TotalPacket = info.TotalPacket;
*/
		m_infos.push_back(stainfo);
	}
	for (uint32_t i = 0; i < m_infos.size(); i++){
		NS_LOG_ERROR("Addr " << m_infos[i].addr); 
	}
}

bool
SbraWifiManager::IsLowLatency (void) const
{
  return true;
}

double
SbraWifiManager::GetAvgMinSnrDb(void)
{
	return m_sum_min_snr / (double)m_num;
}





} // namespace ns3
