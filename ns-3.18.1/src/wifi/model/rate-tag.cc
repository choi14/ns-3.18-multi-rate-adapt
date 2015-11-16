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

#include "rate-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RateTag);

TypeId
RateTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RateTag")
    .SetParent<Tag> ()
    .AddConstructor<RateTag> ()
    .AddAttribute ("Rate", "The rate of the multicast packet",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RateTag::Get),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId
RateTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

RateTag::RateTag ()
  : m_rate (0)
{
}

RateTag::RateTag (uint8_t rate)
  : m_rate (rate)
{
}

uint32_t
RateTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}
void
RateTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_rate);
}
void
RateTag::Deserialize (TagBuffer i)
{
  m_rate = i.ReadU8 ();
}
void
RateTag::Print (std::ostream &os) const
{
  os << "Rate=" << m_rate;
}
void
RateTag::Set (uint8_t rate)
{
  m_rate = rate;
}
uint8_t
RateTag::Get (void) const
{
  return m_rate;
}


}
