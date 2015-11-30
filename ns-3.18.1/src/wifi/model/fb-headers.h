#ifndef FB_HEADERS_H
#define FB_HEADERS_H

//#include <stdint.h>

#include "ns3/header.h"

namespace ns3 {

typedef struct
{
	uint8_t perMCS0;
	uint8_t perMCS1;
	uint8_t perMCS2;
	uint8_t perMCS3;
	uint8_t perMCS4;
	uint8_t perMCS5;
	uint8_t perMCS6;
	uint8_t perMCS7;
}PerRate;

struct rxInfo
{
	int32_t Rssi;
	int32_t Snr;
	uint32_t LossPacket;
	uint32_t TotalPacket;
	PerRate perRate;
};

class FeedbackHeader : public Header
{
public:
	static TypeId GetTypeId (void);
	virtual TypeId GetInstanceTypeId (void) const;
	virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

	void SetRssi (int32_t rssi);
	void SetSnr (int32_t snr);
	void SetLossPacket (uint32_t lossPacket);
	void SetTotalPacket (uint32_t totalPacket);
	void SetPerRate (PerRate perRate);

	int32_t GetRssi (void);
	int32_t GetSnr (void);
	uint32_t GetLossPacket (void);
	uint32_t GetTotalPacket (void);
	PerRate GetPerRate (void);

private:
	uint32_t m_rssi;
	uint32_t m_snr;
	uint32_t m_lossPacket;
	uint32_t m_totalPacket;
	PerRate m_perRate;
};

} // namespace ns3

#endif /* FB_HEADERS_H */
