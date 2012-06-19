/* TBB_Writer_main.cc
 * 
 * LOFAR Transient Buffer Boards (TBB) Data Writer  Copyright (C) 2011, 2012
 * ASTRON (Netherlands Institute for Radio Astronomy)
 * P.O. Box 2, 7990 AA Dwingeloo, The Netherlands.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the LOFAR software suite.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * $Id: TBB_Writer_main.cc 29470 2012-03-14 18:58:53Z amesfoort $
 */
/* 
 * Parts derived from the BF writer written by Jan David Mol, and from
 * TBB writers written by Lars Baehren, Andreas Horneffer, and Joseph Masters.
 */

/*
 * NOTE: Only transient data mode has been implemented and tested.
 * Some code for spectral mode is implemented, but it could never be tested and the TBB HDF5 format considers transient data only.
 * Currently, users I talked to don't care about spectral data, although the hardware should be able to supply it. (Mar 2012)
 */

// Enable sigaction(2), gethostname(2), timer_create(2), and thread-safe _r functions, like gmtime_r().
#define _POSIX_C_SOURCE				200112L
#define _GNU_SOURCE				1	// getopt_long(3)

#include <lofar_config.h>			// before any other include

#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <sys/types.h>
#include <csignal>
#include <ctime>					// strftime()
#include <sys/stat.h>
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <getopt.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <boost/lexical_cast.hpp>

#include <Common/SystemUtil.h>
#include <Common/StringUtil.h>
#include <Common/StreamUtil.h>
#include <Common/Thread/Thread.h>
#include <Common/Exceptions.h>
#include <Common/NewHandler.h>

#include <Storage/TBB_Writer.h>
#include <Storage/TBB_StaticMapping.h>
#include <Storage/IOPriority.h>

#if defined HAVE_PKVERSION
#include "Package__Version.h"		// generated by cmake
#endif

#define TBB_DEFAULT_BASE_PORT		0x7bb0	// i.e. tbb0
#define TBB_DEFAULT_LAST_PORT		0x7bbb	// 0x7bbf for NL, 0x7bbb for int'l stations

using namespace std;

struct progArgs {
	string inFilename;
	string outFilename;
	string parsetFilename;
	string conMapFilename;
	string proto;
	uint16_t port;
	struct timeval timeoutVal;
	bool keepRunning;
};

// Install a new handler to produce backtraces for std::bad_alloc.
LOFAR::NewHandler badAllocExcHandler(LOFAR::BadAllocException::newHandler);

static void termSigsHandler(int sig_nr) {
	if (sig_nr == SIGALRM) {
		// do nothing
	}
}

/*
 * Register signal handlers for SIGINT and SIGTERM to gracefully terminate early,
 * so we can break out of blocking system calls and exit without corruption of already written output.
 * Leave SIGQUIT (Ctrl-\) untouched, so users can still easily quit immediately.
 */
static void setTermSigsHandler() {
	struct sigaction sa;

	sa.sa_handler = termSigsHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	int err = sigaction(SIGINT,  &sa, NULL); // keyb INT (typically Ctrl-C)
	err    |= sigaction(SIGTERM, &sa, NULL);
	err    |= sigaction(SIGALRM, &sa, NULL); // for setitimer(); don't use sleep(3) and friends
	if (err != 0) {
		LOG_WARN("Failed to register SIGINT/SIGTERM handler to allow manual, early, graceful program termination.");
	}
}

static unsigned stationNameToId(const string& stName) {
	if (stName.size() < 5) {
		throw LOFAR::Exception("Station name must be at least 5 characters");
	}
	unsigned id = LOFAR::strToUint(stName.substr(2, 3));
	if (id > 200) {
		unsigned sub = (id - 101) / 100;
		id = id - sub * 80;
	}
	return id;
}

// For in filenames and indirectly, the FILEDATE attribute. The output_format is without seconds; the output_size is incl the '\0'.
static string formatFilenameTimestamp(const struct timeval& tv, const char* output_format, const char* output_format_secs, size_t output_size) {
	struct tm tm = {0}; // init in case gmtime_r() returns NULL (if year does not fit into an int, unlikely)
	gmtime_r(&tv.tv_sec, &tm);
	double secs = tm.tm_sec + tv.tv_usec / 1000000.0;

	char* date_str = new char[output_size];
	size_t nwritten = strftime(date_str, output_size, output_format, &tm);
	/*int nprinted = */snprintf(date_str + nwritten, sizeof(output_format) - nwritten, output_format_secs, secs); // e.g. %06.3f: _total_ width of 6 of "ss.sss"

	string date(date_str);
	delete[] date_str;
	return date;
}

