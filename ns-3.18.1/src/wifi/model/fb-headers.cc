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
  return 16;
}
void
FeedbackHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_rssi);
  start.WriteHtonU32 (m_snr);
  start.WriteHtonU32 (m_lossPacket);
  start.WriteHtonU32 (m_totalPacket);
}
uint32_t
FeedbackHeader::Deserialize (Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_rssi = i.ReadNtohU32 (); 
	m_snr = i.ReadNtohU32 (); 
	m_lossPacket = i.ReadNtohU32 (); 
	m_totalPacket = i.ReadNtohU32 (); 
  return i.GetDistanceFrom (start);
}
// jychoi
void
FeedbackHeader::SetRssi (uint32_t rssi)
{
	m_rssi = rssi;
}
void
FeedbackHeader::SetSnr (uint32_t snr)
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
// jychoi
uint32_t
FeedbackHeader::GetRssi (void)
{
	return m_rssi;
}
uint32_t
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

} // namespace ns3
