#include "tl-header.h"

namespace ns3{

	TlHeader::TlHeader()
		: m_type (TL_TF1),
		  m_k (0),
			m_seq (0),
			m_id (0),
			m_rcv (0)
	{
	}

	TlHeader::~TlHeader()
	{
	}

	TypeId TlHeader::GetTypeId (void)
	{
		static TypeId tid = TypeId ("ns3::TlHeader")
			.SetParent<Header> ()
			.AddConstructor<TlHeader> ()
			;
		return tid;
	}

	TypeId TlHeader::GetInstanceTypeId (void) const
	{
		return GetTypeId ();
	}
	
	void TlHeader::Print (std::ostream &os) const
	{
		switch (m_type)
		{
			case TL_TF1:
				os << std::hex << "TF1 > K = " << m_k << ", Seq = " << m_seq << ", ID = " << m_id << ", Rate = " << m_rate << '\n';
				break;
			case TL_TF2:
				os << std::hex << "TF2 > K = " << m_k << ", Seq = " << m_seq << ", ID = " << m_id << ", Rate = " << m_rate << '\n';
				break;
			case TL_TR1:
				os << std::hex << "TR1 > Rcv = " << m_rcv << ", Rate = " << m_rate << '\n';
				break;
			case TL_TR2:
				os << std::hex << "TR2 > Rcv = " << m_rcv << ", Rate = " << m_rate << '\n';
				break;
			case TL_STF:
				os << std::hex << "STF > K = " << m_k << ", Rate = " << m_rate << '\n';
				break;
			case TL_NBR:
				os << std::hex << "NBR > K = " << m_k << ", Nbr = " << m_nbrList.size() << ", Rate = " << m_rate << '\n';
				break;
			default:
				NS_ASSERT(false);
				break;
		}
	}

	uint32_t TlHeader::GetSerializedSize (void) const
	{
		return GetSize ();
	}

	void TlHeader::Serialize (Buffer::Iterator i) const
	{
		i.WriteU8((uint8_t) m_type);
		switch (m_type)
		{
			case TL_TF1:
			case TL_TF2:
				i.WriteHtonU16(m_k);
				i.WriteHtonU16(m_seq);
				i.WriteHtonU16(m_id);
				i.WriteU8(m_rate);
				break;
			case TL_TR1:
			case TL_TR2:
				i.WriteHtonU16(m_rcv);
				i.WriteU8(m_rate);
				break;
			case TL_STF:
				i.WriteHtonU16(m_k);
				i.WriteU8(m_rate);
				break;
			case TL_NBR:
				i.WriteHtonU16(m_k);
				i.WriteU8(m_nbrList.size());
				i.WriteU8(m_rate);
				for(TlNbrInfoList::const_iterator j = m_nbrList.begin(); j != m_nbrList.end(); j++)
				{
					uint8_t tmpAddr[6];
					(j->nbrAddr).CopyTo(tmpAddr);
					for(uint8_t k = 0; k < 6; k++)
						i.WriteU8(tmpAddr[k]);
					i.WriteHtonU16(j->nbrRcv);
				}
				break;
			default:
				NS_ASSERT(false);
				break;
		}
	}

	uint32_t TlHeader::Deserialize (Buffer::Iterator start)
	{
		Buffer::Iterator i = start;
		m_type = (enum TlType) i.ReadU8();
		switch (m_type)
		{
			case TL_TF1:
			case TL_TF2:
				m_k = i.ReadNtohU16();
				m_seq = i.ReadNtohU16();
				m_id = i.ReadNtohU16();
				m_rate = i.ReadU8();
				break;
			case TL_TR1:
			case TL_TR2:
				m_rcv = i.ReadNtohU16();
				m_rate = i.ReadU8();
				break;
			case TL_STF:
				m_k = i.ReadNtohU16();
				m_rate = i.ReadU8();
				break;
			case TL_NBR:
				{
					uint8_t tmpNbrNum;
					m_k = i.ReadNtohU16();
					tmpNbrNum = i.ReadU8();
					m_rate = i.ReadU8();
					for(uint8_t j = 0; j < tmpNbrNum; j++)
					{
						struct TlNbrInfo tmpInfo;

						uint8_t tmpAddr[6];
						for(uint8_t k = 0; k < 6; k++)
							tmpAddr[k] = i.ReadU8();
						(tmpInfo.nbrAddr).CopyFrom(tmpAddr);
						tmpInfo.nbrRcv = i.ReadNtohU16();
						tmpInfo.tf_cnt = false;
						tmpInfo.nr_cnt = false;
						m_nbrList.push_back(tmpInfo);
					}
					break;
				}
			default:
				{
					NS_ASSERT(false);
					break;
				}
		}
		return i.GetDistanceFrom (start);
	}
	
