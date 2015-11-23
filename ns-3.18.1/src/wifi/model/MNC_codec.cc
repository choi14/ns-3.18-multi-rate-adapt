#include "MNC_codec.h"
#include <stdio.h>
#include "ns3/random-variable.h"
#include <iostream>
#include "ns3/log.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE ("MncCodec");

namespace ns3{

	/* Log table using 0xe5 (229) as the generator */
	uint8_t ltable[256] = {
		0x00, 0xff, 0xc8, 0x08, 0x91, 0x10, 0xd0, 0x36, 
		0x5a, 0x3e, 0xd8, 0x43, 0x99, 0x77, 0xfe, 0x18, 
		0x23, 0x20, 0x07, 0x70, 0xa1, 0x6c, 0x0c, 0x7f, 
		0x62, 0x8b, 0x40, 0x46, 0xc7, 0x4b, 0xe0, 0x0e, 
		0xeb, 0x16, 0xe8, 0xad, 0xcf, 0xcd, 0x39, 0x53, 
		0x6a, 0x27, 0x35, 0x93, 0xd4, 0x4e, 0x48, 0xc3, 
		0x2b, 0x79, 0x54, 0x28, 0x09, 0x78, 0x0f, 0x21, 
		0x90, 0x87, 0x14, 0x2a, 0xa9, 0x9c, 0xd6, 0x74, 
		0xb4, 0x7c, 0xde, 0xed, 0xb1, 0x86, 0x76, 0xa4, 
		0x98, 0xe2, 0x96, 0x8f, 0x02, 0x32, 0x1c, 0xc1, 
		0x33, 0xee, 0xef, 0x81, 0xfd, 0x30, 0x5c, 0x13, 
		0x9d, 0x29, 0x17, 0xc4, 0x11, 0x44, 0x8c, 0x80, 
		0xf3, 0x73, 0x42, 0x1e, 0x1d, 0xb5, 0xf0, 0x12, 
		0xd1, 0x5b, 0x41, 0xa2, 0xd7, 0x2c, 0xe9, 0xd5, 
		0x59, 0xcb, 0x50, 0xa8, 0xdc, 0xfc, 0xf2, 0x56, 
		0x72, 0xa6, 0x65, 0x2f, 0x9f, 0x9b, 0x3d, 0xba, 
		0x7d, 0xc2, 0x45, 0x82, 0xa7, 0x57, 0xb6, 0xa3, 
		0x7a, 0x75, 0x4f, 0xae, 0x3f, 0x37, 0x6d, 0x47, 
		0x61, 0xbe, 0xab, 0xd3, 0x5f, 0xb0, 0x58, 0xaf, 
		0xca, 0x5e, 0xfa, 0x85, 0xe4, 0x4d, 0x8a, 0x05, 
		0xfb, 0x60, 0xb7, 0x7b, 0xb8, 0x26, 0x4a, 0x67, 
		0xc6, 0x1a, 0xf8, 0x69, 0x25, 0xb3, 0xdb, 0xbd, 
		0x66, 0xdd, 0xf1, 0xd2, 0xdf, 0x03, 0x8d, 0x34, 
		0xd9, 0x92, 0x0d, 0x63, 0x55, 0xaa, 0x49, 0xec, 
		0xbc, 0x95, 0x3c, 0x84, 0x0b, 0xf5, 0xe6, 0xe7, 
		0xe5, 0xac, 0x7e, 0x6e, 0xb9, 0xf9, 0xda, 0x8e, 
		0x9a, 0xc9, 0x24, 0xe1, 0x0a, 0x15, 0x6b, 0x3a, 
		0xa0, 0x51, 0xf4, 0xea, 0xb2, 0x97, 0x9e, 0x5d, 
		0x22, 0x88, 0x94, 0xce, 0x19, 0x01, 0x71, 0x4c, 
		0xa5, 0xe3, 0xc5, 0x31, 0xbb, 0xcc, 0x1f, 0x2d, 
		0x3b, 0x52, 0x6f, 0xf6, 0x2e, 0x89, 0xf7, 0xc0, 
		0x68, 0x1b, 0x64, 0x04, 0x06, 0xbf, 0x83, 0x38 };

