#include "fb-headers.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE("FeedbackHeader");

namespace ns3 {
	
NS_OBJECT_ENSURE_REGISTERED(FeedbackHeader);

TypeId
FeedbackHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FeedbackHeader")
    .SetParent<Header> ()
    .AddConstructor<FeedbackHeader> ()
  ;
  return tid;
}

TypeId
FeedbackHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
FeedbackHeader::Print(std::ostream &os) const
{
	os << "RSSI = " << m_rssi << "\n";
}

uint32_t
FeedbackHeader::GetSerializedSize (void) const
{
  return 32;
}
void
FeedbackHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_rssi);
  start.WriteHtonU32 (m_snr);
  start.WriteHtonU32 (m_lossPacket);
  start.WriteHtonU32 (m_totalPacket);
	start.WriteHtonU16 (m_perRate.perMCS0);
	start.WriteHtonU16 (m_perRate.perMCS1);
	start.WriteHtonU16 (m_perRate.perMCS2);
	start.WriteHtonU16 (m_perRate.perMCS3);
	start.WriteHtonU16 (m_perRate.perMCS4);
	start.WriteHtonU16 (m_perRate.perMCS5);
	start.WriteHtonU16 (m_perRate.perMCS6);
	start.WriteHtonU16 (m_perRate.perMCS7);
}
uint32_t
FeedbackHeader::Deserialize (Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_rssi = i.ReadNtohU32 (); 
	m_snr = i.ReadNtohU32 (); 
	m_lossPacket = i.ReadNtohU32 (); 
	m_totalPacket = i.ReadNtohU32 ();
	m_perRate.perMCS0 = i.ReadNtohU16 ();
	m_perRate.perMCS1 = i.ReadNtohU16 ();
	m_perRate.perMCS2 = i.ReadNtohU16 ();
	m_perRate.perMCS3 = i.ReadNtohU16 ();
	m_perRate.perMCS4 = i.ReadNtohU16 ();
	m_perRate.perMCS5 = i.ReadNtohU16 ();
	m_perRate.perMCS6 = i.ReadNtohU16 ();
	m_perRate.perMCS7 = i.ReadNtohU16 ();
  return i.GetDistanceFrom (start);
}
// jychoi
void
FeedbackHeader::SetRssi (int32_t rssi)
{
	m_rssi = rssi;
}
void
FeedbackHeader::SetSnr (int32_t snr)
{
	m_snr = snr;
}
void
FeedbackHeader::SetLossPacket (uint32_t lossPacket)
{
	m_lossPacket = lossPacket;
}
void
FeedbackHeader::SetTotalPacket (uint32_t totalPacket)
{
	m_totalPacket = totalPacket;
}
void
FeedbackHeader::SetPerRate (PerRate perRate)
{
	m_perRate = perRate;
}
// jychoi
int32_t
FeedbackHeader::GetRssi (void)
{
	return m_rssi;
}
int32_t
FeedbackHeader::GetSnr (void)
{
	return m_snr;
}
uint32_t
FeedbackHeader::GetLossPacket (void)
{
	return m_lossPacket;
}
uint32_t
FeedbackHeader::GetTotalPacket (void)
{
	return m_totalPacket;
}
PerRate
FeedbackHeader::GetPerRate (void)
{
	return m_perRate;
}

} // namespace ns3
