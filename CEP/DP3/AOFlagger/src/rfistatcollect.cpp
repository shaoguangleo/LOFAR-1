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

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <cmath>

#include <AOFlagger/msio/date.h>
#include <AOFlagger/rfi/rfistatistics.h>
#include <sys/stat.h>

using namespace std;

void writeFrequencyTotals(const std::map<double, double> &frequencyFlags)
{
	ofstream fileFreq("frequency-totals.txt");
	for(std::map<double, double>::const_iterator i=frequencyFlags.begin();i!=frequencyFlags.end();++i)
		fileFreq << i->first << '\t' << i->second << '\n';
	fileFreq.close();
}

void writeTimeTotals(const std::map<double, long unsigned> &timeTotalCount, const std::map<double, long unsigned> &timeFlagsCount)
{
	ofstream fileTime("time-totals.txt");
	for(std::map<double, long unsigned>::const_iterator i=timeFlagsCount.begin(),
			j=timeTotalCount.begin();
			i!=timeFlagsCount.end();++i,++j)
		fileTime << j->first << '\t' << Date::AipsMJDToTimeString(j->first) << '\t' << j->second << '\t' << i->second << '\t' << (100.0 * (double) i->second / (double) j->second) << '\n';
	fileTime.close();
}

void writeSubBands(const std::map<double, double> &frequencyFlags)
{
	ofstream fileBand("subband-totals.txt");
	size_t index = 0;
	double bandTotal = 0.0;
	for(std::map<double, double>::const_iterator i=frequencyFlags.begin();i!=frequencyFlags.end();++i)
	{
		bandTotal += i->second;
		if(index%255 == 0)
			fileBand << (index/255) << '\t' << i->first << '\t';
		else if(index%255 == 254)
		{
			fileBand << i->first << '\t' << (bandTotal/255.0) << '\n';
			bandTotal = 0.0;
		}
		++index;
	}
	fileBand.close();
	if(index%255 != 0)
		cout << "Warning: " << (index%255) << " rows were not part of a sub-band (channels were not dividable by 256)" << endl;
} 

void writeFlagsPerHour(const std::map<double, long unsigned> &timeTotalCount, const std::map<double, long unsigned> &timeFlagsCount)
{
	ofstream fileTimeph("time-per-hour.txt");
	double lastTime = timeFlagsCount.begin()->first;
	double hourTotal = 0.0;
	const double secondsPerHour = 3600.0;
	size_t curCount = 0;
	for(std::map<double, size_t>::const_iterator i=timeFlagsCount.begin(), j=timeTotalCount.begin();i!=timeFlagsCount.end();++i,++j)
	{
		if(floor(lastTime/secondsPerHour) != floor(i->first/secondsPerHour))
		{
			fileTimeph << floor(lastTime/secondsPerHour) << '\t' << (100.0*hourTotal/curCount) << '\t' << curCount << '\n';
			hourTotal = 0.0;
			curCount=0;
		}
		++curCount;
		hourTotal += (double) i->second / (double) j->second;
		lastTime = i->first;
	}
	fileTimeph << floor(lastTime/secondsPerHour) << '\t' << (100.0*hourTotal/curCount) << '\t' << curCount << '\n';
	fileTimeph.close();
}

void writeTimeForPlot(const std::map<double, long unsigned> &timeTotalCount, const std::map<double, long unsigned> &timeFlagsCount)
{
	ofstream fileTimePlot("time-for-plot.txt");
	size_t curCount = 0;
	double curTimeStart = timeTotalCount.begin()->first;
	double total = 0.0;
	for(std::map<double, size_t>::const_iterator i=timeFlagsCount.begin(), j=timeTotalCount.begin();i!=timeFlagsCount.end();++i,++j)
	{
		if(curCount >= timeTotalCount.size()/200)
		{
			fileTimePlot << curTimeStart << '\t' << (100.0*total/curCount) << '\t' << curCount << '\n';
			total = 0.0;
			curCount=0;
			curTimeStart = i->first;
		}
		++curCount;
		total += (double) i->second / (double) j->second;
	}
	fileTimePlot << curTimeStart << '\t' << (100.0*total/curCount) << '\t' << curCount << '\n';
	fileTimePlot.close();
}

void writeMhzTotals(const std::map<double, double> &frequencyFlags)
{
	ofstream fileMhz("mhz-totals.txt");
	double lastFreq = frequencyFlags.begin()->first;
	double mhzTotal = 0.0;
	size_t curCount = 0;
	for(std::map<double, double>::const_iterator i=frequencyFlags.begin();i!=frequencyFlags.end();++i)
	{
		if(round(lastFreq/1e6) != round(i->first/1e6))
		{
			fileMhz << round(lastFreq/1e6) << '\t' << (mhzTotal/curCount) << '\t' << curCount << '\n';
			mhzTotal = 0.0;
			curCount=0;
		}
		++curCount;
		mhzTotal += i->second;
		lastFreq = i->first;
	}
	fileMhz << round(lastFreq/1e6) << '\t' << (mhzTotal/curCount) << '\t' << curCount << '\n';
	fileMhz.close();
}