	/* Anti-log table: */
	uint8_t atable[256] = {
		0x01, 0xe5, 0x4c, 0xb5, 0xfb, 0x9f, 0xfc, 0x12, 
		0x03, 0x34, 0xd4, 0xc4, 0x16, 0xba, 0x1f, 0x36, 
		0x05, 0x5c, 0x67, 0x57, 0x3a, 0xd5, 0x21, 0x5a, 
		0x0f, 0xe4, 0xa9, 0xf9, 0x4e, 0x64, 0x63, 0xee, 
		0x11, 0x37, 0xe0, 0x10, 0xd2, 0xac, 0xa5, 0x29, 
		0x33, 0x59, 0x3b, 0x30, 0x6d, 0xef, 0xf4, 0x7b, 
		0x55, 0xeb, 0x4d, 0x50, 0xb7, 0x2a, 0x07, 0x8d, 
		0xff, 0x26, 0xd7, 0xf0, 0xc2, 0x7e, 0x09, 0x8c, 
		0x1a, 0x6a, 0x62, 0x0b, 0x5d, 0x82, 0x1b, 0x8f, 
		0x2e, 0xbe, 0xa6, 0x1d, 0xe7, 0x9d, 0x2d, 0x8a, 
		0x72, 0xd9, 0xf1, 0x27, 0x32, 0xbc, 0x77, 0x85, 
		0x96, 0x70, 0x08, 0x69, 0x56, 0xdf, 0x99, 0x94, 
		0xa1, 0x90, 0x18, 0xbb, 0xfa, 0x7a, 0xb0, 0xa7, 
		0xf8, 0xab, 0x28, 0xd6, 0x15, 0x8e, 0xcb, 0xf2, 
		0x13, 0xe6, 0x78, 0x61, 0x3f, 0x89, 0x46, 0x0d, 
		0x35, 0x31, 0x88, 0xa3, 0x41, 0x80, 0xca, 0x17, 
		0x5f, 0x53, 0x83, 0xfe, 0xc3, 0x9b, 0x45, 0x39, 
		0xe1, 0xf5, 0x9e, 0x19, 0x5e, 0xb6, 0xcf, 0x4b, 
		0x38, 0x04, 0xb9, 0x2b, 0xe2, 0xc1, 0x4a, 0xdd, 
		0x48, 0x0c, 0xd0, 0x7d, 0x3d, 0x58, 0xde, 0x7c, 
		0xd8, 0x14, 0x6b, 0x87, 0x47, 0xe8, 0x79, 0x84, 
		0x73, 0x3c, 0xbd, 0x92, 0xc9, 0x23, 0x8b, 0x97, 
		0x95, 0x44, 0xdc, 0xad, 0x40, 0x65, 0x86, 0xa2, 
		0xa4, 0xcc, 0x7f, 0xec, 0xc0, 0xaf, 0x91, 0xfd, 
		0xf7, 0x4f, 0x81, 0x2f, 0x5b, 0xea, 0xa8, 0x1c, 
		0x02, 0xd1, 0x98, 0x71, 0xed, 0x25, 0xe3, 0x24, 
		0x06, 0x68, 0xb3, 0x93, 0x2c, 0x6f, 0x3e, 0x6c, 
		0x0a, 0xb8, 0xce, 0xae, 0x74, 0xb1, 0x42, 0xb4, 
		0x1e, 0xd3, 0x49, 0xe9, 0x9c, 0xc8, 0xc6, 0xc7, 
		0x22, 0x6e, 0xdb, 0x20, 0xbf, 0x43, 0x51, 0x52, 
		0x66, 0xb2, 0x76, 0x60, 0xda, 0xc5, 0xf3, 0xf6, 
		0xaa, 0xcd, 0x9a, 0xa0, 0x75, 0x54, 0x0e, 0x01 };


	/********************* Encoder part ***************************/

	MNC_Encoder::MNC_Encoder()
	{
		m_MNCbuffer_K = CreateObject<WifiMacQueue> ();
		m_MNCbuffer_N = CreateObject<WifiMacQueue> ();
		m_last_eid = 0;
		m_last_seq = 0;
	}

	MNC_Encoder::~MNC_Encoder()
	{
	}

	void MNC_Encoder::MNC_Enqueue(Ptr<const Packet> packet, WifiMacHeader hdr){
		m_MNCbuffer_K->Enqueue(packet, hdr);
	}