// Returns last mod date/time of filename or current time of day if stat()ing
// filename fails, in "YYYY-MM-DDThh:mm:ss.s" UTC format.
// For FILEDATE attribute.
static string getFileModDate(const string& filename) {
	struct timeval tv;
	struct stat st;
	int err;

	err = stat(filename.c_str(), &st);
	if (err != 0) {
		gettimeofday(&tv, NULL); // If stat() fails, this is close enough to file mod date.
	} else {
		tv.tv_sec = st.st_mtime;
		tv.tv_usec = st.st_mtim.tv_nsec / 1000;
	}

	const char output_format[] = "%Y-%m-%dT%H:%M:";
	const char output_format_secs[] = "%04.1f"; // _total_ width of 4 of "ss.s"
	const char output_format_example[] = "YYYY-MM-DDThh:mm:ss.s";
	return formatFilenameTimestamp(tv, output_format, output_format_secs, sizeof(output_format_example));
}

// For UTC timestamp attributes.
static string timeStr(double time) {
	time_t timeSec = static_cast<time_t>(floor(time));
	unsigned long timeNSec = static_cast<unsigned long>(round( (time-floor(time))*1e9 ));

	char utcstr[50];
	if (strftime( utcstr, sizeof(utcstr), "%Y-%m-%dT%H:%M:%S", gmtime(&timeSec) ) == 0) {
		return "";
	}

	return LOFAR::formatString("%s.%09luZ", utcstr, timeNSec);
}

static double toMJD(double time) {
	// 40587 modify Julian day number = 00:00:00 January 1, 1970, GMT
	return 40587.0 + time / (24*60*60);
}

// Populates the Station group. Note: Does _not_ set the dipole datasets.
static void setStationGroup(DAL::TBB_Station& st, const LOFAR::RTCP::Parset& parset, const string& stName, const vector<double>& stPosition) {
	st.groupType().value = "StationGroup";
	st.stationName().value = stName;

	st.stationPositionValue().value = stPosition;
	//st.stationPositionUnit().value = ???; // TODO: ??? array of strings? (also at beamDirection, but not at antennaPosition)
	st.stationPositionFrame().value = parset.positionType(); // returns "ITRF"

	// TODO: Also see bf writer MS makeBeamTable() kind of function. In the future, also indicates broken tiles/dipoles w/ timestamp.
	// TODO: ITRF -> ITRF2008...?
	if (parset.haveAnaBeam()) { // HBA // TODO: AnaBeam vs Beam?
		st.beamDirectionValue().value = parset.getAnaBeamDirection(); // always for beam 0
		st.beamDirectionFrame().value = parset.getAnaBeamDirectionType(); // idem
	} else {
		unsigned nBeams = parset.nrBeams(); // TODO: What if >1 station beams? Now, only write beam0. Probably irrel for now, because of AnaBeam (HBA).
		if (nBeams > 0) {
			st.beamDirectionValue().value = parset.getBeamDirection(0);
			st.beamDirectionFrame().value = parset.getBeamDirectionType(0); // TODO: fix getBeamDirectionType() sprintf -> snprintf (check all parset funcs)
		} else { // No beam (known or at all), so set null vector
			vector<double> noBeamDirVal(2, 0.0);
			st.beamDirectionValue().value = noBeamDirVal;
			st.beamDirectionFrame().value = "ITRF";
		}
	}
	//st.beamDirectionUnit().value = ???; // TODO: ??? beam dir = 2 angles. array of string??? "radians"

	st.clockOffsetValue().value = parset.clockCorrectionTime(stName); // TODO: check if stName is as expected; returns 0.0 if not avail
	st.clockOffsetUnit().value = "s";

	//st.triggerOffset().value = ???; // TODO: remove from DAL

	// Create dipole datasets when the first datagram for that dipole has been received.
}

