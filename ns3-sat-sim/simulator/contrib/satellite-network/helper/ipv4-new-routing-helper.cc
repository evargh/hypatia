/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

#include "ipv4-short-routing-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4ShortRoutingHelper");

Ipv4ShortRoutingHelper::Ipv4ShortRoutingHelper()
{
	// Left empty intentionally
}

Ipv4ShortRoutingHelper::Ipv4ShortRoutingHelper(const Ipv4ShortRoutingHelper &o)
{
	// Left empty intentionally
}

Ipv4ShortRoutingHelper *Ipv4ShortRoutingHelper::Copy(void) const
{
	return new Ipv4ShortRoutingHelper(*this);
}

Ptr<Ipv4RoutingProtocol> Ipv4ShortRoutingHelper::Create(Ptr<Node> node) const
{
	return CreateObject<Ipv4ShortRouting>();
}

} // namespace ns3