	Ptr<const Packet> MNC_Encoder::MNC_Dequeue(WifiMacHeader *hdr){
		return m_MNCbuffer_N->Dequeue(hdr);
	}

	uint32_t MNC_Encoder::GetBufferSize() const{
		return m_MNCbuffer_K->GetSize();
	}

	void MNC_Encoder::FlushAllBuffer(){
		m_MNCbuffer_K->Flush();
		m_MNCbuffer_N->Flush();
	}

	void MNC_Encoder::FlushN(){
			m_MNCbuffer_N->Flush();
	}
	
	void MNC_Encoder::FlushK(){
			m_MNCbuffer_K->Flush();
	}


	void MNC_Encoder::WithCoeffEncoding(uint16_t eid, uint8_t num, uint8_t n, uint8_t p, bool sys){
		if(p != 1 || sys != false)
			return;

		WifiMacHeader tempheader;
		uint8_t k = 0;
		Ptr<Packet> temp;
		std::vector<uint8_t> tempvector[num];
		Ptr<WifiMacQueue> tempbuffer = CreateObject<WifiMacQueue> ();
		uint16_t seq = 0;

		for(uint8_t i = 0; i < num; i++){
			Ptr<const Packet> temppacket = m_MNCbuffer_K->Dequeue(&tempheader);
			temp = temppacket->Copy();
			CoefficientHeader coeffi;
			temp->RemoveHeader(coeffi);
			
			k = coeffi.GetK();
			tempvector[i] = coeffi.GetCoeffi();
			tempbuffer->Enqueue(temp, tempheader);
		}
		
		//std::cout << "num = " << (uint16_t) num << " k = " << (uint16_t) k << std::endl;
		/*
		for(uint8_t j = 0; j < k; j++){
			for(uint8_t i = 0; i < num; i++)
		//		std::cout << "tempvector[i].at[j] = " << (uint16_t) tempvector[i].at(j) << " ";
		//	std::cout << std::endl;
		}*/

		for(uint8_t l = 0; l < n; l++){
			WifiMacHeader tmphdr = tempheader;
			Ptr<Packet> sendpacket = temp->Copy();

			std::vector<uint8_t> coeffi;
			UniformVariable a;
			for(uint8_t j = 0; j < k; j++){
				for(uint8_t i = 0; i < num; i++){
					if(i == 0)
						coeffi.push_back(0);
					
					uint8_t rv = (uint8_t)a.GetInteger(0,255);
					coeffi[j] = gadd(coeffi[j], gmul(tempvector[i].at(j), rv));
				}
			}
			if (eid > m_last_eid){
					seq = l+1;
			}
			else if(eid == m_last_eid){
					seq = m_last_seq + l + 1;
			}
			CoefficientHeader coeffi_hdr(eid, p, k, seq, coeffi);
			sendpacket->AddHeader(coeffi_hdr);
			m_MNCbuffer_N->Enqueue(sendpacket, tmphdr);
		}
		while(tempbuffer->GetSize() > 0){
				Ptr<const Packet> temppacket = tempbuffer->Dequeue(&tempheader);
				Ptr<Packet> packet3 = Create<Packet>();
				packet3 = temppacket->Copy();
				m_MNCbuffer_K->Enqueue(packet3, tempheader);
		}
		m_last_eid = eid;
		m_last_seq = seq;
	}



