//# tCoordClient.cc: Test program for class CoordClient
//#
//# Copyright (C) 2002
//# ASTRON (Netherlands Foundation for Research in Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This program is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//# $Id$

#include <Coord/CoordClient.h>
#include <Coord/SkyCoord.h>
#include <Coord/EarthCoord.h>
#include <Coord/TimeCoord.h>
#include <stdexcept>

int main (int argc, const char* argv[])
{
  try {
    SkyCoord sky(1,0.4);
    Assert (sky.angle1() == 1);
    Assert (sky.angle2() == 0.4);
    cout << sky << endl;
    EarthCoord pos(0.2, 1, 3);
    Assert (pos.longitude() == 0.2);
    Assert (pos.latitude() == 1);
    Assert (pos.height() == 3);
    cout << pos << endl;
    EarthCoord dwl(0.111646531, 0.921760253, 25);
    cout << TimeCoord::getUTCDiff() << endl;
    TimeCoord time(10.5);
    Assert (time.mjd() == 10.5);
    TimeCoord timeNow;
    cout << timeNow.mjd() << endl;
    cout << timeNow << endl;
    TimeCoord timeNow2 (timeNow.mjd(), 1./24);
    cout << timeNow2 << endl;
    TimeCoord timeNow3 (timeNow.mjd(), 1.);

    // Only make a connection if an argument is given.
    if (argc < 2) {
      cout << "Run as:  tCoordClient host [port]" << endl;
      return 0;
    }
    string port = "31337";
    if (argc > 2) {
      port = argv[2];
    }
    cout << argv[1] << ' ' << port << endl;
    CoordClient cc(argv[1], port);
    {
      SkyCoord result = cc.j2000ToAzel (sky, dwl, timeNow);
      cout << result << endl;
      result = cc.azelToJ2000 (result, dwl, timeNow);
      cout << result << endl;
    }
    {
      vector<SkyCoord> skies(4);
      vector<EarthCoord> poss(2);
      vector<TimeCoord> times(3);
      skies[0] = SkyCoord(1,0.3);
      skies[1] = SkyCoord(-1,-0.3);
      skies[2] = SkyCoord(2,0.1);
      skies[3] = SkyCoord(1.5,-0.3);
      poss[0] =  EarthCoord (0.1, 0.8, 1000);
      poss[1] = dwl;
      times[0] = timeNow;
      times[1] = timeNow2;
      times[2] = timeNow3;
      vector<SkyCoord> result = cc.j2000ToAzel (skies, poss, times);
      Assert (result.size() == 4*2*3);
      for (unsigned int i=0; i<4*2*3; i++) {
	cout << result[i] << endl;
      }
      int inx=0;
      for (unsigned int i=0; i<3; i++) {
	for (unsigned int j=0; j<2; j++) {
	  vector<SkyCoord> sk(4);
	  for (unsigned int k=0; k<4; k++) {
	    sk[k] = result[inx++];
	  }
	  vector<SkyCoord> res2 = cc.azelToJ2000 (sk, poss[j], times[i]);
	  for (unsigned int k=0; k<4; k++) {
	    cout << res2[k];
	  }
	  cout << endl;
	}
      }
    }

  } catch (std::exception& x) {
    cout << "Exception: " << x.what() << endl;
    return 1;
  }

  cout << "OK" << endl;
  return 0;
}
