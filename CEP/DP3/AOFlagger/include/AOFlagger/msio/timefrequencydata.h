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

#ifndef TIMEFREQUENCYDATA_H
#define TIMEFREQUENCYDATA_H

#include <string>
#include <vector>
#include <utility>
#include <sstream>

#include "image2d.h"
#include "mask2d.h"
#include "types.h"

#include "../baseexception.h"

class TimeFrequencyData
{
	public:
		enum PhaseRepresentation { PhasePart, AmplitudePart, RealPart, ImaginaryPart, ComplexRepresentation };

		enum FlagCoverage { NoFlagCoverage, GlobalFlagCoverage, IndividualPolarisationFlagCoverage };
		
		TimeFrequencyData() : _containsData(false), _flagCoverage(NoFlagCoverage) { }

		TimeFrequencyData(const TimeFrequencyData &source) :
			_containsData(source._containsData),
			_phaseRepresentation(source._phaseRepresentation),
			_polarisationType(source._polarisationType),
			_flagCoverage(source._flagCoverage),
			_images(source._images),
			_flagging(source._flagging)
		{
		}
		
		TimeFrequencyData(PhaseRepresentation phaseRepresentation,
												 PolarisationType polarisationType,
												 Image2DCPtr image) :
				_containsData(true),
				_phaseRepresentation(phaseRepresentation),
				_polarisationType(polarisationType),
				_flagCoverage(NoFlagCoverage)
		{
			if(phaseRepresentation == ComplexRepresentation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create complex representation from single image");
			if(polarisationType == DipolePolarisation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create dipole polarised data from single image");
			_images.push_back(image);
		}

		TimeFrequencyData(PhaseRepresentation phaseRepresentation,
												 PolarisationType polarisationType,
												 Image2DCPtr imageA,
												 Image2DCPtr imageB) :
				_containsData(true),
				_phaseRepresentation(phaseRepresentation),
				_polarisationType(polarisationType),
				_flagCoverage(NoFlagCoverage)
		{
			if(polarisationType != AutoDipolePolarisation && polarisationType != CrossDipolePolarisation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create auto dipole polarised data from single image");
			if(phaseRepresentation == ComplexRepresentation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create complex representation from single image");
			_images.push_back(imageA);
			_images.push_back(imageB);
		}

		TimeFrequencyData(PolarisationType polarisationType,
											Image2DCPtr real,
											Image2DCPtr imaginary) :
				_containsData(true),
				_phaseRepresentation(ComplexRepresentation),
				_polarisationType(polarisationType),
				_flagCoverage(NoFlagCoverage)
		{
			if(polarisationType == DipolePolarisation || polarisationType == AutoDipolePolarisation || polarisationType == CrossDipolePolarisation)
				throw BadUsageException("Wrong constructor called");
			_images.push_back(real);
			_images.push_back(imaginary);
		}
		
		TimeFrequencyData(PolarisationType polarisationType,
											Image2DCPtr realA,
											Image2DCPtr imaginaryA,
											Image2DCPtr realB,
											Image2DCPtr imaginaryB) :
				_containsData(true),
				_phaseRepresentation(ComplexRepresentation),
				_polarisationType(polarisationType),
				_flagCoverage(NoFlagCoverage)
		{
			if(polarisationType != AutoDipolePolarisation && polarisationType != CrossDipolePolarisation)
				throw BadUsageException("Incorrect construction of time/frequency data: trying to create non-auto/cross dipole polarised data from two images");
			_images.push_back(realA);
			_images.push_back(imaginaryA);
			_images.push_back(realB);
			_images.push_back(imaginaryB);
		}
		
		TimeFrequencyData(PhaseRepresentation phaseRepresentation,
											Image2DCPtr xx,
											Image2DCPtr xy,
											Image2DCPtr yx,
											Image2DCPtr yy) :
				_containsData(true),
				_phaseRepresentation(phaseRepresentation),
				_polarisationType(DipolePolarisation),
				_flagCoverage(NoFlagCoverage)
		{
			if(phaseRepresentation == ComplexRepresentation) throw;
			_images.push_back(xx);
			_images.push_back(xy);
			_images.push_back(yx);
			_images.push_back(yy);
		}

		TimeFrequencyData(Image2DCPtr xxReal,
											Image2DCPtr xxImag,
											Image2DCPtr xyReal,
											Image2DCPtr xyImag,
											Image2DCPtr yxReal,
											Image2DCPtr yxImag,
											Image2DCPtr yyReal,
											Image2DCPtr yyImag) :
				_containsData(true),
				_phaseRepresentation(ComplexRepresentation),
				_polarisationType(DipolePolarisation),
				_flagCoverage(NoFlagCoverage)
		{
			_images.push_back(xxReal);
			_images.push_back(xxImag);
			_images.push_back(xyReal);
			_images.push_back(xyImag);
			_images.push_back(yxReal);
			_images.push_back(yxImag);
			_images.push_back(yyReal);
			_images.push_back(yyImag);
		}

		TimeFrequencyData &operator=(const TimeFrequencyData &source)
		{
			_containsData = source._containsData;
			
			_phaseRepresentation = source._phaseRepresentation;
			_polarisationType = source._polarisationType;
			_flagCoverage = source._flagCoverage;
	
			_images = source._images;
			_flagging = source._flagging;
			return *this;
		}
		
		bool IsEmpty() const { return !_containsData; }

		bool HasXX() const
		{
			return _polarisationType == DipolePolarisation || _polarisationType == AutoDipolePolarisation || _polarisationType == XXPolarisation;
		}

		bool HasXY() const
		{
			return _polarisationType == DipolePolarisation || _polarisationType == CrossDipolePolarisation || _polarisationType == XYPolarisation;
		}

		bool HasYX() const
		{
			return _polarisationType == DipolePolarisation || _polarisationType == CrossDipolePolarisation || _polarisationType == YXPolarisation;
		}

		bool HasYY() const
		{
			return _polarisationType == DipolePolarisation || _polarisationType == AutoDipolePolarisation || _polarisationType == YYPolarisation;
		}
		
		/**
		 * This function returns a new Image2D that contains
		 * an image that can be used best for thresholding-like
		 * RFI methods, or visualization. The encapsulated data
		 * may be converted in order to do so.
		 * @return A new image containing the TF-data.
		 */
		Image2DCPtr GetSingleImage() const
		{
			switch(_phaseRepresentation)
			{
				case PhasePart:
				case AmplitudePart:
				case RealPart:
				case ImaginaryPart:
					return GetSingleImageFromSinglePhaseImage();
				case ComplexRepresentation:
					return GetSingleAbsoluteFromComplex();
			}
			throw BadUsageException("Incorrect phase representation");
		}

		Mask2DCPtr GetSingleMask() const
		{
			switch(_flagCoverage)
			{
				case NoFlagCoverage:
					return GetSetMask<false>();
				case GlobalFlagCoverage:
					return _flagging[0];
				case IndividualPolarisationFlagCoverage:
					return GetCombinedMask();
			}
			throw BadUsageException("Incorrect flag coverage");
		}

		std::pair<Image2DCPtr,Image2DCPtr> GetSingleComplexImage() const
		{
			if(_phaseRepresentation != ComplexRepresentation)
				throw BadUsageException("Trying to create single complex image, but no complex data available");
			switch(_polarisationType)
			{
				case SinglePolarisation:
				case StokesIPolarisation:
				case StokesQPolarisation:
				case StokesUPolarisation:
				case StokesVPolarisation:
				case XXPolarisation:
				case XYPolarisation:
				case YXPolarisation:
				case YYPolarisation:
					return std::pair<Image2DCPtr,Image2DCPtr>(_images[0], _images[1]);
				case DipolePolarisation:
				case AutoDipolePolarisation:
				case CrossDipolePolarisation:
					throw BadUsageException("Not implemented");
					//return CreateSingleComplexImageFromDipole();
			}
		}
		
		void Set(PolarisationType polarisationType,
								Image2DCPtr real,
								Image2DCPtr imaginary)
		{
			_containsData = true;
			_phaseRepresentation = ComplexRepresentation;
			_polarisationType = polarisationType;
			if(polarisationType == DipolePolarisation) throw;
			_images.clear();
			_images.push_back(real);
			_images.push_back(imaginary);
		}

		void SetNoMask()
		{
			_flagging.clear();
			_flagCoverage = NoFlagCoverage;
		}

		void SetGlobalMask(Mask2DCPtr mask)
		{
			_flagging.clear();
			_flagging.push_back(mask);
			_flagCoverage = GlobalFlagCoverage;
		}

		Mask2DCPtr GetMask(enum PolarisationType polarisation) const
		{
			if(polarisation == DipolePolarisation || polarisation == AutoDipolePolarisation)
				throw BadUsageException("Need a single polarisation for GetMask()");
			else if(_flagCoverage == NoFlagCoverage)
				return GetSetMask<false>();
			else if(polarisation == _polarisationType)
				return _flagging[0];
			else if(_polarisationType==DipolePolarisation) {
				if(_flagCoverage == GlobalFlagCoverage) return _flagging[0];
				switch(polarisation)
				{
					case XXPolarisation: return _flagging[0];
					case XYPolarisation: return _flagging[1];
					case YXPolarisation: return _flagging[2];
					case YYPolarisation: return _flagging[3];
					case StokesIPolarisation: return GetSingleMask();
					case StokesQPolarisation: return GetSingleMask();
					case StokesUPolarisation: return GetSingleMask();
					case StokesVPolarisation: return GetSingleMask();
					case SinglePolarisation: throw BadUsageException("Requesting single polarisation mask from dipole information, which polarisation to return?");
					case DipolePolarisation:
					case AutoDipolePolarisation:
					case CrossDipolePolarisation:
						break;
				}
			}
			else if(_polarisationType==AutoDipolePolarisation) {
				if(_flagCoverage == GlobalFlagCoverage) return _flagging[0];
				if(polarisation == XXPolarisation)
					return _flagging[0];
				else if(polarisation == YYPolarisation)
					return _flagging[1];
				else if(polarisation == StokesIPolarisation)
					return GetSingleMask();
				else if(polarisation == SinglePolarisation)
					throw BadUsageException("Requesting single polarisation mask from auto dipole information, which polarisation to return?");
			}
			else if(_polarisationType==CrossDipolePolarisation) {
				if(_flagCoverage == GlobalFlagCoverage) return _flagging[0];
				if(polarisation == XYPolarisation)
					return _flagging[0];
				else if(polarisation == YXPolarisation)
					return _flagging[1];
				else if(polarisation == StokesIPolarisation)
					return GetSingleMask();
				else if(polarisation == SinglePolarisation)
					throw BadUsageException("Requesting single polarisation mask from cross dipole information, which polarisation to return?");
			}
			else if((_flagCoverage==GlobalFlagCoverage || _flagCoverage==IndividualPolarisationFlagCoverage) && ( _polarisationType == SinglePolarisation || _polarisationType == StokesIPolarisation)) {
				if(polarisation == XXPolarisation || polarisation == XYPolarisation || polarisation == YXPolarisation || polarisation == YYPolarisation || polarisation == StokesIPolarisation || polarisation == SinglePolarisation)
				return _flagging[0];
			}
			throw BadUsageException("Mask requested that was not available in the given time frequency data");
		}

		void SetIndividualPolarisationMasks(Mask2DCPtr xxMask, Mask2DCPtr yyMask)
		{
			if(_polarisationType != AutoDipolePolarisation)
				throw BadUsageException("Trying to set two individual mask in non-dipole time frequency data");
			_flagging.clear();
			_flagging.push_back(xxMask);
			_flagging.push_back(yyMask);
			_flagCoverage = IndividualPolarisationFlagCoverage;
		}

		void SetIndividualPolarisationMasks(Mask2DCPtr xxMask, Mask2DCPtr xyMask, Mask2DCPtr yxMask, Mask2DCPtr yyMask)
		{
			if(_polarisationType != DipolePolarisation)
				throw BadUsageException("Trying to set four individual mask in non-dipole time frequency data");
			_flagging.clear();
			_flagging.push_back(xxMask);
			_flagging.push_back(xyMask);
			_flagging.push_back(yxMask);
			_flagging.push_back(yyMask);
			_flagCoverage = IndividualPolarisationFlagCoverage;
		}

		void SetMaskFrom(const TimeFrequencyData &data)
		{
			data.CopyFlaggingTo(this);
		}

		TimeFrequencyData *CreateTFData(PhaseRepresentation phase) const
		{
			if(phase == _phaseRepresentation)
				return new TimeFrequencyData(*this);
			else if(_phaseRepresentation == ComplexRepresentation)
			{
				if(_polarisationType == DipolePolarisation)
				{
					return CreateTFDataFromDipoleComplex(phase);
				} else if(_polarisationType == AutoDipolePolarisation) {
					return CreateTFDataFromAutoDipoleComplex(phase);
				} else {
					return CreateTFDataFromSingleComplex(phase);
				}
			} else throw BadUsageException("Request for time/frequency data with a phase representation that can not be extracted from the source (source is not complex)");
		}

		TimeFrequencyData *CreateTFData(PolarisationType polarisation) const
		{
			TimeFrequencyData *data = 0;
			if(_polarisationType == polarisation)
				data = new TimeFrequencyData(*this);
			else if(_polarisationType == DipolePolarisation && _phaseRepresentation == ComplexRepresentation) {
				switch(polarisation)
				{
					case XXPolarisation:
						data = new TimeFrequencyData(XXPolarisation, _images[0], _images[1]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case XYPolarisation:
						data = new TimeFrequencyData(XYPolarisation, _images[2], _images[3]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case YXPolarisation:
						data = new TimeFrequencyData(YXPolarisation, _images[4], _images[5]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case YYPolarisation:
						data = new TimeFrequencyData(YYPolarisation, _images[6], _images[7]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesIPolarisation:
						data= new TimeFrequencyData(StokesIPolarisation, GetStokesIFromDipole(0, 6), GetStokesIFromDipole(1, 7));
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesQPolarisation:
						data= new TimeFrequencyData(StokesQPolarisation, GetStokesQFromDipole(0, 6), GetStokesQFromDipole(1, 7));
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesUPolarisation:
						data= new TimeFrequencyData(StokesUPolarisation, GetStokesUFromDipole(2, 4), GetStokesUFromDipole(3, 5));
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesVPolarisation:
						data= new TimeFrequencyData(StokesVPolarisation, GetRealStokesVFromDipole(3, 4), GetImaginaryStokesVFromDipole(2, 5));
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case AutoDipolePolarisation:
						data = new TimeFrequencyData(AutoDipolePolarisation, _images[0], _images[1], _images[6], _images[7]);
						if(_flagCoverage == GlobalFlagCoverage)
							data->SetGlobalMask(GetMask(polarisation));
						else if(_flagCoverage == IndividualPolarisationFlagCoverage)
							data->SetIndividualPolarisationMasks(_flagging[0], _flagging[3]);
						break;
					case CrossDipolePolarisation:
						data = new TimeFrequencyData(CrossDipolePolarisation, _images[2], _images[3], _images[4], _images[5]);
						break;
					case SinglePolarisation:
						throw BadUsageException("Single polarisation requested from dipole polarisation time frequency data: which single polarisation do you want?");
					case DipolePolarisation:
						break;
				}
			} else if(_polarisationType == DipolePolarisation) {
				switch(polarisation)
				{
					case XXPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, XXPolarisation, _images[0]);
						break;
					case XYPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, XYPolarisation, _images[1]);
						break;
					case YXPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, YXPolarisation, _images[2]);
						break;
					case YYPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, YYPolarisation, _images[3]);
						break;
					case StokesIPolarisation:
					case StokesQPolarisation:
					case StokesUPolarisation:
					case StokesVPolarisation:
					case AutoDipolePolarisation:
					case CrossDipolePolarisation:
					case DipolePolarisation:
					case SinglePolarisation:
						throw BadUsageException("Requested polarisation type not available in time frequency data");
				}
				data->SetGlobalMask(GetMask(polarisation));
			} else if(_polarisationType == AutoDipolePolarisation && _phaseRepresentation == ComplexRepresentation) {
				switch(polarisation)
				{
					case XXPolarisation:
						data = new TimeFrequencyData(XXPolarisation, _images[0], _images[1]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case YYPolarisation:
						data = new TimeFrequencyData(YYPolarisation, _images[2], _images[3]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesIPolarisation:
						data= new TimeFrequencyData(StokesIPolarisation, GetStokesIFromDipole(0, 2), GetStokesIFromDipole(1, 3));
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesQPolarisation:
					case StokesUPolarisation:
					case StokesVPolarisation:
					case XYPolarisation:
					case YXPolarisation:
					case AutoDipolePolarisation:
					case DipolePolarisation:
					case CrossDipolePolarisation:
						throw BadUsageException("Trying to create the time frequency data from auto dipole without the requested polarisation(s)"); 
					case SinglePolarisation:
						throw BadUsageException("Single polarisation requested from auto dipole polarisation time frequency data: which single polarisation do you want?");
				}
			} else if(_polarisationType == AutoDipolePolarisation) {
				switch(polarisation)
				{
					case XXPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, XXPolarisation, _images[0]);
						break;
					case YYPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, YYPolarisation, _images[1]);
						break;
					case XYPolarisation:
					case YXPolarisation:
					case StokesIPolarisation:
					case StokesQPolarisation:
					case StokesUPolarisation:
					case StokesVPolarisation:
					case AutoDipolePolarisation:
					case CrossDipolePolarisation:
					case DipolePolarisation:
					case SinglePolarisation:
						throw BadUsageException("Requested polarisation type not available in time frequency data");
				}
				data->SetGlobalMask(GetMask(polarisation));
			} else if(_polarisationType == CrossDipolePolarisation && _phaseRepresentation == ComplexRepresentation) {
				switch(polarisation)
				{
					case XYPolarisation:
						data = new TimeFrequencyData(XYPolarisation, _images[0], _images[1]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case YXPolarisation:
						data = new TimeFrequencyData(YXPolarisation, _images[2], _images[3]);
						data->SetGlobalMask(GetMask(polarisation));
						break;
					case StokesIPolarisation:
					case StokesQPolarisation:
					case StokesUPolarisation:
					case StokesVPolarisation:
					case XXPolarisation:
					case YYPolarisation:
					case AutoDipolePolarisation:
					case DipolePolarisation:
					case CrossDipolePolarisation:
						throw BadUsageException("Trying to create the time frequency data from auto dipole without the requested polarisation(s)"); 
					case SinglePolarisation:
						throw BadUsageException("Single polarisation requested from auto dipole polarisation time frequency data: which single polarisation do you want?");
				}
			} else if(_polarisationType == CrossDipolePolarisation) {
				switch(polarisation)
				{
					case XYPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, XYPolarisation, _images[0]);
						break;
					case YXPolarisation:
						data = new TimeFrequencyData(_phaseRepresentation, YXPolarisation, _images[1]);
						break;
					case XXPolarisation:
					case YYPolarisation:
					case StokesIPolarisation:
					case StokesQPolarisation:
					case StokesUPolarisation:
					case StokesVPolarisation:
					case AutoDipolePolarisation:
					case DipolePolarisation:
					case SinglePolarisation:
					case CrossDipolePolarisation:
						throw BadUsageException("Requested polarisation type not available in time frequency data");
				}
				data->SetGlobalMask(GetMask(polarisation));
			} else if(polarisation == DipolePolarisation) {
				throw BadUsageException("Requesting time frequency data with dipole polarisation, but not all polarisations are available");
			} else {
				throw BadUsageException("Trying to convert the polarization in time frequency data in an invalid way");
			}
			return data;
		}

		Image2DCPtr GetRealPart() const
		{
			if(_polarisationType == DipolePolarisation || _polarisationType == AutoDipolePolarisation)
			{
				throw BadUsageException("This image contains >1 polarizations; which real part should I return?");
			} else if(_phaseRepresentation == ComplexRepresentation || _phaseRepresentation == RealPart) {
				return _images[0];
			} else {
				throw BadUsageException("Trying to retrieve real part from time frequency data in which phase doesn't have a complex or real representation");
			}
		}

		Image2DCPtr GetImaginaryPart() const
		{
			if(_polarisationType == DipolePolarisation)
			{
				throw BadUsageException("This image contains 4 polarizations; which imaginary part should I return?");
			} else if(_phaseRepresentation == ComplexRepresentation) {
				return _images[1];
			} else if(_phaseRepresentation == ImaginaryPart) {
				return _images[0];
			} else {
				throw BadUsageException("Trying to retrieve imaginary part from time frequency data in which phase doesn't have a complex or real representation");
			}
		}

		size_t ImageWidth() const
		{
			return _images[0]->Width();
		}
		size_t ImageHeight() const
		{
			return _images[0]->Height();
		}

		enum PhaseRepresentation PhaseRepresentation() const
		{
			return _phaseRepresentation;
		}

		enum PolarisationType Polarisation() const
		{
			return _polarisationType;
		}

		static TimeFrequencyData *CreateTFDataFromDiff(const TimeFrequencyData &lhs, const TimeFrequencyData &rhs)
		{
			if(lhs._images.size() != rhs._images.size())
			{
				std::stringstream s;
				s << "Can not subtract time-frequency data: they do not have the same number of polarisations or phase representation! ("
					<< lhs._images.size() << " vs. " << rhs._images.size() << ")";
				throw BadUsageException(s.str());
			}
			TimeFrequencyData *data = new TimeFrequencyData(lhs);
			for(size_t i=0;i<lhs._images.size();++i)
				data->_images[i] = Image2D::CreateFromDiff(lhs._images[i], rhs._images[i]);
			return data;
		}

		static TimeFrequencyData *CreateTFDataFromSum(const TimeFrequencyData &lhs, const TimeFrequencyData &rhs)
		{
			if(lhs._images.size() != rhs._images.size())
			{
				std::stringstream s;
				s << "Can not add time-frequency data: they do not have the same number of polarisations or phase representation! ("
					<< lhs._images.size() << " vs. " << rhs._images.size() << ")";
				throw BadUsageException(s.str());
			}
			TimeFrequencyData *data = new TimeFrequencyData(lhs);
			for(size_t i=0;i<lhs._images.size();++i)
				data->_images[i] = Image2D::CreateFromSum(lhs._images[i], rhs._images[i]);
			return data;
		}

		size_t ImageCount() const { return _images.size(); }
		size_t MaskCount() const { return _flagging.size(); }

		Image2DCPtr GetImage(size_t imageIndex) const
		{
			return _images[imageIndex];
		}
		Mask2DCPtr GetMask(size_t maskIndex) const
		{
			return _flagging[maskIndex];
		}
		void SetImage(size_t imageIndex, Image2DCPtr image)
		{
			_images[imageIndex] = image;
		}
		void SetMask(size_t maskIndex, Mask2DCPtr mask)
		{
			_flagging[maskIndex] = mask;
		}
		void SetMask(const TimeFrequencyData &source)
		{
			source.CopyFlaggingTo(this);
		}

		static TimeFrequencyData *CreateTFDataFromComplexCombination(const TimeFrequencyData &real, const TimeFrequencyData &imaginary);

		static TimeFrequencyData *CreateTFDataFromDipoleCombination(const TimeFrequencyData &xx, const TimeFrequencyData &xy, const TimeFrequencyData &yx, const TimeFrequencyData &yy);

		static TimeFrequencyData *CreateTFDataFromAutoDipoleCombination(const TimeFrequencyData &xx, const TimeFrequencyData &yy);

		static TimeFrequencyData *CreateTFDataFromCrossDipoleCombination(const TimeFrequencyData &xx, const TimeFrequencyData &yy);

		void SetImagesToZero();
		void MultiplyImages(long double factor);
		void JoinMask(const TimeFrequencyData &other);

		void Trim(unsigned timeStart, unsigned freqStart, unsigned timeEnd, unsigned freqEnd)
		{
			for(std::vector<Image2DCPtr>::iterator i=_images.begin();i!=_images.end();++i)
				*i = (*i)->Trim(timeStart, freqStart, timeEnd, freqEnd);
		}
		std::string Description() const
		{
			if(_phaseRepresentation == ComplexRepresentation)
			{
				switch(_polarisationType)
				{
					case StokesIPolarisation: return "Stokes I";
					case StokesQPolarisation: return "Stokes Q";
					case StokesUPolarisation: return "Stokes U";
					case StokesVPolarisation: return "Stokes V";
					case DipolePolarisation: return "All polarizations"; 
					case AutoDipolePolarisation: return "Combination of XX and YY";
					case CrossDipolePolarisation: return "Combination of XY and YX";
					case XXPolarisation: return "XX";
					case XYPolarisation: return "XY";
					case YXPolarisation: return "YX";
					case YYPolarisation: return "YY";
					case SinglePolarisation: return "Single polarization";
				}
				return "";
			} else {
				std::stringstream s;
				switch(_phaseRepresentation)
				{
					case RealPart: s << "Real component of "; break;
					case ImaginaryPart: s << "Imaginary component of "; break;
					case PhasePart: s << "Phase of "; break;
					case AmplitudePart: s << "Amplitude of "; break;
					case ComplexRepresentation:
						break;
				}
				switch(_polarisationType)
				{
					case StokesIPolarisation: s << "stokes I"; break;
					case StokesQPolarisation: s << "stokes Q"; break;
					case StokesUPolarisation: s << "stokes U"; break;
					case StokesVPolarisation: s << "stokes V"; break;
					case DipolePolarisation: s << "all polarizations"; break;
					case AutoDipolePolarisation: s << "combination of XX and YY"; break;
					case CrossDipolePolarisation: s << "combination of XY and YX"; break;
					case XXPolarisation: s << "XX"; break;
					case XYPolarisation: s << "XY"; break;
					case YXPolarisation: s << "YX"; break;
					case YYPolarisation: s << "YY"; break;
					case SinglePolarisation: s << "single polarization"; break;
				}
				return s.str();
			}
		}
		size_t PolarisationCount() const
		{
			return PolarisationCount(_polarisationType);
		}
		static size_t PolarisationCount(enum PolarisationType type)
		{
			switch(type)
			{
				case SinglePolarisation:
				case XXPolarisation:
				case XYPolarisation:
				case YXPolarisation:
				case YYPolarisation:
				case StokesIPolarisation:
				case StokesQPolarisation:
				case StokesUPolarisation:
				case StokesVPolarisation:
					return 1;
				case AutoDipolePolarisation:
				case CrossDipolePolarisation:
					return 2;
				case DipolePolarisation:
					return 4;
			}
			throw BadUsageException("Incorrect polarization");
		}
		TimeFrequencyData *CreateTFDataFromPolarisationIndex(size_t index) const
		{
			switch(_polarisationType)
			{
				case SinglePolarisation:
				case XXPolarisation:
				case XYPolarisation:
				case YXPolarisation:
				case YYPolarisation:
				case StokesIPolarisation:
				case StokesQPolarisation:
				case StokesUPolarisation:
				case StokesVPolarisation:
					if(index == 0)
						return new TimeFrequencyData(*this);
					else
						throw BadUsageException("Index out of bounds in CreateTFDataFromPolarisationIndex(..), one polarisation");
				case AutoDipolePolarisation:
					if(index == 0)
						return CreateTFData(XXPolarisation);
					else if(index == 1)
						return CreateTFData(YYPolarisation);
					else
						throw BadUsageException("Index out of bounds in CreateTFDataFromPolarisationIndex(..), two polarisations");
				case CrossDipolePolarisation:
					if(index == 0)
						return CreateTFData(XYPolarisation);
					else if(index == 1)
						return CreateTFData(YXPolarisation);
					else
						throw BadUsageException("Index out of bounds in CreateTFDataFromPolarisationIndex(..), two polarisations");
				case DipolePolarisation:
					switch(index)
					{
						case 0: return CreateTFData(XXPolarisation);
						case 1: return CreateTFData(XYPolarisation);
						case 2: return CreateTFData(YXPolarisation);
						case 3: return CreateTFData(YYPolarisation);
						default: throw BadUsageException("Index out of bounds in CreateTFDataFromPolarisationIndex(..), four polarisations");
					}
			}
			throw BadUsageException("Incorrect polarization");
		}
		void SetPolarizationData(size_t polarizationIndex, const TimeFrequencyData &data)
		{
			if(data.PolarisationCount() != 1)
				throw BadUsageException("Trying to set multiple polarizations by single polarization index");
			else if(data.PhaseRepresentation() != PhaseRepresentation())
				throw BadUsageException("Trying to combine TFData's with different phase representations");
			else {
				if(data.PhaseRepresentation() == ComplexRepresentation)
				{
					_images[polarizationIndex*2] = data._images[0];
					_images[polarizationIndex*2+1] = data._images[1];
				} else {
					_images[polarizationIndex] = data._images[0];
				}
				if(data._flagCoverage != NoFlagCoverage)
				{
					if(data._flagging.size() != 1)
						throw BadUsageException("Input has several flag masks");

					switch(_flagCoverage)
					{
						case NoFlagCoverage: {
							Mask2DCPtr emptyMask = Mask2D::CreateSetMaskPtr<false>(ImageWidth(), ImageHeight());
							for(size_t i=0;i<PolarisationCount();++i)
								_flagging.push_back(emptyMask);
							_flagCoverage = IndividualPolarisationFlagCoverage;
							} break;
						case GlobalFlagCoverage: {
							for(size_t i=0;i<PolarisationCount()-1;++i)
								_flagging.push_back(_flagging[0]);
							_flagCoverage = IndividualPolarisationFlagCoverage;
							} break;
						case IndividualPolarisationFlagCoverage:
							break;
					}

					_flagging[polarizationIndex] = data._flagging[0];
				}
			}
		}

		void SetImageSize(size_t width, size_t height)
		{
			for(size_t i=0;i<_images.size();++i)
			{
				_images[i] = Image2D::CreateEmptyImagePtr(width, height);
			}
		}

		void CopyImagesFrom(const TimeFrequencyData &source, size_t destX, size_t destY)
		{
			if(source._images.size() != _images.size())
				throw BadUsageException("CopyImagesFrom: tf data's do not match");
			for(size_t i=0;i<_images.size();++i)
			{
				Image2DPtr image = Image2D::CreateCopy(_images[i]);
				image->CopyFrom(source._images[i], destX, destY);
				_images[i] = image;
			}
		}
	private:
		Image2DCPtr GetSingleAbsoluteFromComplex() const
		{
			switch(_polarisationType)
			{
				case DipolePolarisation:
					return GetSingleAbsoluteFromComplexDipole();
				case AutoDipolePolarisation:
					return GetSingleAbsoluteFromComplexAutoDipole();
				case CrossDipolePolarisation:
					return GetSingleAbsoluteFromComplexCrossDipole();
				case SinglePolarisation:
				case StokesIPolarisation:
				case StokesQPolarisation:
				case StokesUPolarisation:
				case StokesVPolarisation:
				case XXPolarisation:
				case XYPolarisation:
				case YXPolarisation:
				case YYPolarisation:
					return GetAbsoluteFromComplex(0, 1);
			}
			throw BadUsageException("Incorrect polarisation type");
		}

		Image2DCPtr GetSingleImageFromSinglePhaseImage() const
		{
			switch(_polarisationType)
			{
				case DipolePolarisation:
					if(_phaseRepresentation == PhasePart)
						return GetSinglePhaseFromDipolePhase(0, 3);
					else
						return GetStokesIFromDipole(0, 3);
				case AutoDipolePolarisation:
					if(_phaseRepresentation == PhasePart)
						return GetSinglePhaseFromDipolePhase(0, 1);
					else
						return GetStokesIFromDipole(0, 1);
				case CrossDipolePolarisation:
					if(_phaseRepresentation == PhasePart)
						return GetSinglePhaseFromDipolePhase(0, 1);
					else
						return GetStokesIFromDipole(0, 1);
				case SinglePolarisation:
				case StokesIPolarisation:
				case StokesQPolarisation:
				case StokesUPolarisation:
				case StokesVPolarisation:
				case XXPolarisation:
				case XYPolarisation:
				case YXPolarisation:
				case YYPolarisation:
					return _images[0];
			}
			throw BadUsageException("Incorrect polarisation type");
		}

		Image2DCPtr GetAbsoluteFromComplex(size_t realImage, size_t imagImage) const
		{
			return GetAbsoluteFromComplex(_images[realImage], _images[imagImage]);
		}

		Image2DCPtr GetRealPartFromDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return _images[0];
				case XYPolarisation: return _images[2];
				case YXPolarisation: return _images[4];
				case YYPolarisation: return _images[6];
				default: throw BadUsageException("Could not extract real part for given polarisation");
			}
		}

		Image2DCPtr GetRealPartFromAutoDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return _images[0];
				case YYPolarisation: return _images[2];
				default: throw BadUsageException("Could not extract real part for given polarisation");
			}
		}