	void MNC_Encoder::Encoding(uint16_t eid, uint8_t k, uint8_t n, uint8_t p, bool sys){
		WifiMacHeader tempheader;
		Ptr<const Packet> temppacket = m_MNCbuffer_K->Dequeue(&tempheader);
		Ptr<Packet> temp2 = temppacket->Copy();
		WifiMacHeader tmp_hdr2 = tempheader;

		uint16_t seq = 0; 

		if(k <= n && sys && p == 1){
			for(uint8_t i = 0; i < n; i++){
				WifiMacHeader tmphdr = tempheader;
				Ptr<Packet> temp = temppacket->Copy();
				std::vector<uint8_t> coeffi;
				if(i < k){
					for(uint8_t j = 0; j < k; j++){
						if(j == i)
							coeffi.push_back(1);
						else
							coeffi.push_back(0);
					}
				}
				else{
					UniformVariable a;
					for(uint32_t j = 0; j < (uint32_t)p*p*k; j++){
						uint8_t rv = (uint8_t)a.GetInteger(0,255);
						coeffi.push_back(rv);
					}
				}
				if(eid > m_last_eid){
						seq = i+1;
				}
				else if(eid == m_last_eid){
						seq = m_last_seq + i + 1;
				}
				CoefficientHeader coeffi_hdr(eid, p, k, seq, coeffi);
				temp->AddHeader(coeffi_hdr);
				m_MNCbuffer_N->Enqueue(temp, tmphdr);
				//std::cout << "temppacket = " << temppacket->GetSize() << std::endl;
				//std::cout << "temp = " << temp->GetSize() << std::endl;
			}
		}
		else{
			for(uint8_t i = 0; i < n; i++){
				WifiMacHeader tmphdr = tempheader;
				Ptr<Packet> temp = temppacket->Copy();

				std::vector<uint8_t> coeffi;
				UniformVariable a;
				for(uint32_t j = 0; j < (uint32_t)p*p*k; j++){
					uint8_t rv = (uint8_t)a.GetInteger(0,255);
					coeffi.push_back(rv);
				}
				if(eid > m_last_eid){
						seq = i+1;
				}
				else if(eid == m_last_eid){
						seq = m_last_seq + i + 1;
				}
				CoefficientHeader coeffi_hdr(eid, p, k, seq, coeffi);
				temp->AddHeader(coeffi_hdr);
				m_MNCbuffer_N->Enqueue(temp, tmphdr);
			}
		}
		m_MNCbuffer_K->Enqueue(temp2, tmp_hdr2);
		m_last_eid = eid;
		m_last_seq = seq;
	}

	uint8_t MNC_Encoder::gadd(uint8_t a, uint8_t b) {
		return a ^ b;
	}

	uint8_t MNC_Encoder::gsub(uint8_t a, uint8_t b) {
		return a ^ b;
	}

	uint8_t MNC_Encoder::gmul(uint8_t a, uint8_t b) {
		int s;
		int q;
		int z = 0;
		s = ltable[a] + ltable[b];
		s %= 255;
		/* Get the antilog */
		s = atable[s];
		/* Now, we have some fancy code that returns 0 if either
			 a or b are zero; we write the code this way so that the
			 code will (hopefully) run at a constant speed in order to
			 minimize the risk of timing attacks */
		q = s;
		if(a == 0) {
			s = z;
		}
		else {
			s = q;
		}
		if(b == 0) {
			s = z;
		} 
		else {
			q = z;
		}
		return s;
	}

	uint8_t MNC_Encoder::gmul_inverse(uint8_t in) {
		/* 0 is self inverting */
		if(in == 0) 
			return 0;
		else
			return atable[(255 - ltable[in])];
	}

	// a / b
	uint8_t MNC_Encoder::gdiv(uint8_t a, uint8_t b){
		if(b == 0) return 0;
		return gmul(a, gmul_inverse(b));
	}

	/********************* Decoder part ***************************/

	MNC_Decoder::MNC_Decoder()
	{
		m_eid = 0;
		m_dtime = Simulator::Now().GetMilliSeconds();
		m_systematic = false;
		m_MNCbuffer_Rx = CreateObject<WifiMacQueue> ();
		m_MNCbuffer_Decoded = CreateObject<WifiMacQueue> ();
		m_MNCbuffer_Encoded = CreateObject<WifiMacQueue> ();
		m_MNCbuffer_Sys = CreateObject<WifiMacQueue> ();
	}

	MNC_Decoder::~MNC_Decoder()
	{
	}

	void MNC_Decoder::MNC_Rx_Enqueue(Ptr<const Packet> packet, const WifiMacHeader *hdr){
		m_MNCbuffer_Rx->Enqueue(packet, *hdr);
	}

	void MNC_Decoder::MNC_Rx_EEnqueue(Ptr<const Packet> packet, const WifiMacHeader *hdr){
		m_MNCbuffer_Encoded->Enqueue(packet, *hdr);
	}

