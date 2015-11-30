/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2013 University of Surrey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 * Author: Konstantinos Katsaros <dinos.katsaros@gmail.com>
 */

#include "coefficient-header.h"
#include "ns3/double.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CoefficientHeader);

TypeId
CoefficientHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CoefficientHeader")
    .SetParent<Header> ()
    .AddConstructor<CoefficientHeader> ()
  ;
  return tid;
}

TypeId
CoefficientHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

CoefficientHeader::CoefficientHeader ()
{
}

CoefficientHeader::CoefficientHeader (uint16_t eid, uint8_t p, uint8_t k, std::vector<uint8_t> coeffi)
{
	m_eid = eid;
	m_p = p;
	m_k = k;
	m_seq = 0;
	m_n = k;
	m_coeffi = coeffi;
}

CoefficientHeader::CoefficientHeader (uint16_t eid, uint8_t p, uint8_t k, uint16_t seq, std::vector<uint8_t> coeffi)
{
	m_eid = eid;
	m_p = p;
	m_k = k;
	m_seq = seq;
	m_n = k;
	m_coeffi = coeffi;
}

CoefficientHeader::CoefficientHeader (uint16_t eid, uint8_t p, uint8_t k, uint8_t n, uint16_t seq, std::vector<uint8_t> coeffi)  // Use this
{
	m_eid = eid;
	m_p = p;
	m_k = k;
	m_seq = seq;
	m_n = n;
	m_coeffi = coeffi;
}

uint32_t
CoefficientHeader::GetSerializedSize (void) const
{
  return (uint32_t) 7+m_p*m_p*m_k;
	//std::cout << "GetSerializedSize" << (uint16_t) (4 + m_p * m_p * m_k) << std::endl;
}

void
CoefficientHeader::Serialize (Buffer::Iterator i) const
{
	i.WriteU16(m_eid);
	i.WriteU8(m_p);
	i.WriteU8(m_k);
	i.WriteU8(m_n);
    i.WriteU16(m_seq);
	uint32_t size = (uint32_t)m_p*m_p*m_k;
	for (uint32_t j = 0; j < size; j++)
	{
		i.WriteU8 (m_coeffi[j]);
	}
}

uint32_t
CoefficientHeader::Deserialize (Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_eid = i.ReadU16 ();
	m_p = i.ReadU8 ();
	m_k = i.ReadU8 ();
	m_n = i.ReadU8 ();
    m_seq = i.ReadU16 ();
//	std::cout << "CoefficientHeader::Deserialize : " << (uint16_t) m_eid << ", " << (uint16_t) m_p << ", " << (uint16_t) m_k << ", ";

	uint32_t size = (uint32_t)m_p*m_p*m_k;
	for (uint32_t j = 0; j < size; j++)
	{
		uint8_t value = i.ReadU8();
		m_coeffi.push_back(value);
		//std::cout << (uint16_t) value << " ";
	}
	//std::cout << std::endl;
	return i.GetDistanceFrom (start);
}

void
CoefficientHeader::Print (std::ostream &os) const
{
	os << "m_p=" << m_p;
}

void
CoefficientHeader::SetP (uint8_t p)
{
	m_p = p;
}

uint8_t
CoefficientHeader::GetP (void) const
{
	return m_p;
}
void
CoefficientHeader::SetK (uint8_t k)
{
	m_k = k;
}

uint8_t
CoefficientHeader::GetK (void) const
{
	return m_k;
}

void
CoefficientHeader::SetN (uint8_t n)
{
	m_n = n;
}

uint8_t
CoefficientHeader::GetN (void) const
{
	return m_n;
}

void
CoefficientHeader::SetEid (uint16_t eid)
{
	m_eid = eid;
}

uint16_t
CoefficientHeader::GetEid (void) const
{
	return m_eid;
}
void
CoefficientHeader::SetSeq (uint16_t seq)
{
  m_seq = seq;
}

uint16_t
CoefficientHeader::GetSeq (void) const
{
  return m_seq;
}



void
CoefficientHeader::SetCoeffi (std::vector<uint8_t> coeffi)
{
	m_coeffi = coeffi;
}

std::vector<uint8_t> 
CoefficientHeader::GetCoeffi (void) 
{
	return m_coeffi;
}
}
