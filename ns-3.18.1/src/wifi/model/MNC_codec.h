#ifndef MNC_CODEC_H
#define MNC_CODEC_H

#include "ns3/object.h"
#include "wifi-mac-queue.h"
#include "coefficient-header.h"

namespace ns3{

class MNC_Encoder
{
	public:
		MNC_Encoder();
		virtual ~MNC_Encoder();

		void MNC_Enqueue(Ptr<const Packet> packet, WifiMacHeader hdr);
		Ptr<const Packet> MNC_Dequeue(WifiMacHeader *hdr);
		void Encoding(uint16_t eid, uint8_t k, uint8_t n, uint8_t p, bool sys);
		void WithCoeffEncoding(uint16_t eid, uint8_t num, uint8_t n, uint8_t p, bool sys);
		uint32_t GetBufferSize() const;
		void FlushAllBuffer();
		void FlushN();
		void FlushK();

	private:
		Ptr<WifiMacQueue> m_MNCbuffer_K;
		Ptr<WifiMacQueue> m_MNCbuffer_N;
		uint16_t m_last_eid;
		uint16_t m_last_seq;
		
		uint8_t gmul(uint8_t a, uint8_t b);
		uint8_t gmul_inverse(uint8_t in);
		uint8_t gsub(uint8_t a, uint8_t b);
		uint8_t gadd(uint8_t a, uint8_t b);
		uint8_t gdiv(uint8_t a, uint8_t b);
};

class MNC_Decoder
{
	public:
		MNC_Decoder();
		virtual ~MNC_Decoder();

		void MNC_Rx_Enqueue(Ptr<const Packet> packet, const WifiMacHeader *hdr);
		void MNC_Rx_EEnqueue(Ptr<const Packet> packet,const WifiMacHeader *hdr);
		Ptr<const Packet> MNC_Rx_Dequeue(WifiMacHeader *hdr);
		Ptr<const Packet> MNC_Rx_DDequeue(WifiMacHeader *hdr);
		Ptr<const Packet> MNC_Rx_EDequeue(WifiMacHeader *hdr);
		Ptr<const Packet> MNC_Rx_SDequeue(WifiMacHeader *hdr);
		bool Decoding(uint8_t k, uint8_t p, bool stamp);
		void Systematicing();
		uint32_t GetBufferSize() const;
		uint32_t GetDBufferSize() const;
		uint32_t GetEBufferSize() const;
		uint32_t GetSBufferSize() const;
		void SetEid(uint16_t eid);
		uint16_t GetEid() const;
		//void SetDtime(int64_t dtime);
		int64_t GetDtime() const;
		//std::vector<uint8_t> GetCoeff(Ptr<const Packet> packet);

	private:
		Ptr<WifiMacQueue> m_MNCbuffer_Rx;
		Ptr<WifiMacQueue> m_MNCbuffer_Decoded;
		Ptr<WifiMacQueue> m_MNCbuffer_Encoded;
		Ptr<WifiMacQueue> m_MNCbuffer_Sys;
		
		uint8_t gmul(uint8_t a, uint8_t b);
		uint8_t gmul_inverse(uint8_t in);
		uint8_t gsub(uint8_t a, uint8_t b);
		uint8_t gadd(uint8_t a, uint8_t b);
		uint8_t gdiv(uint8_t a, uint8_t b);
		
		void lu_fac(uint8_t m, uint8_t n, uint8_t **Arr, uint8_t **U, uint8_t **L);

		uint8_t LU_decoding(uint8_t *coeffi, uint16_t rcv_num, uint8_t k, uint8_t p);
		
		uint16_t m_eid;
		int64_t m_dtime;
		
		bool m_systematic;

};
}
#endif
