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

message Vehicle {
    string srcEndpoint;
    string dstEndpoint;
    unsigned int vehNumber;
    simtime_t startTime;          // When the vehicle was created
    simtime_t totalJunctionTime;  // Total time spent at all junctions
    unsigned int junctionCount;   // Number of junctions visited
}

cd /workspaces/skeleton/cms_detour/simulations ; opp_run -l ../libcms_sheet2_detour_v2_skeleton.so -l /opt/veins-5.3.1/src/libveins.so -n ../src:.:/opt/veins-5.3.1/src/veins -u Cmdenv -c DistributedForwarding omnetpp.ini
cd /workspaces/nwsim_project/simulations/nwsim_project ; opp_run -l ..