	Ptr<const Packet> MNC_Decoder::MNC_Rx_Dequeue(WifiMacHeader *hdr){
		return m_MNCbuffer_Rx->Dequeue(hdr);
	}
	Ptr<const Packet> MNC_Decoder::MNC_Rx_DDequeue(WifiMacHeader *hdr){
		return m_MNCbuffer_Decoded->Dequeue(hdr);
	}

	Ptr<const Packet> MNC_Decoder::MNC_Rx_EDequeue(WifiMacHeader *hdr){
		return m_MNCbuffer_Encoded->Dequeue(hdr);
	}

	Ptr<const Packet> MNC_Decoder::MNC_Rx_SDequeue(WifiMacHeader *hdr){
		return m_MNCbuffer_Sys->Dequeue(hdr);
	}

	uint32_t MNC_Decoder::GetBufferSize() const{
		return m_MNCbuffer_Rx->GetSize();
	}

	uint32_t MNC_Decoder::GetDBufferSize() const{
		return m_MNCbuffer_Decoded->GetSize();
	}

	uint32_t MNC_Decoder::GetEBufferSize() const{
		return m_MNCbuffer_Encoded->GetSize();
	}

	uint32_t MNC_Decoder::GetSBufferSize() const{
		return m_MNCbuffer_Sys->GetSize();
	}

	void MNC_Decoder::SetEid(uint16_t eid){
		m_eid = eid;
	}

	uint16_t MNC_Decoder::GetEid() const{
		return m_eid;
	}
	/*
		 void MNC_Decoder::SetDtime(int64_t dtime){
		 m_dtime = dtime;
		 }
		 */
	int64_t MNC_Decoder::GetDtime() const{
		return m_dtime;
	}

	bool MNC_Decoder::Decoding(uint8_t k, uint8_t p, bool stamp){
		uint16_t rcv_num = m_MNCbuffer_Rx->GetSize();
		//std::cout << "rcv_num : " << rcv_num << std::endl;
		uint8_t *Coeff_Matrix = new uint8_t[rcv_num*k*p*p];
		std::vector<uint8_t> tempvector;
		uint16_t cm_count = 0;
		//uint16_t id = 0;
		WifiMacHeader tempheader;
		Ptr<Packet> ori_packet = Create<Packet>();
		Ptr<WifiMacQueue> tempRxBuffer = CreateObject<WifiMacQueue> ();

		// Make coefficient matrix from recevied packets
		while(m_MNCbuffer_Rx->GetSize() != 0){
			Ptr<const Packet> temppacket = m_MNCbuffer_Rx->Dequeue(&tempheader);
			//NS_LOG_DEBUG("size temppacket" << temppacket->GetSize());
			Ptr<Packet> packet2 = Create<Packet>();
			packet2 = temppacket->Copy();
			tempRxBuffer->Enqueue(packet2, tempheader);

			ori_packet = temppacket->Copy();
			CoefficientHeader coeffi_hdr;
			ori_packet->RemoveHeader(coeffi_hdr);	

			//id = coeffi_hdr.GetEid();
			
			//if(m_eid != id){
			//	std::cout << "ERRRRRRORR" << std::endl;
			//}

			coeffi_hdr.GetP();
			coeffi_hdr.GetK();
			tempvector = coeffi_hdr.GetCoeffi();

			for(uint16_t i = 0; i < tempvector.size(); i++){
				Coeff_Matrix[cm_count] = tempvector[i];
				cm_count++;
			}
			//NS_LOG_DEBUG("ID: " << id << " m_MNCbuffer_Rx2  " << m_MNCbuffer_Rx->GetSize() << "  size of coeff: " << tempvector.size() << "  size of Coeff Matrix: " << cm_count);
		}	

		NS_LOG_DEBUG("m_MNCbuffer_Rx3  " << m_MNCbuffer_Rx->GetSize());			
		uint16_t rank = LU_decoding(Coeff_Matrix, rcv_num, k, p); // Get the rank of Coeffi_Matrix which is composed of the MNC coefficients of the received packets

		if(rank != k*p){ // Check full rank or not
			while(tempRxBuffer->GetSize() > 0){
				Ptr<const Packet> temppacket = tempRxBuffer->Dequeue(&tempheader);
				Ptr<Packet> packet3 = Create<Packet>();
				packet3 = temppacket->Copy();
				m_MNCbuffer_Rx->Enqueue(packet3, tempheader);
			}
			return false;
		}
		//std::cout << "m_eid: " << m_eid << " -> Decoding success" << std::endl;

		for(int i = 0; i < k; i++){
			Ptr<Packet> temp = Create<Packet>();
			temp = ori_packet->Copy();
			m_MNCbuffer_Decoded->Enqueue(temp, tempheader);
		}

		if(stamp) std::cout << "m_MNCbuffer_Rx " << rcv_num << std::endl;
		return true;
	}
	
