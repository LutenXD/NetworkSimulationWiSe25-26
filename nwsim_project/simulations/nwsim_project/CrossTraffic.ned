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

package nwsim_project.simulations.nwsim_project;

import ned.DelayChannel;
import nwsim_project.Junction;
import nwsim_project.Endpoint;

network CrossTraffic
{
    @display("bgb=425,425");
    submodules:
        JuncNW: Junction {
            @display("p=125,125");
        }
        JuncNE: Junction {
            @display("p=275,125");
        }
        JuncSE: Junction {
            @display("p=275,275");
        }
        JuncSW: Junction {
            @display("p=125,275");
        }

        EndNW: Endpoint {
            @display("p=125,50");
        }
        EndNE: Endpoint {
            @display("p=275,50");
        }
        EndEN: Endpoint {
            @display("p=350,125");
        }
        EndES: Endpoint {
            @display("p=350,275");
        }
        EndSE: Endpoint {
            @display("p=275,350");
        }
        EndSW: Endpoint {
            @display("p=125,350");
        }
        EndWS: Endpoint {
            @display("p=50,275");
        }
        EndWN: Endpoint {
            @display("p=50,125");
        }
    connections:
        JuncNW.east <--> {delay=100s;} <--> JuncNE.west;
        JuncNW.south <--> {delay=100s;} <--> JuncSW.north;
        JuncNE.south <--> {delay=100s;} <--> JuncSE.north;
        JuncSW.east <--> {delay=100s;} <--> JuncSE.west;

        EndNW.conn <--> {delay=100s;} <--> JuncNW.north;
        EndNE.conn <--> {delay=100s;} <--> JuncNE.north;
        EndEN.conn <--> {delay=100s;} <--> JuncNE.east;
        EndES.conn <--> {delay=100s;} <--> JuncSE.east;
        EndSE.conn <--> {delay=100s;} <--> JuncSE.south;
        EndSW.conn <--> {delay=100s;} <--> JuncSW.south;
        EndWS.conn <--> {delay=100s;} <--> JuncSW.west;
        EndWN.conn <--> {delay=100s;} <--> JuncNW.west;
}
