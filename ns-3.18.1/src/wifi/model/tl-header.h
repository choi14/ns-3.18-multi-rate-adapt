#ifndef TL_H
#define TL_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"
#include <stdint.h>
#include <vector>

#define TLK 100
#define TLTHRE 80

namespace ns3{
	struct TlNbrInfo
	{
		Mac48Address nbrAddr;
		uint16_t nbrRcv;
		bool tf_cnt;
		bool nr_cnt;
		std::vector<struct TlNbrInfo> nbr2hopList;
	};

	typedef std::vector<struct TlNbrInfo> TlNbrInfoList;

	enum TlType
	{
		TL_TF1 = 1,
		TL_TF2 = 2,
		TL_TR1 = 3,
		TL_TR2 = 4,
		TL_STF = 5,
		TL_NBR = 6,
	};

	class TlHeader : public Header
	{
		public:
			TlHeader();
			~TlHeader();
			static TypeId GetTypeId (void);
			virtual TypeId GetInstanceTypeId (void) const;
			virtual void Print (std::ostream &os) const;
			virtual uint32_t GetSerializedSize (void) const;
			virtual void Serialize (Buffer::Iterator start) const;
			virtual uint32_t Deserialize (Buffer::Iterator start);

			void SetType (enum TlType type);
			void SetK (uint16_t k);
			void SetSeq (uint16_t seq);
			void SetId (uint16_t id);
			void SetRcv (uint16_t rcv);
			void SetRate (uint8_t rate);
			void SetNbrList (TlNbrInfoList nbrList);

			enum TlType GetType (void) const;
			uint16_t GetK (void) const;
			uint16_t GetSeq (void) const;
			uint16_t GetId (void) const;
			uint16_t GetRcv (void) const;
			uint8_t GetRate (void) const;
			TlNbrInfoList GetNbrList (void) const;

		private:
			uint32_t GetSize (void) const;
			
			enum TlType m_type;
			uint16_t m_k;
			uint16_t m_seq;
			uint16_t m_id;
			uint16_t m_rcv;
			uint8_t m_rate;
			TlNbrInfoList m_nbrList;
	};

	class TlNbrInfoFactory
	{
		public:
			TlNbrInfoFactory();
			~TlNbrInfoFactory();
			
			void Insert1hopInfo(Mac48Address nbrName, uint16_t num);
			uint16_t Plus1hopInfo(Mac48Address nbrName);
			uint16_t Get1hopInfo(Mac48Address nbrName) const;
			void Insert2hopInfo(Mac48Address nbr1hopName, Mac48Address nbr2hopName, uint16_t num);
			uint16_t Get2hopInfo(Mac48Address nbr1hopName, Mac48Address nbr2hopName) const;
			void Print(std::ostream &os) const;
			Mac48Address NextSTF(void);
			bool FillSTF(void);
			bool EndSTF(void) const;
			TlNbrInfoList GetNbrInfoList(void) const;

		private:
			TlNbrInfoList m_nbrList;
			uint16_t m_threshold;
	};
}

#endif