	void MNC_Decoder::Systematicing(){
		WifiMacHeader tempheader;
		std::vector<uint8_t> tempvector;
		Ptr<Packet> ori_packet = Create<Packet>();
		uint16_t id = 0;

		while(m_MNCbuffer_Rx->GetSize() != 0){
			Ptr<const Packet> temppacket = m_MNCbuffer_Rx->Dequeue(&tempheader);
			//NS_LOG_DEBUG("size temppacket" << temppacket->GetSize());
			ori_packet = temppacket->Copy();
			CoefficientHeader coeffi_hdr;
			ori_packet->RemoveHeader(coeffi_hdr);	

			id = coeffi_hdr.GetEid();
			if(m_eid != id){
				std::cout << "ERRRRRRORR" << std::endl;
			}

			coeffi_hdr.GetP();
			coeffi_hdr.GetK();
			tempvector = coeffi_hdr.GetCoeffi();
			bool issys = false;

			for(uint16_t i = 0 ; i < tempvector.size(); i++){
				if(issys == true){
					if(tempvector[i] == 0){
						continue;
					}
					else if(tempvector[i] == 1){
						issys = false;
						break;
					}
					else{
						issys = false;
						break;
					}
				}
				else{
					if(tempvector[i] == 0){
						continue;
					}
					else if(tempvector[i] == 1){
						issys = true;
						continue;
					}
					else{
						issys = false;
						break;
					}
				}
			}

			if(issys)
				m_MNCbuffer_Sys->Enqueue(ori_packet, tempheader);

			//std::cout << m_MNCbuffer_Rx->GetSize() << " " << m_MNCbuffer_Sys->GetSize() << "  " << ori_packet->GetSize() << std::endl;
		}	
	}
/*
	std::vector<uint8_t> MNC_Decoder::GetCoeff(Ptr<const Packet> temppacket){
		NS_LOG_DEBUG("GetCoeffi ");
		Ptr<Packet> packet = temppacket->Copy();
		CoefficientHeader coeffi_hdr;
		packet->RemoveHeader(coeffi_hdr);	
		NS_LOG_DEBUG("coeffi_hdr p: " << coeffi_hdr.GetP());
		coeffi_hdr.GetEid();
		coeffi_hdr.GetP();
		coeffi_hdr.GetK();
		std::vector<uint8_t> coeffi = coeffi_hdr.GetCoeffi();

		return coeffi;
	}
*/
	uint8_t MNC_Decoder::gadd(uint8_t a, uint8_t b) {
		return a ^ b;
	}

	uint8_t MNC_Decoder::gsub(uint8_t a, uint8_t b) {
		return a ^ b;
	}
	uint8_t MNC_Decoder::gmul(uint8_t a, uint8_t b) {
		int s;
		int q;
		int z = 0;
		s = ltable[a] + ltable[b];
		s %= 255;
		/* Get the antilog */
		s = atable[s];
		/* Now, we have some fancy code that returns 0 if either
			 a or b are zero; we write the code this way so that the
			 code will (hopefully) run at a constant speed in order to
			 minimize the risk of timing attacks */
		q = s;
		if(a == 0) {
			s = z;
		}
		else {
			s = q;
		}
		if(b == 0) {
			s = z;
		} 
		else {
			q = z;
		}
		return s;
	}

	uint8_t MNC_Decoder::gmul_inverse(uint8_t in) {
		/* 0 is self inverting */
		if(in == 0) 
			return 0;
		else
			return atable[(255 - ltable[in])];
	}

	// a / b
	uint8_t MNC_Decoder::gdiv(uint8_t a, uint8_t b){
		if(b == 0) return 0;
		return gmul(a, gmul_inverse(b));
	}

