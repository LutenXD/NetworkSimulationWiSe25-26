//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "Junction.h"

Define_Module(Junction);

void Junction::initialize()
{
}

void Junction::handleMessage(cMessage* msg)
{
    if (std::string(msg->getArrivalGate()->getName()) == "north$i") send(msg, "south$o");
    if (std::string(msg->getArrivalGate()->getName()) == "east$i") send(msg, "west$o");
    if (std::string(msg->getArrivalGate()->getName()) == "south$i") send(msg, "north$o");
    if (std::string(msg->getArrivalGate()->getName()) == "west$i") send(msg, "east$o");
}