	void TlHeader::SetType (enum TlType type)
	{
		m_type = type;
	}

	void TlHeader::SetK (uint16_t k)
	{
		m_k = k;
	}

	void TlHeader::SetSeq (uint16_t seq)
	{
		m_seq = seq;
	}

	void TlHeader::SetId (uint16_t id)
	{
		m_id = id;
	}

	void TlHeader::SetRcv (uint16_t rcv)
	{
		m_rcv = rcv;
	}

	void TlHeader::SetRate (uint8_t rate)
	{
		m_rate = rate;
	}
	
	void TlHeader::SetNbrList (TlNbrInfoList nbrList)
	{
		m_nbrList = nbrList;
	}

	enum TlType TlHeader::GetType (void) const
	{
		return m_type;
	}

	uint16_t TlHeader::GetK (void) const
	{
		return m_k;
	}

	uint16_t TlHeader::GetSeq (void) const
	{
		return m_seq;
	}

	uint16_t TlHeader::GetId (void) const
	{
		return m_id;
	}

	uint16_t TlHeader::GetRcv (void) const
	{
		return m_rcv;
	}

	uint8_t TlHeader::GetRate (void) const
	{
		return m_rate;
	}

	TlNbrInfoList TlHeader::GetNbrList (void) const
	{
		return m_nbrList;
	}

  uint32_t TlHeader::GetSize (void) const
	{
		uint32_t size = 0;
		switch(m_type)
		{
			case TL_TF1:
			case TL_TF2:
				size = 1 + 2 + 2 + 2 + 1;
				break;
			case TL_TR1:
			case TL_TR2:
			case TL_STF:
				size = 1 + 2 + 1;
				break;
			case TL_NBR:
				size = 1 + 2 + 1 + 1 + m_nbrList.size() * (6 + 2);
				break;
		}
		return size;
	}
	
	TlNbrInfoFactory::TlNbrInfoFactory()
		: m_threshold(TLTHRE)
	{
	}

	TlNbrInfoFactory::~TlNbrInfoFactory()
	{
	}

	void TlNbrInfoFactory::Insert1hopInfo(Mac48Address nbrName, uint16_t num)
	{
		for(TlNbrInfoList::iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->nbrAddr == nbrName)
			{
				i->nbrRcv = num;
				return;
			}
		}

		struct TlNbrInfo info;
		info.nbrAddr = nbrName;
		info.nbrRcv = num;
		info.tf_cnt = false;
		info.nr_cnt = false;