	uint8_t MNC_Decoder::LU_decoding(uint8_t *coeffi, uint16_t rcv_num, uint8_t k, uint8_t p){
		uint16_t usize = rcv_num*p;
		uint16_t msize = k*p;
		//if(m_eid == 63) std::cout << "usize = " << usize << std::endl;
		//NS_LOG_DEBUG("msize: " << msize << " MNC_K: " << (uint16_t)k << " MNC_p: " << (uint16_t)p << " n_rx:"<< rcv_num);
		uint8_t** coefficient = NULL;
		uint8_t** U = NULL;
		uint8_t** L = NULL;
		unsigned int i, j, m, n;
		unsigned int mrow;
		unsigned int rank = msize;

		coefficient = new uint8_t* [usize];
		U = new uint8_t* [usize];
		L = new uint8_t* [usize];

		for(uint16_t i = 0; i < usize; i++){		
			(coefficient[i]) = new uint8_t [msize];
			(U[i]) = new uint8_t [msize];
			(L[i]) = new uint8_t [msize];
		}

		uint32_t count = 0;
		// Fill coefficient matrix
		for(i = 0 ; i < rcv_num; i++){
			mrow = i*p;
			for(j = 0; j < msize; j+=p){
				for(m = 0; m < p; m++){
					for(n = 0; n < p; n++){
						coefficient[mrow+m][j+n] = coeffi[count];
						count++;
					}
				}
			}
		}
		for(i = rcv_num; i < k; i++){
			mrow = i * p;
			for(j = 0; j < msize; j+=p){
				for(m = 0; m < p; m++){
					for(n = 0; n < p; n++){
						coefficient[mrow+m][j+n] = 0;
					}
				}
			}
		}

		// Arr = LU
		lu_fac(usize, msize, coefficient, U, L);

		// Check rank
		for(i = 0; i < msize; i++){
			if(U[i][i] == 0){
				rank--;
			}
		}
		NS_LOG_DEBUG("rank: " << rank);

		for(uint16_t i = 0; i < usize; i++){		
			delete[] (coefficient[i]);
			delete[] (U[i]);
			delete[] (L[i]);
		}

		delete[] coefficient;
		delete[] U;
		delete[] L;

		return rank;
	}

	void MNC_Decoder::lu_fac(uint8_t m, uint8_t n, uint8_t **Arr, uint8_t **U, uint8_t **L){

		// Arr : m x n
		// U   : n x n
		// L   : n x n

		uint8_t i, j, k;
		uint8_t change;
		uint8_t temp_c;

		//if(m > n) return; // Only m <= n

		for(i = 0; i < m; i++){
			for(j = 0; j < n; j++){
				U[i][j] = Arr[i][j];
			}
		}

		for(i = 0; i < n; i++){
			for(j = 0; j < n; j++){
				L[i][j] = 0;
			}
		}

		for(j = 0; j < n; j++){
			L[j][j] = 1;
			for(i = j + 1; i < m; i++){
				if(U[j][j] == 0){
					for(k = j + 1; k < m; k++){
						if(U[k][j] != 0) break;
					}
					change = k;
					if(change == m) continue;

					for(k = 0; k < n; k++){
						temp_c = Arr[j][k];
						Arr[j][k] = Arr[change][k];
						Arr[change][k] = temp_c;
					}

					for(k = j; k < n; k++){
						temp_c = U[j][k];
						U[j][k] = U[change][k];
						U[change][k] = temp_c;
					}

					for(k = 0; k < j; k++){
						temp_c = L[j][k];
						L[j][k] = L[change][k];
						L[change][k] = temp_c;
					}
				}

				/*
					 if(m_eid == 63){
					 printf("\nU i = %d, j = %d\n", i, j);
					 for(int o = 0; o < m; o++){
					 for(int l = 0; l < n; l++)
					 printf("%d ", U[o][l]);
					 printf("\n");
					 }
					 printf("\nU[i][j] = %d, U[j][j] = %d\n", U[i][j], U[j][j]);
					 }
					 */

				L[i][j] = gdiv(U[i][j], U[j][j]);
				if(L[i][j] == 0) continue;
				for(k = j; k < n; k++){
					U[i][k] = gsub(U[i][k], gmul(U[j][k], L[i][j]));
				}
			}
		}
	}
}