static void setTriggerGroup(DAL::TBB_Trigger& tg, const LOFAR::RTCP::Parset& parset) {
	tg.groupType().value = "TriggerGroup";
//	tg.triggerType().value = "Unknown"; // We don't get this or any other trigger data yet, so do the minimum.
//	tg.triggerVersion().value = 0; // There is no trigger alg impl yet.

	// TODO: put these into DAL, because we can set them
	// Trigger parameters (how to decide if there is a trigger) (per obs)
//	tg.paramCoincidenceChannels().value = parset.tbbNumberOfCoincidenceChannels();	// int(->unsigned); from Observation.ObservationControl.StationControl.TBBControl.NoCoincChann
//	tg.paramCoincidenceTime()    .value = parset.tbbCoincidenceTime();				// double; from Observation.ObservationControl.StationControl.TBBControl.CoincidenceTime
//	tg.paramDirectionFit()       .value = parset.tbbDoDirectionFit();				// string; from Observation.ObservationControl.StationControl.TBBControl.DoDirectionFit
//	tg.paramElevationMin()       .value = parset.tbbMinElevation();					// double; from Observation.ObservationControl.StationControl.TBBControl.MinElevation
//	tg.paramFitVarianceMax()     .value = parset.tbbMaxFitVariance();				// double; Observation.ObservationControl.StationControl.TBBControl.MaxFitVariance
	// unused: Observation.ObservationControl.StationControl.TBBControl.ParamExtension = []

	// Trigger data (per trigger)
	// N/A atm

	// TODO: (this is already done for any attribute in any group?)
	/* From an e-mail by Pim Schellart:
	 * To actually implement this, given the likely need for many
	 * changes, it is probably easiest to provide only one method for the
	 * TriggerGroup that allows setting (keyword, value) pairs and one to
	 * retrieve an attribute. That way we can just get/set whatever we need
	 * in that group without having to bother you with it.
	 */
}

static void setTBB_RootAttributes(DAL::TBB_File& file, const LOFAR::RTCP::Parset& parset, const string& stName) {
	//file.operatingMode().value = "transient"; // TODO: add string mode field: "transient" or "spectral". Maybe it's Observation.TBB.TBBsetting.operatingMode = 1

	// The writer creates one HDF5 file per station, so create only one Station Group here.
	DAL::TBB_Station station(file.station(stName));

	// Find the station name we're looking for ("CS001" == "CS001HBA0") and retrieve it's pos using the found idx.
	vector<double> stPos;

	vector<string> obsStationNames(parset.allStationNames()); // can also contain "CS001HBA0"
	vector<string>::const_iterator nameIt(obsStationNames.begin());

	vector<double> stationPositions(parset.positions()); // len must be (is generated as) 3x #stations
	vector<double>::const_iterator posIt(stationPositions.begin());

	const size_t stNameLen = 5;
	for ( ; nameIt != obsStationNames.end(); ++nameIt, posIt += 3) {
		if (nameIt->substr(0, stNameLen) == stName) {
			break;
		}
	}
	if (nameIt != obsStationNames.end() // found?
				&& posIt < stationPositions.end()) { // only fails if Parset provided broken vectors
		stPos.assign(posIt, posIt + 3);
	} else { // not found or something wrong; create anyway or we lose data
		stPos.assign(3, 0.0);
	}
	station.create();
	setStationGroup(station, parset, stName, stPos);

	// Trigger Group
	DAL::TBB_Trigger tg(file.triggerData()); // TODO: triggerData() -> trigger()
	tg.create();
	setTriggerGroup(tg, parset);

	// TODO: dump all Observation.TBB.* parset attribs? what are those?
}

