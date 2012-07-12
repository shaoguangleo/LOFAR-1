#include <iostream>

#include <AOFlagger/remote/clusteredobservation.h>
#include <AOFlagger/remote/observationtimerange.h>
#include <AOFlagger/remote/processcommander.h>

using namespace std;
using namespace aoRemote;

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		cerr << "Usage: aofrequencyfilter <reffile>\n";
	}
	else {
		ClusteredObservation *obs = ClusteredObservation::Load(argv[1]);
		ProcessCommander commander(*obs);
		commander.PushReadAntennaTablesTask();
		commander.PushReadBandTablesTask();
		commander.Run(false);
		commander.CheckErrors();
		
		ObservationTimerange timerange(*obs);
		const std::vector<BandInfo> &bands = commander.Bands();
		for(size_t i=0; i!=bands.size(); ++i)
			timerange.SetBandInfo(i, bands[i]);
		
		unsigned polarizationCount = 4;
		size_t timestepCount = 128;
		
		timerange.Initialize(polarizationCount, timestepCount);
		MSRowDataExt *rowBuffer[obs->Size()];
		for(size_t i=0;i<obs->Size();++i)
			rowBuffer[i] = new MSRowDataExt[timestepCount];
		// We ask for "0" rows, which means we will ask for the total number of rows
		commander.PushReadDataRowsTask(timerange, 0, 0, rowBuffer);
		commander.Run(false);
		commander.CheckErrors();
		const size_t totalRows = commander.RowsTotal();
		cout << "Total rows to filter: " << totalRows << '\n';
		
		for(size_t i=0;i<obs->Size();++i)
			delete[] rowBuffer[i];
		delete obs;
	}
}
