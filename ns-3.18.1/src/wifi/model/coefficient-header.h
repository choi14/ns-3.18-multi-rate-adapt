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

#ifndef COEFFICIENT_HEADER_H
#define COEFFICIENT_HEADER_H

#include "ns3/header.h"
#include <stdint.h>

namespace ns3 {


class CoefficientHeader : public Header
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create a CoefficientTag with the default snr 0 
   */
  CoefficientHeader();
  CoefficientHeader(uint16_t eid, uint8_t p, uint8_t k, uint16_t seq, std::vector<uint8_t> coeffi);
  CoefficientHeader(uint16_t eid, uint8_t p, uint8_t k, std::vector<uint8_t> coeffi);

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * Set the SNR to the given value.
   *
   * @param snr the value of the snr to set
   */
  void SetP (uint8_t p);
  void SetK (uint8_t k);
  void SetEid (uint16_t id);
  void SetSeq (uint16_t seq);
  uint8_t GetP (void) const;
  uint8_t GetK (void) const;
  uint16_t GetEid (void) const;
  uint16_t GetSeq (void) const;

  void SetCoeffi (std::vector<uint8_t> coeffi);
  std::vector<uint8_t> GetCoeffi (void);

private:
  uint16_t m_eid;
  uint16_t m_seq;
  uint8_t m_p;
  uint8_t m_k;
  std::vector<uint8_t> m_coeffi;

};


}
#endif /* SNR_TAG_H */