void readChannels(RFIStatistics &statistics, std::string &filename, bool autocorrelation)
{
	ifstream f(filename.c_str());
	while(!f.eof())
	{
		RFIStatistics::ChannelInfo channel;
		f
		>> channel.frequencyHz
		>> channel.totalCount
		>> channel.rfiCount
		>> channel.rfiSummedAmplitude
		>> channel.broadbandRfiCount
		>> channel.lineRfiCount
		>> channel.broadbandRfiAmplitude
		>> channel.lineRfiAmplitude;
		statistics.Add(channel, autocorrelation);
	}
}

void readTimesteps(RFIStatistics &statistics, std::string &filename, bool autocorrelation)
{
	ifstream f(filename.c_str());
	while(!f.eof())
	{
		RFIStatistics::TimestepInfo timestep;
		f
		>> timestep.time
		>> timestep.totalCount
		>> timestep.rfiCount
		>> timestep.rfiSummedAmplitude
		>> timestep.broadbandRfiCount
		>> timestep.lineRfiCount
		>> timestep.broadbandRfiAmplitude
		>> timestep.lineRfiAmplitude;
		statistics.Add(timestep, autocorrelation);
	}
}

void readAmplitudes(RFIStatistics &statistics, std::string &filename, bool autocorrelation)
{
	ifstream f(filename.c_str());
	while(!f.eof())
	{
		RFIStatistics::AmplitudeBin amplitude;
		f
		>> amplitude.centralAmplitude
		>> amplitude.centralLogAmplitude
		>> amplitude.count
		>> amplitude.rfiCount
		>> amplitude.broadbandRfiCount
		>> amplitude.lineRfiCount;
		statistics.Add(amplitude, autocorrelation);
	}
}

void readBaselines(RFIStatistics &statistics, std::string &filename)
{
	ifstream f(filename.c_str());
	while(!f.eof())
	{
		RFIStatistics::BaselineInfo baseline;
		f
		>> baseline.antenna1
		>> baseline.antenna2
		>> baseline.antenna1Name
		>> baseline.antenna2Name
		>> baseline.count
		>> baseline.rfiCount
		>> baseline.broadbandRfiCount
		>> baseline.lineRfiCount;
		statistics.Add(baseline);
	}
}

int main(int argc, char **argv)
{
	cout << 
		"RFI statistics collector\n"
		"This program will collect statistics of several rficonsole runs and\n"
		"write them in one file.\n\n"
		"Author: André Offringa (offringa@astro.rug.nl)\n"
		<< endl;

	if(argc == 1)
	{
		std::cerr << "Usage: " << argv[0] << " [files]" << std::endl;
	}
	else
	{
		std::map<double, double> frequencyFlags;
		std::map<double, long unsigned> timeTotalCount, timeFlagsCount;
		RFIStatistics statistics;

		for(int i=1;i<argc;++i)
		{
			string filename = argv[i];
			cout << "Reading " << filename << "..." << endl;
			if(filename.find("counts-channels-auto.txt"))
				readChannels(statistics, filename, true);
			else if(filename.find("counts-channels-cross.txt"))
				readChannels(statistics, filename, false);
			else if(filename.find("counts-timesteps-auto.txt"))
				readTimesteps(statistics, filename, true);
			else if(filename.find("counts-timesteps-cross.txt"))
				readTimesteps(statistics, filename, false);
			else if(filename.find("counts-amplitudes-auto.txt"))
				readAmplitudes(statistics, filename, true);
			else if(filename.find("counts-amplitudes-cross.txt"))
				readAmplitudes(statistics, filename, false);
			else if(filename.find("counts-baselines.txt"))
				readBaselines(statistics, filename);
			bool
				antennafile = filename.find("antenn") != string::npos,
				freqfile = filename.find("freq") != string::npos,
				timefile = filename.find("time") != string::npos;
			if((antennafile && freqfile) || (freqfile && timefile) || (antennafile && timefile) || !(antennafile || freqfile || timefile))
				throw runtime_error("Could not determine type of file.");

			
			if(freqfile)
			{
				ifstream f(filename.c_str());
				while(!f.eof())
				{
					double frequency, totalCount, flagCount, percentage;
					f >> frequency >> totalCount >> flagCount >> percentage;
					frequencyFlags.insert(pair<double, double>(frequency, percentage));
				}
			} else if(timefile)
			{
				ifstream f(filename.c_str());
				while(!f.eof())
				{
					double time, percentage;
					unsigned long totalCount, flagCount;
					f >> time >> totalCount >> flagCount >> percentage;
					if(timeTotalCount.count(time)!=0)
					{
						totalCount += timeTotalCount[time];
						flagCount += timeFlagsCount[time];
					}
					timeTotalCount[time] = totalCount;
					timeFlagsCount[time] = flagCount;
				}
			}
		}
		writeFrequencyTotals(frequencyFlags);
		writeTimeTotals(timeTotalCount, timeFlagsCount);
		writeSubBands(frequencyFlags);
		writeFlagsPerHour(timeTotalCount, timeFlagsCount);
		writeTimeForPlot(timeTotalCount, timeFlagsCount);
		writeMhzTotals(frequencyFlags);
		statistics.Save();
	}
}