		Image2DCPtr GetImaginaryPartFromDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return _images[1];
				case XYPolarisation: return _images[3];
				case YXPolarisation: return _images[5];
				case YYPolarisation: return _images[7];
				default: throw BadUsageException("Could not extract imaginary part for given polarisation");
			}
		}

		Image2DCPtr GetImaginaryPartFromAutoDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return _images[1];
				case YYPolarisation: return _images[3];
				default: throw BadUsageException("Could not extract imaginary part for given polarisation");
			}
		}

		Image2DCPtr GetAmplitudePartFromDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return GetAbsoluteFromComplex(0, 1);
				case XYPolarisation: return GetAbsoluteFromComplex(2, 3);
				case YXPolarisation: return GetAbsoluteFromComplex(4, 5);
				case YYPolarisation: return GetAbsoluteFromComplex(6, 7);
				default: throw BadUsageException("Could not extract amplitude part for given polarisation");
			}
		}

		Image2DCPtr GetAmplitudePartFromAutoDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return GetAbsoluteFromComplex(0, 1);
				case YYPolarisation: return GetAbsoluteFromComplex(2, 3);
				default: throw BadUsageException("Could not extract amplitude part for given polarisation");
			}
		}

		Image2DCPtr GetPhasePartFromDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return GetPhaseFromComplex(_images[0], _images[1]);
				case XYPolarisation: return GetPhaseFromComplex(_images[2], _images[3]);
				case YXPolarisation: return GetPhaseFromComplex(_images[4], _images[5]);
				case YYPolarisation: return GetPhaseFromComplex(_images[6], _images[7]);
				default: throw BadUsageException("Could not extract phase part for given polarisation");
			}
		}

		Image2DCPtr GetPhasePartFromAutoDipole(enum PolarisationType polarisation) const
		{
			switch(polarisation)
			{
				case XXPolarisation: return GetPhaseFromComplex(_images[0], _images[1]);
				case YYPolarisation: return GetPhaseFromComplex(_images[2], _images[3]);
				default: throw BadUsageException("Could not extract phase part for given polarisation");
			}
		}

		void CopyFlaggingTo(TimeFrequencyData *data) const
		{
			if(_flagCoverage == NoFlagCoverage)
				data->SetNoMask();
			else if(_flagCoverage == GlobalFlagCoverage || PolarisationCount()==1)
				data->SetGlobalMask(_flagging[0]);
			else if(_flagCoverage == IndividualPolarisationFlagCoverage)
			{
				if(Polarisation() == DipolePolarisation && data->Polarisation() == DipolePolarisation)
					data->SetIndividualPolarisationMasks(_flagging[0], _flagging[1], _flagging[2], _flagging[3]);
				else if((Polarisation()==AutoDipolePolarisation && data->Polarisation() == AutoDipolePolarisation)
					|| (Polarisation()==CrossDipolePolarisation && data->Polarisation() == CrossDipolePolarisation))
					data->SetIndividualPolarisationMasks(_flagging[0], _flagging[1]);
				else
					throw BadUsageException("Trying to copy flagging from dipole time frequency data to single polarisation time frequency data");
			}
			else
				throw BadUsageException("Invalid flag coverage");
		}
		
		Image2DCPtr GetSingleAbsoluteFromComplexDipole() const
		{
			Image2DCPtr real = GetSum(0, 6);
			Image2DCPtr imag = GetSum(1, 7);
			return GetAbsoluteFromComplex(real, imag);
		}

		Image2DCPtr GetSingleAbsoluteFromComplexAutoDipole() const
		{
			Image2DCPtr real = GetSum(0, 2);
			Image2DCPtr imag = GetSum(1, 3);
			return GetAbsoluteFromComplex(real, imag);
		}

		Image2DCPtr GetSingleAbsoluteFromComplexCrossDipole() const
		{
			Image2DCPtr real = GetSum(0, 2);
			Image2DCPtr imag = GetSum(1, 3);
			return GetAbsoluteFromComplex(real, imag);
		}

		Image2DCPtr GetStokesIFromDipole(size_t xx, size_t yy) const
		{
			return GetSum(xx, yy);
		}
		Image2DCPtr GetStokesQFromDipole(size_t xx, size_t yy) const
		{
			return GetDifference(xx, yy);
		}
		Image2DCPtr GetStokesUFromDipole(size_t xy, size_t yx) const
		{
			return GetSum(xy, yx);
		}
		Image2DCPtr GetRealStokesVFromDipole(size_t xyImaginary, size_t yxReal) const
		{
			return GetNegatedSum(xyImaginary, yxReal);
		}
		Image2DCPtr GetImaginaryStokesVFromDipole(size_t xyReal, size_t yxImaginary) const
		{
			return GetDifference(xyReal, yxImaginary);
		}

		Image2DCPtr GetAbsoluteFromComplex(Image2DCPtr real, Image2DCPtr imag) const;
		Image2DCPtr GetPhaseFromComplex(Image2DCPtr real, Image2DCPtr imag) const;

		Image2DCPtr GetSum(size_t left, size_t right) const;
		Image2DCPtr GetNegatedSum(size_t left, size_t right) const;
		Image2DCPtr GetDifference(size_t left, size_t right) const;

		Image2DCPtr GetSinglePhaseFromDipolePhase(size_t xx, size_t yy) const;
		Image2DCPtr GetZeroImage() const;
		template<bool InitValue>
		Mask2DCPtr GetSetMask() const
		{
			return Mask2D::CreateSetMaskPtr<InitValue>(_images[0]->Width(), _images[0]->Height());
		}
		Mask2DCPtr GetCombinedMask() const;

		TimeFrequencyData *CreateTFDataFromSingleComplex(enum PhaseRepresentation phase) const;
		TimeFrequencyData *CreateTFDataFromDipoleComplex(enum PhaseRepresentation phase) const;
		TimeFrequencyData *CreateTFDataFromAutoDipoleComplex(enum PhaseRepresentation phase) const;

		bool _containsData;
		
		enum PhaseRepresentation _phaseRepresentation;
		enum PolarisationType _polarisationType;
		FlagCoverage _flagCoverage;

		// The images that are referenced from this object
		// From least significant to most significant index contribution:
		// phase, polarisation
		std::vector<Image2DCPtr> _images;
		std::vector<Mask2DCPtr> _flagging;
};

#endif
