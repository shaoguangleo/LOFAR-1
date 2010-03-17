/***************************************************************************
 *   Copyright (C) 2008 by A.R. Offringa   *
 *   offringa@astro.rug.nl   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <AOFlagger/imaging/model.h>

#include <AOFlagger/msio/image2d.h>
#include <AOFlagger/imaging/uvimager.h>
#include <AOFlagger/imaging/observatorium.h>

#include <iostream>


Model::Model()
{
}

Model::~Model()
{
}

void Model::SimulateObservation(UVImager &imager, Observatorium &observatorium, num_t delayDirectionDEC, num_t delayDirectionRA, num_t frequency)
{
	for(size_t i=0;i<observatorium.AntennaCount();++i)
	{
		for(size_t j=i+1;j<observatorium.AntennaCount();++j)
		{
			const AntennaInfo
				&antenna1 = observatorium.GetAntenna(i),
				&antenna2 = observatorium.GetAntenna(j);
			double
				dx = antenna1.position.x - antenna2.position.x,
				dy = antenna1.position.y - antenna2.position.y,
				dz = antenna1.position.z - antenna2.position.z;
			SimulateCorrelation(imager, delayDirectionDEC, delayDirectionRA, dx, dy, dz, frequency, 12*60*60, 10.0);
		} 
	}
}

void Model::SimulateCorrelation(UVImager &imager, num_t delayDirectionDEC, num_t delayDirectionRA, num_t dx, num_t dy, num_t dz, num_t frequency, double totalTime, double integrationTime)
{
	num_t wavelength = 1.0L / frequency;
	for(num_t t=0.0;t<totalTime;t+=integrationTime)
	{
		num_t u, v, r, i;
		GetUVPosition(u, v, t*(M_PI/12.0/60.0/60.0), delayDirectionDEC, delayDirectionRA, dx, dy, dz, wavelength);
		SimulateAntenna(delayDirectionDEC, delayDirectionRA, dx, dy, dz, frequency, t, r, i);
		imager.SetUVValue(u, v, r, i, 1.0);
		imager.SetUVValue(-u, -v, r, -i, 1.0);
		//imager.SetUVValue(u, v, 1.0, 1.0, 1.0);
		//imager.SetUVValue(-u, -v, 1.0, -1.0, 1.0);
	}
}

void Model::SimulateAntenna(num_t delayDirectionDEC, num_t delayDirectionRA, num_t dx, num_t dy, num_t dz, num_t frequency, num_t earthLattitude, num_t &r, num_t &i)
{
	num_t w = GetWPosition(delayDirectionDEC, delayDirectionRA, frequency, 0.0, earthLattitude, dx, dy);
	r = 0.0;
	i = 0.0;
	for(std::vector<PointSource *>::const_iterator iter=_sources.begin();iter!=_sources.end();++iter)
	{
		num_t fieldStrength = (*iter)->sqrtFluxIntensity;
		r += fieldStrength * cos(w * M_PI * 2.0);
		i += fieldStrength * sin(w * M_PI * 2.0);
	}
}

void Model::SimulateBaseline(long double delayDirectionDEC, long double delayDirectionRA, long double dx, long double dy, long double dz, long double frequencyStart, long double frequencyEnd, long double seconds, class Image2D &destR, class Image2D &destI)
{
	for(unsigned fIndex=0;fIndex<destR.Height();++fIndex)
	{
		long double frequency = (frequencyEnd - frequencyStart) * (long double) fIndex/destR.Height() + frequencyStart;

		for(unsigned tIndex=0;tIndex<destR.Width();++tIndex)
		{
			long double timeRotation =
				(long double) tIndex * M_PI * 2.0L * seconds /
				(24.0L*60.0L*60.0L * destR.Width());

			num_t u,v;
			GetUVPosition(u, v, timeRotation, delayDirectionDEC, delayDirectionRA, dx, dy, dz, 1.0L/frequency);
			num_t r, i;
			AddFTOfSources(u, v, r, i);
			destR.AddValue(tIndex, fIndex, r);
			destI.AddValue(tIndex, fIndex, i);
		}
	}
}

void Model::GetUVPosition(num_t &u, num_t &v, num_t earthLattitudeAngle, num_t delayDirectionDEC, num_t delayDirectionRA, num_t dx, num_t dy, num_t dz, num_t wavelength)
{
	long double pointingLattitude = delayDirectionRA;

	// Rotate baseline plane towards phase center, first rotate around z axis, then around x axis
	long double raRotation = earthLattitudeAngle - pointingLattitude + M_PI*0.5L;
	long double tmpCos = cosl(raRotation);
	long double tmpSin = sinl(raRotation);

	long double dxProjected = tmpCos*dx - tmpSin*dy;
	long double tmpdy = tmpSin*dx + tmpCos*dy;

	tmpCos = cosl(-delayDirectionDEC);
	tmpSin = sinl(-delayDirectionDEC);
	long double dyProjected = tmpCos*tmpdy - tmpSin*dz;

	// Now, the newly projected positive z axis of the baseline points to the phase center

	long double baselineLength = sqrtl(dxProjected*dxProjected + dyProjected*dyProjected);
	
	long double baselineAngle;
	if(baselineLength == 0.0)
		baselineAngle = 0.0;
	else {
		baselineLength /= 299792458.0L * wavelength;
		if(dxProjected > 0.0L)
			baselineAngle = atanl(dyProjected/dxProjected);
		else
			baselineAngle = M_PI - atanl(dyProjected/-dxProjected);
	}
		
	u = cosl(baselineAngle)*baselineLength;
	v = -sinl(baselineAngle)*baselineLength;
}

num_t Model::GetWPosition(num_t delayDirectionDec, num_t delayDirectionRA, num_t frequency, num_t earthLattitudeAngleStart, num_t earthLattitudeAngleEnd, num_t dx, num_t dy)
{
	num_t wavelength = 299792458.0L / frequency;
	num_t raSinStart = sinl(-delayDirectionRA - earthLattitudeAngleStart);
	num_t raCosStart = cosl(-delayDirectionRA - earthLattitudeAngleStart);
	num_t raSinEnd = sinl(-delayDirectionRA - earthLattitudeAngleEnd);
	num_t raCosEnd = cosl(-delayDirectionRA - earthLattitudeAngleEnd);
	num_t decCos = cosl(delayDirectionDec);
	// term "+ dz * decCos" is eliminated because of subtraction
	num_t wPosition =
		( (dx*raCosStart - dy*raSinStart)
		-
		(dx*raCosEnd - dy*raSinEnd) ) * (-decCos) / wavelength;
	return wPosition;
}

void Model::AddFTOfSources(num_t u, num_t v, num_t &rVal, num_t &iVal)
{
	rVal = 0.0;
	iVal = 0.0;

	for(std::vector<PointSource *>::const_iterator i=_sources.begin();i!=_sources.end();++i)
	{
		AddFTOfSource(u, v, rVal, iVal, (*i));
	}
}

void Model::AddFTOfSource(num_t u, num_t v, num_t &r, num_t &i, const PointSource *source)
{
	// Calculate F(X) = f(x) e ^ {i 2 pi (x1 u + x2 v) } 
	long double fftRotation = (u * source->dec/180.0L + v * source->ra/180.0L) * -2.0L * M_PI;
	r += cosl(fftRotation) * source->fluxIntensity;
	i += sinl(fftRotation) * source->fluxIntensity;
}