static void setCommonLofarAttributes(DAL::TBB_File& file, const LOFAR::RTCP::Parset& parset, const string& filename) {
	file.groupType().value = "Root";
	const string baseFilename(LOFAR::basename(filename));
	file.fileName().value = baseFilename;
	file.fileDate().value = getFileModDate(baseFilename);
	file.fileType().value = "tbb"; // TODO: this is obviously tbb specific; the rest of this function is not. -> DAL; also the next one

	file.telescope().value = "LOFAR";
	file.observer() .value = "unknown"; // TODO: name(s) of the observer(s); bf writes "unknown"

	file.projectID()     .value = parset.getString("Observation.Campaign.name");
	file.projectTitle()  .value = parset.getString("Observation.Campaign.title");
	file.projectPI()     .value = parset.getString("Observation.Campaign.PI");
	ostringstream oss;
	// Use ';' instead of ',' to pretty print, because ',' already occurs in names (e.g. Smith, J.).
	LOFAR::writeVector(oss, parset.getStringVector("Observation.Campaign.CO_I"), "; ", "", "");
	file.projectCOI()    .value = oss.str();
	file.projectContact().value = parset.getString("Observation.Campaign.contact");

/*	// TODO: add getString() and getStringVector() routines to RTCP Parset (sub)class
	file.projectID()     .value = parset.observationName(); // TODO: check if Name -> ID
	file.projectTitle()  .value = parset.observationTitle();
	file.projectPI()     .value = parset.observationPI();
	file.projectCOI()    .value = parset.observationCOI(); // TODO: see above
	file.projectContact().value = parset.observationContact();
*/
	//file.observationID() .value = str(format("%s") % parset.observationID());
	file.observationID() .value = LOFAR::formatString("%u", parset.observationID());

	file.observationStartUTC().value = timeStr(parset.startTime()); // TODO: parset gives this as a double?!?! also below
	file.observationStartMJD().value = toMJD(parset.startTime()); // TODO: needed? also below

	// The stop time can be a bit further than the one actually specified, because we process in blocks.
	const size_t nBlocks = ceil((parset.stopTime() - parset.startTime()) / parset.CNintegrationTime());
	const double stopTime = parset.startTime() + nBlocks * parset.CNintegrationTime();
	file.observationEndUTC().value = timeStr(stopTime);
	file.observationEndMJD().value = toMJD(stopTime);

	file.observationNofStations().value = parset.nrStations(); // TODO: SS beamformer?

	// For the observation attribs, dump all stations participating in the observation (i.e. allStationNames(), not mergedStatioNames()).
	// This may not correspond to which station HDF5 groups will be written, but that is true anyway, regardless of any merging (e.g. w/ piggy-backed TBB).
	file.observationStationsList().value = parset.allStationNames(); // TODO: SS beamformer?

	// TODO: DAL: this is for the original obs; DAL TBB users need to know which stations and dipoles are actually there. Use some way to enumerate to get it from HDF5. And some way to iterate over all or over a user-specified selection (see DAL1).

	const vector<double> subbandCenterFrequencies(parset.subbandToFrequencyMapping());
	double min_centerfrequency = *min_element(subbandCenterFrequencies.begin(), subbandCenterFrequencies.end());
	double max_centerfrequency = *max_element(subbandCenterFrequencies.begin(), subbandCenterFrequencies.end());
	double sum_centerfrequencies = accumulate(subbandCenterFrequencies.begin(), subbandCenterFrequencies.end(), 0.0);
	double subbandBandwidth = parset.sampleRate();

	file.observationFrequencyMin()   .value = (min_centerfrequency - subbandBandwidth / 2) / 1e6;
	file.observationFrequencyCenter().value = sum_centerfrequencies / subbandCenterFrequencies.size();
	file.observationFrequencyMax()   .value = (max_centerfrequency + subbandBandwidth / 2) / 1e6;
	file.observationFrequencyUnit()  .value = "MHz";

	file.observationNofBitsPerSample().value = parset.nrBitsPerSample();

	file.clockFrequency()    .value = parset.clockSpeed() / 1e6;
	file.clockFrequencyUnit().value = "MHz";

	file.antennaSet().value = parset.antennaSet(); // TODO: does this provide the strings from the ICD const tables?!?

	file.filterSelection().value = parset.getString("Observation.bandFilter");

	//file.target().value = ???; // TODO: not in bf h5 writer; as array of strings?
	// Code from bf MS Writer:
	//vector<string> targets(parset.getStringVector("Observation.Beam[" + toString(subarray) + "].target"));

	/*unexp
	 * Set release string, e.g. "LOFAR-Release-1.0".
	 * Only works if writer is rebuilt and reinstalled on every release, so force cmake deps. (Means on build failure, everyone will know soon!)
	 */
/*
#if defined HAVE_PKVERSION
	const string releaseStr(TBB_WriterVersion::getVersion());
#else // TODO: test both cases
	ostringstream vss;
	string type("brief");
	LOFAR::Version::show<TBB_WriterVersion>(vss, "TBB_Writer",  type);
	const string releaseStr(vss.str());
#endif
	file.systemVersion().value = versionStr;
*/
	//file.pipelineName().value = ???; // TODO: remove from DAL; for offline/LTA only
	//file.pipelineVersion().value = ???; // idem

	//file.ICDNumber().value = "1"; //TODO: this is wrong if we provide a spec doc, and redundant if we already have 'tbb'; LOFAR-USG-ICD-001: TBB Time-Series Data"); // make it 1 or 001, or drop this, it's in file type (data product type)
	//file.ICDVersion().value = "2.02.15"; // patch number is useless; and this is wrong if we provide a spec doc
	//file.notes().value = ""; // TODO: needed?
	// TODO: No TBB_SysLog group anymore, add it back to CommonLofarAttributes instead of Notes?
	/*if (parset.fakeInputData()) { // ignore checkFakeInputData(), always annotate the data product if data is fake
		syslog.append("input data is fake!");
	}*/
}

