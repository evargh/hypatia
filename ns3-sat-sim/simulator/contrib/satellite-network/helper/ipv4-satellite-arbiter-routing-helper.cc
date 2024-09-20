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

#include "ipv4-satellite-arbiter-routing-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4SatelliteArbiterRoutingHelper");

Ipv4SatelliteArbiterRoutingHelper::Ipv4SatelliteArbiterRoutingHelper() {
    // Left empty intentionally
}

Ipv4SatelliteArbiterRoutingHelper::Ipv4SatelliteArbiterRoutingHelper (const Ipv4SatelliteArbiterRoutingHelper &o) {
    // Left empty intentionally
}

Ipv4SatelliteArbiterRoutingHelper* Ipv4SatelliteArbiterRoutingHelper::Copy (void) const {
  return new Ipv4SatelliteArbiterRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol> Ipv4SatelliteArbiterRoutingHelper::Create (Ptr<Node> node) const {
  return CreateObject<Ipv4SatelliteArbiterRouting> ();
}

} // namespace ns3
