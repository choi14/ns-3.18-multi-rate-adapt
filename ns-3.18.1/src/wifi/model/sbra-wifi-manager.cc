#include "sbra-wifi-manager.h"
#include "yans-error-rate-model.h"
#include "wifi-phy.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include <cmath>
#include "ns3/log.h"
#define Min(a,b) ((a < b) ? a : b)

/*jychoi
#include <ostream>
#include <vector>
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/net-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/regular-wifi-mac.h"
*/

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
		;
	return tid;
}

SbraWifiManager::SbraWifiManager ()
{
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

bool
SbraWifiManager::IsLowLatency (void) const
{
  return true;
}

} // namespace ns3