/*
 * The generated format is: <Prefix><Observation ID>_<Optional Descriptors>_<Filetype>.<Extension>
 * From LOFAR-USG-ICD005 spec named "LOFAR Data Format ICD File Naming Conventions", by A. Alexov et al.
 * Example for TBB: "L10000_D20120101T031641.123Z_tbb.h5" or "L10000_CS001_%03hhu%03hhu_D20120101T031641.123Z_R%03u_tbb.raw"
 */
static string genOutputFilename(const string& observationId,
		const string& stationName, const string& rsprcu, const string& dateTime, const string& readOut, // <-- may all be empty
		const string& fileType, const string& filenameExtension) {
	ostringstream oss;

	oss << 'L' << observationId;
	if (!stationName.empty()) {
		oss << "_" << stationName;
	}
	if (!rsprcu.empty()) {
		oss << "_" << rsprcu;
	}
	if (!dateTime.empty()) {
		oss << "_" << dateTime;
	}
	if (!readOut.empty()) {
		oss << "_R" << readOut;
	}
	oss << "_" << fileType << "." << filenameExtension;

	return oss.str();
}

/*
 * Return station names that will send data to this host.
 * 
 * This function will be redundant once the mapping info is available through the parset.
 * Multiple stations may be sending to the same locus node (i.e. TBB writer).
 * This mapping is indicated in MAC/Deployment/data/StaticMetaData/TBBConnections.dat
 */
static vector<string> getTBB_StationNames(string& tbbMappingFilename) {
	vector<string> stationNames;

	LOFAR::TBB_StaticMapping tbbMapping(tbbMappingFilename);
	if (tbbMapping.empty()) {
		throw LOFAR::IOException("Failed to derive any node-to-station mappings from the static TBB mapping file");
	}

	string myHname(LOFAR::myHostname(true));
	stationNames = tbbMapping.getStationNames(myHname);
	if (stationNames.empty()) {
		// Likely, it only knows e.g. 'tbbsinknode123' instead of 'tbbsinknode123.example.com', so retry.
		myHname = LOFAR::myHostname(false);
		stationNames = tbbMapping.getStationNames(myHname);
		if (stationNames.empty()) {
			throw LOFAR::IOException("Failed to retrieve station names that will send TBB data to this node");
		}
	}

	return stationNames;
}

static vector<string> getTBB_InputStreamNames(struct progArgs& args) {
	/*
	 * Proto: TBB always arrives over UDP (but command-line override).
	 * 
	 * Ports: Each board sends data to port TBB_DEFAULT_BASE_PORT + itsBoardNr, so that's where put a r/w thread pair each to listen.
	 * The number of TBB boards can be retrieved using LCS/ApplCommon/src/StationConfig.cc: nrTBBs = StationInfo.getInt ("RS.N_TBBOARDS");
	 * but we know that stations have 6 (NL stations) or 12 (EU stations) TBB boards, so use defines for now.
	 */
	vector<string> allInputStreamNames;

	uint16_t portsEnd = args.port + TBB_DEFAULT_LAST_PORT - TBB_DEFAULT_BASE_PORT;
	for (uint16_t port = args.port; port <= portsEnd; port++) {
		string streamName(args.proto + ":0.0.0.0:" + LOFAR::formatString("%hu", port));
		allInputStreamNames.push_back(streamName);
	}

	return allInputStreamNames;
}

