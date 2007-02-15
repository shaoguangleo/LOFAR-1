/***************************************************************************
 *   Copyright (C) 2007 by ASTRON, Adriaan Renting                         *
 *   renting@astron.nl                                                     *
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
#include <tables/Tables.h>
#include <tables/Tables/TableIter.h>
#include "DFTImager.h"
#include <casa/Quanta/MVEpoch.h>

using namespace casa;

enum CorrelationTypes {None=0,I=1,Q=2,U=3,V=4,RR=5,RL=6,LR=7,LL=8,XX=9,XY=10,YX=11,YY=12}; //found somewhere in AIPS++, don't remember where

//===============>>>  itoa  <<<===============
//ANSI C++ doesn't seem to have a decent function for this, or I'm not aware of it. Need to rename it to IntToStr(), to avoid confusion
std::string itoa(int value, int base=10)
{ // found on http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
  //maybe copyright Robert Jan Schaper, Ray-Yuan Sheu, Rodrigo de Salvo Braz, Wes Garland and John Maloney
  enum { kMaxDigits = 35 };
  std::string buf;
  buf.reserve( kMaxDigits ); // Pre-allocate enough space.
        // check that the base if valid
  if (base < 2 || base > 16) return buf;
  int quotient = value;
        // Translating number to string with base:
  do {
    buf += "0123456789abcdef"[ std::abs( quotient % base ) ];
    quotient /= base;
  } while ( quotient );
        // Append the negative sign for base 10
  if ( value < 0 && base == 10) buf += '-';
  std::reverse( buf.begin(), buf.end() );
  return buf;
}

namespace LOFAR
{
  namespace CS1
  {
    //===============>>>  DFTImager::DFTImager  <<<===============
    /* initialize some meta data and get the datastorage the right size. */
    DFTImager::DFTImager(MS_File* InputMSfile,
                        Image_File*InputImageFile)
    {
      MSfile           = InputMSfile;
      Imagefile        = InputImageFile;
      NumAntennae      = (*MSfile).itsNumAntennae;
      NumPairs         = (*MSfile).itsNumPairs;
      NumBands         = (*MSfile).itsNumBands;
      NumChannels      = (*MSfile).itsNumChannels;
      NumPolarizations = (*MSfile).itsNumPolarizations;
      NumTimeslots     = (*MSfile).itsNumTimeslots;
      AntennaNames     = (*MSfile).itsAntennaNames;
      Resolution       = 41;

      PairsIndex.resize(NumPairs);
    //  Statistics = Cube<int>(NumBands, NumAntennae, NumAntennae, 0);
    //  PolarizationsToCheck.resize(NumPolarizations);
    //  DeterminePolarizationsToCheck(UseOnlyXpolarizations);

      TimeslotData.resize(NumPairs*NumBands);
      for (int i = 0; i < NumPairs*NumBands; i++)
      { TimeslotData[i].resize(NumPolarizations, NumChannels, WindowSize);
      }

      int index = 0;
      for (int i = 0; i < NumAntennae; i++)
      { for(int j = i; j < NumAntennae; j++)
        { PairsIndex[index]           = pairii(i, j);
          BaselineIndex[pairii(i, j)] = index++;
        }
      }
      ComputeBaselineLengths();
    }

    //===============>>>  DFTImager::~DFTImager  <<<===============

    DFTImager::~DFTImager()
    {
    }

    //===============>>> DFTImager::DetermineCorrelationsToCheck <<<===============
    /* create a list of polarizations we want to check, maybe we only want to to XY, YX */
    // void DFTImager::DeterminePolarizationsToCheck(bool UseOnlyXpolarizations)
    // {
    //   if (UseOnlyXpolarizations)
    //   {
    //     bool noCorrError = true;
    //     for (int i = 0; i < NumPolarizations; i++)
    //     {
    //       switch((*MSfile).itsPolarizations[i])
    //       {
    //         case None:
    //         case I:
    //         case RR:
    //         case LL:
    //         case XX:
    //         case YY:
    //           if (UseOnlyXpolarizations)
    //             PolarizationsToCheck[i] = false;
    //           break;
    //         case Q:
    //         case U:
    //         case V:
    //         case RL:
    //         case LR:
    //         case XY:
    //         case YX:
    //           noCorrError = false;
    //           if (UseOnlyXpolarizations)
    //             PolarizationsToCheck[i] = true;
    //           break;
    //       }
    //     }
    //     if (noCorrError)
    //     {
    //       cout << "There are no crosspolarizations to flag!";
    //       exit(1);
    //     }
    //   }
    //   else
    //   {
    //     for (int i = 0; i < NumPolarizations; i++)
    //     {  PolarizationsToCheck[i] = true;
    //     }
    //   }
    // }

    //===============>>> DFTImager::ComputeBaselineLengths  <<<===============
    /* compute baseline lengths, and determine the longest one.*/
    void DFTImager::ComputeBaselineLengths()
    {
      MaxBaselineLength = 0.0;
      BaselineLengths.resize(NumPairs);
      //Antenna positions
      MSAntenna antennas    = (*MSfile).antennas();
      ROArrayColumn<Double>  position(antennas, "POSITION");
      for (int i = 0; i < NumAntennae; i++ )
      {
        for (int j = i; j < NumAntennae; j++)
        {
          Vector<Double> p(position(i) - position(j));
          double temp   = sqrt(p(0)*p(0) + p(1)*p(1) + p(2)*p(2));
          BaselineLengths[BaselineIndex[pairii(i, j)]] = temp;
          if (temp > MaxBaselineLength && temp < 3000000) //radius of the Earth in meters? WSRT sometimes has fake telescopes at 3854243
          { MaxBaselineLength = temp;                     // non-existent antenna's can have position (0,0,0)
          }
        }
      }
    }

    //===============>>> DFTImager::ImageTimeslot  <<<===============
    /* This function iterates over baseline and band and uses FlagBaselineBand() to determine
      for each one if it needs to be flagged. It treats the autocorrelations separately,
      to detect entire malfunctioning telescopes. Finally it writes the flags.
    */
    void DFTImager::ImageTimeslot()
    {
/*      Cube<bool>     Image(NumPolarizations, NumChannels, NumPairs*NumBands, false);
      vector<bool>   test(NumPairs*NumBands);
      for (int i = 0; i < NumBands; i++)
      {
        vector< vector<double> > CrossCorrRMS(NumPolarizations, NumPairs - NumAntennae);
        for(int j = 0; j < NumAntennae; j++)
        {
          for(int k = j+1; k < NumAntennae; k++)
          {
            int index = i*NumPairs + BaselineIndex[pairii(j, k)];
            Matrix<Bool> image = Image.xyPlane(index);
            if ((BaselineLengths[BaselineIndex[pairii(j, k)]] < 3000000))//radius of the Earth in meters? WSRT sometimes has fake telescopes at 3854243 m
            { //we skip the non-existent telescopes
              test = ImageBaselineBand(&image,
                                        &(TimeslotData[index]),
                                        Position);
            }
          }
        }
        //this code could be separated in to different functions
        for(int j = 0; j < NumAntennae; j++) //write the data
        {
          for(int k = j; k < NumAntennae; k++)
          {
            int index = i*NumPairs + BaselineIndex[pairii(j, k)];
            Matrix<bool> image = Image.xyPlane(index);
            (*Imagefile).WriteImageData(&image);
          }
        }
      }*/
    }

    //===============>>> DFTImager::UpdateTimeslotData  <<<===============
    /* This function reads the visibility data for one timeslot and checks if
      a mosaicing mode is active*/
    /* The datastructure Timeslotdata is rather complex because it flattens
       half-filled antenna x antenna x bands matrix into a vector of NumPairs X bands length. */
    bool DFTImager::UpdateTimeslotData(vector<int>* OldFields,
                                                  vector<int>* OldBands,
                                                  int* TimeCounter,
                                                  Table* TimeslotTable,
                                                  double* Time)
    {
      int           rowcount = (*TimeslotTable).nrow();
      ROTableVector<Int>     antenna1((*TimeslotTable), "ANTENNA1");
      ROTableVector<Int>     antenna2((*TimeslotTable), "ANTENNA2");
      ROTableVector<Int>     bandnr  ((*TimeslotTable), "DATA_DESC_ID");
      ROTableVector<Int>     fieldid ((*TimeslotTable), "FIELD_ID");
      ROTableVector<Double>  time    ((*TimeslotTable), "TIME_CENTROID");//for testing purposes
      ROArrayColumn<Complex> data    ((*TimeslotTable), "DATA");
      Cube<Complex>          Data(NumPolarizations, NumChannels, rowcount);

      data.getColumn(Data); //We're not checking Data.nrow() Data.ncolumn(), assuming all data is the same size.
      (*Time) = time(0);//for testing purposes, might be useful in the future

      bool NewField_or_Frequency = false;

      for (int i = 0; i < rowcount; i++)
      {
        int bi    = BaselineIndex[pairii(antenna1(i), antenna2(i))];
        int field = fieldid(i);
        int band  = bandnr(i);
        int index = (band % NumBands) * NumPairs + bi;
        if ((*TimeCounter) > WindowSize - 1)
        {
          if (field != (*OldFields)[bi]) //pointing mosaicing
          { NewField_or_Frequency = true;
          }
          if (band  != (*OldBands)[index]) //frequency mosaicing
          { NewField_or_Frequency = true;
          }
        }
        (*OldFields)[bi]   = field;
        (*OldBands)[index] = band;

        TimeslotData[index].xyPlane((*TimeCounter) % WindowSize) = Data.xyPlane(i); //TimeslotData is only WindowSize size!
      }
      (*TimeCounter)++; //we want to reset if we detect a gap in DATA_DESC_ID or FIELD. Maybe TIME too???
      return NewField_or_Frequency; //assuming everybody is changing field or frequency at the same time!
    }

    //===============>>> DFTImager::FlagDataOrBaselines  <<<===============
    /* This function iterates over the data per timeslot and uses Flagtimeslot()
      to actually flag datapoints (if flagDatapoints), and entire baselines (if flagRMS)*/
    void DFTImager::MakeImage(int Resolution)
    {
      TableIterator timeslot_iter = (*MSfile).TimeslotIterator();
      int           TimeCounter   = 0;
      double        Time          = 0.0;//for testing purposes
      vector<int>   OldFields(NumPairs);         //to check on multipointing and mosaicing
      vector<int>   OldBands(NumPairs*NumBands); //to check on multifrequency and freq mosaicing
      vector< Cube<float> > Image(NumBands);

      int           step          = NumTimeslots / 10 + 1; //not exact but it'll do
      int           row           = 0;

      while (!timeslot_iter.pastEnd())
      {
        Table TimeslotTable  = timeslot_iter.table();
        bool  NewFieldorFreq = UpdateTimeslotData(&OldFields,
                                                  &OldBands,
                                                  &TimeCounter,
                                                  &TimeslotTable,
                                                  &Time);
        if (NewFieldorFreq)
        {
          cout << "Error: current version can not handle multiple fields/frequqncies in dataset." << endl;
          break;
        }
        cout << "Processing: " << MVTime(Time/(24*3600)).string(MVTime::YMD) << endl; //for testing purposes

        ImageTimeslot();
        timeslot_iter++;
        if (row++ % step == 0) // to tell the user how much % we have processed,
        { cout << 10*(row/step) << "%" << endl; //not very accurate for low numbers of timeslots, but it'll do for now
        }
      }
      Imagefile->WriteImage(&Image);
    }
    //===============>>> DFTImager  <<<===============
  } //namespace CS1
}; //namespace LOFAR