		m_nbrList.push_back(info);
	}

	uint16_t TlNbrInfoFactory::Plus1hopInfo(Mac48Address nbrName)
	{
		for(TlNbrInfoList::iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->nbrAddr == nbrName)
			{
				i->nbrRcv++;
				return i->nbrRcv;
			}
		}

		struct TlNbrInfo info;
		info.nbrAddr = nbrName;
		info.nbrRcv = 1;
		info.tf_cnt = false;
		info.nr_cnt = false;

		m_nbrList.push_back(info);

		return info.nbrRcv;
	}


	uint16_t TlNbrInfoFactory::Get1hopInfo(Mac48Address nbrName) const
	{
		for(TlNbrInfoList::const_iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->nbrAddr == nbrName)
			{
				return i->nbrRcv;
			}
		}

		std::cout << "It's not 1hop node: " << nbrName << std::endl;
		return 0;
	}

	void TlNbrInfoFactory::Insert2hopInfo(Mac48Address nbr1hopName, Mac48Address nbr2hopName, uint16_t num){
		for(TlNbrInfoList::iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->nbrAddr == nbr1hopName){
				for(TlNbrInfoList::iterator j = i->nbr2hopList.begin(); j != i->nbr2hopList.end(); j++)
				{
					if(j->nbrAddr == nbr2hopName)
					{
						j->nbrRcv = num;
						return;
					}
				}

				struct TlNbrInfo info;
				info.nbrAddr = nbr2hopName;
				info.nbrRcv = num;
				info.tf_cnt = false;
				info.nr_cnt = false;

				i->nbr2hopList.push_back(info);
				return;
			}
		}

		struct TlNbrInfo info2;
		info2.nbrAddr = nbr2hopName;
		info2.nbrRcv = num;
		info2.tf_cnt = false;
		info2.nr_cnt = false;
		
		struct TlNbrInfo info1;
		info1.nbrAddr = nbr1hopName;
		info1.nbrRcv = 0;
		info1.tf_cnt = false;
		info1.nr_cnt = false;
		
		info1.nbr2hopList.push_back(info2);
		m_nbrList.push_back(info1);

	}

	uint16_t TlNbrInfoFactory::Get2hopInfo(Mac48Address nbr1hopName, Mac48Address nbr2hopName) const
	{
		for(TlNbrInfoList::const_iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->nbrAddr == nbr1hopName){
				for(TlNbrInfoList::const_iterator j = i->nbr2hopList.begin(); j != i->nbr2hopList.end(); j++)
				{
					if(j->nbrAddr == nbr2hopName)
					{
						return j->nbrRcv;
					}
				}
				std::cout << "There is 1hop node but no 2hop node: " << nbr1hopName << ", " << nbr2hopName << std::endl;
				return 0;
			}
		}

		std::cout << "There is no 1hop node: " << nbr1hopName << std::endl;
		return 0;
	}
	
	void TlNbrInfoFactory::Print(std::ostream &os) const
	{
		//os << "Neighbor information\n"; // in " << m_low->GetAddress() << '\n';
		uint8_t num = 41;
		uint16_t pdr_vector[num][num];
		for(uint8_t i = 0; i < num; i++)
			for(uint8_t j = 0; j < num; j++)
				pdr_vector[i][j] = 0;
		for(TlNbrInfoList::const_iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			uint8_t buffer1[6];
			i->nbrAddr.CopyTo(buffer1);
			pdr_vector[0][buffer1[5]-1] = pdr_vector[buffer1[5]-1][0] = i->nbrRcv;

			for(TlNbrInfoList::const_iterator j = i->nbr2hopList.begin(); j != i->nbr2hopList.end(); j++)
			{
				uint8_t buffer2[6];
				j->nbrAddr.CopyTo(buffer2);
				pdr_vector[buffer1[5]-1][buffer2[5]-1] = pdr_vector[buffer2[5]-1][buffer1[5]-1] = j->nbrRcv;
			}
		}
		for(uint8_t i = 0; i < num; i++){
			for(uint8_t j = 0; j < num; j++){
				os << pdr_vector[i][j] << " ";
			}
			os << '\n';
		}
		/*
		for(TlNbrInfoList::const_iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			os << i->nbrAddr << ": " << i->nbrRcv << ", " << i->tf_cnt << ", " << i->nr_cnt << '\n';
			for(TlNbrInfoList::const_iterator j = i->nbr2hopList.begin(); j != i->nbr2hopList.end(); j++)
			{
				os << "  " << j->nbrAddr << ": " << j->nbrRcv << ", " << j->tf_cnt << ", " << j->nr_cnt << '\n';
			}
		}
		*/
	}

	Mac48Address TlNbrInfoFactory::NextSTF(void)
	{
		for(TlNbrInfoList::iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->tf_cnt == false)
			{
		/*		if(i->nbrRcv < m_threshold)
				{
					i->tf_cnt = true;
					i->nr_cnt = true;
					if(EndSTF()){
						return Mac48Address();
						// End topology learning
					}
				}
				else
				{*/
					i->tf_cnt = true;
					i->nr_cnt = false;
					return i->nbrAddr;
				//}
			}
		}
		//std::cout << "NextSTF Error" << std::endl;
		// Cannot enter this
		return Mac48Address();
	}

	bool TlNbrInfoFactory::FillSTF(void)
	{
		for(TlNbrInfoList::iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->tf_cnt == true && i->nr_cnt == false)
			{
				i->nr_cnt = true;
				return true;
			}
		}
		return false;
	}

	bool TlNbrInfoFactory::EndSTF(void) const
	{
		for(TlNbrInfoList::const_iterator i = m_nbrList.begin(); i != m_nbrList.end(); i++)
		{
			if(i->nr_cnt == false)
				return false;
		}
		return true;
	}
	
	TlNbrInfoList TlNbrInfoFactory::GetNbrInfoList(void) const
	{
		return m_nbrList;
	}
}