static void doThreadedWrites(map<unsigned, LOFAR::RTCP::SmartPtr<LOFAR::RTCP::TBB_StationOut> >& h5Outputs,
				vector<string>& inputStreamNames, LOFAR::RTCP::Parset& parset, struct timeval& timeoutVal) {
	// Mask all signals to inherit for workers. This forces signals to be delivered to the main thread.
	sigset_t sigset_old;
	sigset_t sigset_all_masked;
	sigfillset(&sigset_all_masked);
	int err = pthread_sigmask(SIG_SETMASK, &sigset_all_masked, &sigset_old);
	if (err != 0) {
		throw LOFAR::SystemCallException("pthread_sigmask() to mask sigs failed");
	}

	// Create a TBB_Writer workers for each (possible) input stream.
	string obsIdStr(LOFAR::formatString("%u", parset.observationID()));
	struct timeval tvmax = {0, 0};
	vector<struct timeval> timeoutStamps(inputStreamNames.size(), tvmax);
	vector<LOFAR::RTCP::SmartPtr<LOFAR::RTCP::TBB_Writer> > writers;
	for (size_t i = 0; i < inputStreamNames.size(); i++) {
		string inputStreamName(inputStreamNames[i]);
		const string logPrefix(LOFAR::formatString("[TBB obs %s stream %s]", obsIdStr.c_str(), inputStreamNames[i].c_str()));
		writers.push_back(new LOFAR::RTCP::TBB_Writer(h5Outputs, inputStreamName, parset, timeoutStamps[i], logPrefix));
	}

	// Restore signal mask. Skipped if the above throws, but then we are terminating already.
	if (err == 0) {
		err = pthread_sigmask(SIG_SETMASK, &sigset_old, NULL);
		if (err != 0) {
			throw LOFAR::SystemCallException("pthread_sigmask() to restore sigs failed");
		}
	}

	/*
	 * We don't know how much data comes in, so cancel workers when all are idle for a timeout_sec seconds.
	 * In some situations, threads can become active again after idling a bit.
	 * So periodically monitor thread timeout stamps.
	 * Poor man's sync, but per-thread timers to break read() to notify us of idleness does not work.
	 * This can be improved once the LOFAR system keeps track of TBB dumping and can inform us when to shutdown.
	 */
	struct itimerval timer = {timeoutVal, timeoutVal};
	if (setitimer(ITIMER_REAL, &timer, NULL) != 0) {
		throw LOFAR::SystemCallException("setitimer failed");
	}

	bool anyFrameReceived = false; // don't quit if there is no data immediately after starting
	size_t workersDone;
	do {
		pause();

		workersDone = 0;
		for (size_t i = 0; i < inputStreamNames.size(); i++) {
			struct timeval now;
			gettimeofday(&now, NULL);
			time_t lastActive_sec = timeoutStamps[i].tv_sec; // racy read (and no access once guarantee)
			if (lastActive_sec != 0) {
				anyFrameReceived = true;
			}
			if (anyFrameReceived && lastActive_sec <= now.tv_sec - timeoutVal.tv_sec) {
				workersDone += 1;
			}
		}
	} while (workersDone < inputStreamNames.size());

	// Cancel writer threads and join with them when destructing writers objects.
	for (size_t i = 0; i < writers.size(); i++) {
		writers[i]->cancel();
	}
}

static void printUsage(const char* progname) {
	cout << "Usage: " << progname << " [--inputfile=input_file.raw] [--outputfile=output_file.h5] [--parsetfile=file.parset]"
		" [--conmapfile=TBBConnections.dat] [--protocol=tcp] [--portbase=1234] [--timeout=30] [--keeprunning=1]" << endl;
}

