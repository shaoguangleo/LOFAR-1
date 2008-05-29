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
#ifndef IMAGER_PROCESSCONTROL_H
#define IMAGER_PROCESSCONTROL_H

#include <PLC/ProcessControl.h>
#include <APS/ParameterSet.h>

/**
@author Adriaan Renting
*/

namespace LOFAR
{
  namespace CS1
  {
    class MS_File; //forward declaration
    class Image_File; //forward declaration
    class DFTImager; //forward declaration
    class ImagerProcessControl : public LOFAR::ACC::PLC::ProcessControl
    {
    private:
      std::string itsMSName;
      std::string itsImageName;
      int         itsResolution;
      MS_File*    itsMS;
      Image_File* itsImage;
      DFTImager*  itsImager;
    public:
      ImagerProcessControl(void);

      ~ImagerProcessControl(void);
      // \name Command to control the processes.
      // There are a dozen commands that can be sent to a application process
      // to control its flow. The return values for these command are:<br>
      // - True   - Command executed succesfully.
      // - False  - Command could not be executed.
      //
      // @{

      // During the \c define state the process check the contents of the
      // ParameterSet it received during start-up. When everthing seems ok the
      // process constructs the communication channels for exchanging data
      // with the other processes. The connection are NOT made in the stage.
      tribool define   (void);

      // When a process receives an \c init command it allocates the buffers it
      // needs an makes the connections with the other processes. When the
      // process succeeds in this it is ready for dataprocessing (or whatever
      // task the process has).
      tribool init     (void);

      // During the \c run phase the process does the work it is designed for.
      // The run phase stays active until another command is send.
      tribool run      (void);

      tribool pause(const std::string&);
      tribool quit(void);
      tribool recover(const std::string&);
      tribool reinit(const  std::string&);
      tribool snapshot(const std::string&);
      std::string askInfo(const std::string&);

    }; //class ImagerProcessControl
  } //namespace CS1
}; //namespace LOFAR

#endif
