//# UVW.cc: Calculate station UVW from station position and phase center
//#
//# Copyright (C) 2003
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

#include <MEQ/UVW.h>
#include <MEQ/Request.h>
#include <MEQ/VellSet.h>
#include <MEQ/Cells.h>
#include <aips/Measures/MBaseline.h>
#include <aips/Measures/MPosition.h>
#include <aips/Measures/MEpoch.h>
#include <aips/Measures/MeasConvert.h>
#include <aips/Quanta/MVuvw.h>

namespace Meq {


UVW::UVW()
{
  ///  itsRefU = itsU;
}

UVW::~UVW()
{}

int UVW::getResult (Result::Ref& resref, const Request& request, bool)
{
  const Cells& cells = request.cells();
  // Get RA and DEC of phase center.
  Result::Ref ra, dec;
  int flag = children()[0]->execute (ra, request);
  flag |= children()[1]->execute (dec, request);
  ra.persist();
  dec.persist();
  // Get station positions.
  Result::Ref stx, sty, stz;
  flag |= children()[2]->execute (stx, request);
  flag |= children()[3]->execute (sty, request);
  flag |= children()[4]->execute (stz, request);
  stx.persist();
  sty.persist();
  stz.persist();
  if (flag & Node::RES_WAIT) {
    return flag;
  }
  // For the time being we only support scalars.
  const Vells& vra  = ra().vellSet(0).getValue();
  const Vells& vdec = dec().vellSet(0).getValue();
  const Vells& vstx = stx().vellSet(0).getValue();
  const Vells& vsty = sty().vellSet(0).getValue();
  const Vells& vstz = stz().vellSet(0).getValue();
  Assert (vra.nelements()==1 && vdec.nelements()==1
	  && vstx.nelements()==1 && vsty.nelements()==1
	  && vstz.nelements()==1);
  // Allocate a 3-plane result
  Result &res_set = resref <<= new Result(3);
  // Get RA and DEC of phase center.
  MVDirection phaseRef(vra.as<double>(), vdec.as<double>());
  // Set correct size of values.
  int nfreq = cells.nfreq();
  int ntime = cells.ntime();
  LoMat_double& matU = res_set.setNewVellSet(0).setReal(nfreq,ntime);
  LoMat_double& matV = res_set.setNewVellSet(1).setReal(nfreq,ntime);
  LoMat_double& matW = res_set.setNewVellSet(2).setReal(nfreq,ntime);
  // Calculate the UVW coordinates using the AIPS++ code.
  MVPosition mvpos(vstx.as<double>(),vsty.as<double>(),vstz.as<double>());
  MVBaseline mvbl(mvpos);
  MBaseline mbl(mvbl, MBaseline::ITRF);
  Quantum<double> qepoch(0, "s");
  qepoch.setValue (cells.time(0));
  MEpoch mepoch(qepoch, MEpoch::UTC);
  MeasFrame frame;
  frame.set (mepoch);
  mbl.getRefPtr()->set(frame);      // attach frame
  MBaseline::Convert mcvt(mbl, MBaseline::J2000);
  for (Int i=0; i<cells.ntime(); i++) {
    qepoch.setValue (cells.time(i));
    mepoch.set (qepoch);
    frame.set (mepoch);
    const MVBaseline& bas2000 = mcvt().getValue();
    MVuvw uvw2000 (bas2000, phaseRef);
    const Vector<double>& xyz = uvw2000.getValue();
    for (int j=0; j<nfreq; j++) 
    {
      matU(j,i) = xyz(0);
      matV(j,i) = xyz(1);
      matW(j,i) = xyz(2);
    }
  }
  return 0;
}

void UVW::checkChildren()
{
  Function::convertChildren (5);
}

} // namespace Meq