// TODO: remove _VALUE except wcscoords REFERENCE_VALUE (e.g. in ICD003 Table 14, 15).
// TODO: cross-check formats with casacore (see Ger's e-mail)
// TODO: most cerr/cout through logger, some into h5out Syslog
static int parseArgs(int argc, char *argv[], struct progArgs* args) {
	int rv = 0;

	// Default values
	args->inFilename = "";	// default is to read from a socket
	args->outFilename = "";	// default is generated according to ICD005; TODO: if user-specified, we don't have the %03u conv specifiers...
	args->parsetFilename = "";	// default means most meta data fields cannot be set
	args->conMapFilename = "";	// default is to find the filename at $LOFARROOT/etc/StaticMetaData/TBBConnections.dat
	args->proto = "udp";
	args->port = TBB_DEFAULT_BASE_PORT;
	args->timeoutVal.tv_sec = 10; // default secs of inactivity after which to terminate all input workers and close output files
	args->timeoutVal.tv_usec = 0;
	args->keepRunning = false;

	static struct option long_opts[] = {
		// {const char *name, int has_arg, int *flag, int val}
		{"help",        no_argument,       NULL, 'h'},
		{"version",     no_argument,       NULL, 'v'},

		{"inputfile",   required_argument, NULL, 'i'},
		{"outputfile",  required_argument, NULL, 'o'},
		{"parsetfile",  required_argument, NULL, 'c'}, // 'c'onfig
		{"conmapfile",  required_argument, NULL, 'm'}, // 'm'ap
		{"protocol",    required_argument, NULL, 'p'},
		{"portbase",    required_argument, NULL, 'b'}, // 'b'ase
		{"timeout",     required_argument, NULL, 't'},

		{"keeprunning", optional_argument, NULL, 'k'},

		{NULL, 0, NULL, 0}
	};

	opterr = 0; // prevent error printing to stderr by getopt*()
	int opt;
	while ((opt = getopt_long(argc, argv, "hvi:o:c:m:p:b:t:k::", long_opts, NULL)) != -1) {
		switch (opt) {
		case 'h':
		case 'v':
			if (rv == 0) {
				rv = 2;
			}
			break;
		case 'i':
			args->inFilename = optarg;
			break;
		case 'o':
		{
			args->outFilename = optarg;
			string ext(".h5");
			size_t len = args->outFilename.size();
			if (len < 3 || strcasecmp(&optarg[len - ext.size()], ext.c_str()) != 0) {
				args->outFilename.append(ext); // don't error, just correct
			}
			break;
		}
		case 'c':
			args->parsetFilename = optarg;
			break;
		case 'm':
			args->conMapFilename = optarg;
			break;
		case 'p':
			if (strcmp(optarg, "tcp") == 0 || strcmp(optarg, "udp") == 0) {
				args->proto = optarg;
			} else {
				cerr << "Invalid protocol argument: " << optarg << endl;
				rv = 1;
			}
			break;
		case 'b':
			try {
				args->port = boost::lexical_cast<uint16_t>(optarg);
			} catch (boost::bad_lexical_cast& /*exc*/) {
				cerr << "Invalid port argument: " << optarg << endl;
				rv = 1;
			}
			break;
		case 't':
			try {
				args->timeoutVal.tv_sec = boost::lexical_cast<unsigned long>(optarg);
			} catch (boost::bad_lexical_cast& /*exc*/) {
				cerr << "Invalid timeout argument: " << optarg << endl;
				rv = 1;
			}
			break;
		case 'k':
			if (optarg == NULL || optarg[0] == '\0') {
				args->keepRunning = true;
				break;
			}
			try {
				args->keepRunning = boost::lexical_cast<bool>(optarg);
			} catch (boost::bad_lexical_cast& /*exc*/) {
				cerr << "Invalid keeprunning argument: " << optarg << endl;
				rv = 1;
			}
			break;
		default: // '?'
			cerr << "Invalid program argument." << endl; // TODO: print which one; not optarg
			rv = 1;
		}
	}

	if (!args->outFilename.empty() && (
			args->outFilename == args->inFilename ||
			args->outFilename == args->parsetFilename ||
			args->outFilename == args->conMapFilename)) {
		cerr << "Output filename (ending in .h5) cannot be equal to another filename argument." << endl;
		rv = 1;
	}

	if (optind < argc) {
		cerr << "Failed to recognize arguments:";
		while (optind < argc) {
			cerr << " " << argv[optind++];
		}
		cerr << endl;
		rv = 1;
	}

	return rv;
}

