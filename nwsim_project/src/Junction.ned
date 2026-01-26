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

package src;

simple Junction
{
    parameters:
        @display("i=block/routing");
        string junctionMode = default("queue"); // Junction operation mode: "queue", "circle", or "roundrobin"
        
        @signal[queueLength](type="long");
        @signal[junctionProcessingTime](type="simtime_t");
        @signal[vehicleCount](type="long");
        
        @statistic[queueLength](title="Queue Length"; source="queueLength"; record=vector,stats; interpolationmode=sample-hold);
        @statistic[junctionProcessingTime](title="Junction Processing Time"; source="junctionProcessingTime"; record=vector,stats);
        @statistic[vehicleCount](title="Vehicle Count"; source="vehicleCount"; record=vector,stats);
        
    gates:
        inout north;
        inout east;
        inout south;
        inout west;
}
