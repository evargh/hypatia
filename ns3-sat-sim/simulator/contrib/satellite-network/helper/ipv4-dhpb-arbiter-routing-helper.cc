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

#include "ipv4-dhpb-arbiter-routing-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4DhpbArbiterRoutingHelper");

Ipv4DhpbArbiterRoutingHelper::Ipv4DhpbArbiterRoutingHelper()
{
	// Left empty intentionally
}

Ipv4DhpbArbiterRoutingHelper::Ipv4DhpbArbiterRoutingHelper(const Ipv4DhpbArbiterRoutingHelper &o)
{
	// Left empty intentionally
}

Ipv4DhpbArbiterRoutingHelper *Ipv4DhpbArbiterRoutingHelper::Copy(void) const
{
	return new Ipv4DhpbArbiterRoutingHelper(*this);
}

Ptr<Ipv4RoutingProtocol> Ipv4DhpbArbiterRoutingHelper::Create(Ptr<Node> node) const
{
	return CreateObject<Ipv4DhpbArbiterRouting>();
}

} // namespace ns3