int main(int argc, char* argv[]) {
	struct progArgs args;
	int err;

	// Allocate stdout and stderr buffer beforehand and set stderr also to line buffered. We don't use stdin.
	const size_t stdBufferSize = 1024;
	char* stdoutBuffer = new char[stdBufferSize];
	char* stderrBuffer = new char[stdBufferSize];
	setvbuf(stdout, stdoutBuffer, _IOLBF, stdBufferSize);
	setvbuf(stderr, stderrBuffer, _IOLBF, stdBufferSize);

#if defined HAVE_LOG4CPLUS
	INIT_LOGGER(string(getenv("LOFARROOT") ? : ".") + "/etc/Storage_main.log_prop");
#elif defined HAVE_LOG4CXX
#error LOG4CXX support is broken (nonsensical?) -- please fix this code if you want to use it
	Context::initialize();
	setLevel("Global", 8);
#else
	INIT_LOGGER_WITH_SYSINFO(str(boost::format("Storage@%02d") % (argc > 1 ? atoi(argv[1]) : -1))); // TODO: no boost fmt or everywhere; not like this
#endif

	err = parseArgs(argc, argv, &args);
	if (err != 0) {
		if (err == 2) err = 0; // just asking for help is not an error
		printUsage(argv[0]);
		return err;
	}

	setTermSigsHandler();

	// We don't run alone, so increase the QoS we get from the OS to decrease the chance of data loss.
	setIOpriority();
	setRTpriority();
	lockInMemory();

	unsigned run_nr = 0;
	do {
		err = 0;
		run_nr += 1; // mangle into filenames to avoid accidental overwriting with keeprunning

	try {
		/*
		 * Retrieve the station name that will send data to this host (input thread will check this at least once).
		 * Getting this from static meta data is inflexible.
		 */
		string tbbMappingFilename(args.conMapFilename);
		if (tbbMappingFilename.empty()) {
			const string defaultTbbMappingFilename("TBBConnections.dat");
			char* lrpath = getenv("LOFARROOT");
			if (lrpath != NULL) {
				tbbMappingFilename = string(lrpath) + "/etc/StaticMetaData/";
			}
			tbbMappingFilename.append(defaultTbbMappingFilename);
		}
		vector<string> stationNames(getTBB_StationNames(tbbMappingFilename));

		vector<string> inputStreamNames(getTBB_InputStreamNames(args));
		LOFAR::RTCP::Parset parset(args.parsetFilename);

		// Generate the output filename ourselve (if not from command-line), because for TBB it's not in the parset.
		string obsIdStr(LOFAR::formatString("%u", parset.observationID()));

		struct timeval tv;
		gettimeofday(&tv, NULL); // TODO: must be time of 1st frame arrival
		const char output_format[] = "D%Y%m%dT%H%M"; // without secs
		const char output_format_secs[] = "%06.3fZ";
		const char output_format_example[] = "DYYYYMMDDTHHMMSS.SSSZ";
		string triggerDateTime(formatFilenameTimestamp(tv, output_format, output_format_secs, sizeof(output_format_example)));

		string runNrStr(LOFAR::formatString("%03u", run_nr));
		string h5FilenameFmt(genOutputFilename(obsIdStr, "%s", "", triggerDateTime, runNrStr, "tbb", "h5" ));

		/*
		 * The converters receive a stationId in the incoming datagrams, so create an HDF5 output per station
		 * and map from stationId to the TBB_StationOut struct.
		 */
		map<unsigned, LOFAR::RTCP::SmartPtr<LOFAR::RTCP::TBB_StationOut> > h5Outputs;
		for (unsigned i = 0; i < stationNames.size(); i++) {
			string h5Filename(LOFAR::formatString(h5FilenameFmt.c_str(), stationNames[i].c_str()));

			// Fully regenerate raw fmt every time, because we cannot have snprintf()/formatString() _only_ treat the station name conv.
			// The rsp/rcu are for the workers to fill in.
			string rawFilenameStationFmt(genOutputFilename(obsIdStr, stationNames[i], "%03hhu%03hhu", triggerDateTime, runNrStr, "tbb", "raw"));
			LOFAR::RTCP::SmartPtr<LOFAR::RTCP::TBB_StationOut> stOut = new LOFAR::RTCP::TBB_StationOut(h5Filename, rawFilenameStationFmt);

			// Don't need to grab the mutex, because we are the main thread initializing before we create output threads.
			setCommonLofarAttributes(stOut->h5Out, parset, h5Filename);
			setTBB_RootAttributes(stOut->h5Out, parset, stationNames[i]);
			stOut->h5Out.flush();

			unsigned stationId = stationNameToId(stationNames[i]);
			h5Outputs.insert(make_pair(stationId, stOut));
		}

		LOG_INFO_STR("Expecting from " << stationNames.size() << " station(s); about to receive from "
					<< inputStreamNames.size() << " input stream(s)");

		doThreadedWrites(h5Outputs, inputStreamNames, parset, args.timeoutVal);

	} catch (LOFAR::Exception& exc) {
		LOG_FATAL_STR(exc.text());
		err = 1;
	} catch (DAL::DALException& exc) {
		LOG_FATAL_STR("[obs unknown] Caught DAL::DALException: " << exc.what()); // TODO: fix obs unknown
		err = 1;
	} catch (exception& exc) {
		LOG_FATAL_STR("[obs unknown] Caught std::exception: " << exc.what());
		err = 1;
	} catch (...) {
		LOG_FATAL("[obs unknown] Caught non-std::exception");
		err = 1;
	}

	} while (args.keepRunning);

	fclose(stderr);
	fclose(stdout);
	delete[] stderrBuffer;
	delete[] stdoutBuffer;

	return err;
}
