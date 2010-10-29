//#  Job.cc:
//#
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//#  This program is free software; you can redistribute it and/or modify
//#  it under the terms of the GNU General Public License as published by
//#  the Free Software Foundation; either version 2 of the License, or
//#  (at your option) any later version.
//#
//#  This program is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#  GNU General Public License for more details.
//#
//#  You should have received a copy of the GNU General Public License
//#  along with this program; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//#  $Id: ION_main.cc 15296 2010-03-24 10:19:41Z romein $

#include <lofar_config.h>

#include <BeamletBufferToComputeNode.h>
#include <Common/LofarLogger.h>
#include <Interface/CN_Command.h>
#include <Interface/CN_Configuration.h>
#include <Interface/CN_ProcessingPlan.h>
#include <Interface/Exceptions.h>
#include <Interface/PrintVector.h>
#include <Interface/RSPTimeStamp.h>
#include <InputSection.h>
#include <ION_main.h>
#include <Job.h>
#include <OutputSection.h>
#include <StreamMultiplexer.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/format.hpp>
using boost::format;

namespace LOFAR {
namespace RTCP {

unsigned Job::nextJobID = 1;
void	 *Job::theInputSection;
Mutex	 Job::theInputSectionMutex;
unsigned Job::theInputSectionRefCount = 0;


Job::Job(const char *parsetName)
:
  itsParset(parsetName),
  itsJobID(nextJobID ++), // no need to make thread safe
  itsObservationID(itsParset.observationID()),
  itsIsRunning(false),
  itsDoCancel(false)
{
  itsLogPrefix = str(format("[obs %d] ") % itsParset.observationID());

  checkParset();

  LOG_INFO_STR(itsLogPrefix << "----- Creating new observation");
  LOG_DEBUG_STR(itsLogPrefix << "usedCoresInPset = " << itsParset.usedCoresInPset());

  itsNrRuns = static_cast<unsigned>(ceil((itsParset.stopTime() - itsParset.startTime()) / itsParset.CNintegrationTime()));
  LOG_DEBUG_STR(itsLogPrefix << "itsNrRuns = " << itsNrRuns);

  // synchronize roughly every 5 seconds to see if the job is cancelled
  itsNrRunTokensPerBroadcast = static_cast<unsigned>(ceil(5.0 / itsParset.CNintegrationTime()));
  itsNrRunTokens	     = itsNrRunTokensPerBroadcast;

  itsHasPhaseOne   = itsParset.phaseOnePsetIndex(myPsetNumber) >= 0;
  itsHasPhaseTwo   = itsParset.phaseTwoPsetIndex(myPsetNumber) >= 0;
  itsHasPhaseThree = itsParset.phaseThreePsetIndex(myPsetNumber) >= 0;

  itsJobThread = new Thread(this, &Job::jobThread, itsLogPrefix + "[JobThread] ", 65536);
}


Job::~Job()
{
  delete itsJobThread;
  jobQueue.remove(this);

  LOG_INFO_STR(itsLogPrefix << "----- Observation " << (itsIsRunning ? "ended" : "cancelled") << " successfully");
}


void Job::createIONstreams()
{
  if (myPsetNumber == 0) {
    std::vector<unsigned> involvedPsets = itsParset.usedPsets();

    for (unsigned i = 0; i < involvedPsets.size(); i ++)
      if (involvedPsets[i] != 0) // do not send to itself
	itsIONstreams.push_back(new MultiplexedStream(*allIONstreamMultiplexers[involvedPsets[i]], itsJobID));
  } else {
    itsIONstreams.push_back(new MultiplexedStream(*allIONstreamMultiplexers[0], itsJobID));
  }
}


void Job::deleteIONstreams()
{
  if (myPsetNumber == 0)
    for (unsigned i = 0; i < itsIONstreams.size(); i ++)
      delete itsIONstreams[i];
  else
    delete itsIONstreams[0];

  itsIONstreams.clear();
}


void Job::barrier()
{
  char byte = 0;

  if (myPsetNumber == 0) {
    for (unsigned i = 0; i < itsIONstreams.size(); i ++) {
      itsIONstreams[i]->read(&byte, sizeof byte);
      itsIONstreams[i]->write(&byte, sizeof byte);
    }
  } else {
    itsIONstreams[0]->write(&byte, sizeof byte);
    itsIONstreams[0]->read(&byte, sizeof byte);
  }
}


template <typename T> void Job::broadcast(T &value)
{
  if (myPsetNumber == 0)
    for (unsigned i = 0; i < itsIONstreams.size(); i ++)
      itsIONstreams[i]->write(&value, sizeof value);
  else
    itsIONstreams[0]->read(&value, sizeof value);
}


void Job::execSSH(const char *sshKey, const char *userName, const char *hostName, const char *executable, const char *rank, const char *parset, const char *isBigEndian)
{
  // DO NOT DO ANY CALL THAT GRABS A LOCK, since the lock may be held by a
  // thread that is no longer part of our address space

  // create a blocking stdin pipe
  // rationale: this forked process inherits stdin from the parent process, which is unusable because IONProc is started in the background
  // and routed through mpirun as well. Also, it is shared by all forked processes. Nevertheless, we want Storage to be able to determine
  // when to shut down based on whether stdin is open. So we create a new stdin. Even though the pipe we create below will block since there
  // will never be anything to read, closing it will propagate to Storage and that's enough.
  int pipefd[2];
  int ignoreResult = pipe(pipefd);

  ignoreResult = close(0);
  ignoreResult = dup(pipefd[0]);

  char cwd[1024];
  getcwd( cwd, sizeof cwd );

  execl("/usr/bin/ssh",
    "ssh",
    "-i", sshKey,
    "-c", "blowfish",
    "-o", "StrictHostKeyChecking=no",
    "-o", "UserKnownHostsFile=/dev/null",
    "-o", "ServerAliveInterval=30",
    "-l", userName,
    hostName,

    "cd", cwd, "&&",

    executable, rank, parset, isBigEndian,

    static_cast<char *>(0)
  );

  ignoreResult = write(2, "exec failed\n", 12); // Logger uses mutex, hence write directly
  exit(1);
}


void Job::forkSSH(const char *sshKey, const char *userName, const char *hostName, const char *executable, const char *rank, const char *parset, const char *isBigEndian, int &storagePID)
{
  LOG_INFO_STR("child will exec("
    "\"/usr/bin/ssh\", "
    "\"ssh\", "
    "\"-i\", \"" << sshKey << "\", "
    "\"-c\", \"blowfish\", "
    "\"-o\", \"StrictHostKeyChecking=no\", "
    "\"-o\", \"UserKnownHostsFile=/dev/null\", "
    "\"-o\", \"ServerAliveInterval=30\", "
    "\"-l\", \"" << userName << "\", "
    "\"" << hostName << "\", "
    "\"cd\", (cwd), \"&&\", " 
    "\"" << executable << "\", "
    "\"" << rank << "\", "
    "\"" << parset << "\", "
    "\"" << isBigEndian << "\", "
    "0)"
  );

  switch (storagePID = fork()) {
    case -1 : throw SystemCallException("fork", errno, THROW_ARGS);

    case  0 : execSSH(sshKey, userName, hostName, executable, rank, parset, isBigEndian);
  }
}


void Job::joinSSH(int childPID, const std::string &hostName, unsigned &timeout)
{
  if (childPID != 0) {
    int status;

    // always try at least one waitpid(). if child has not exited, optionally
    // sleep and try again.
    for (;;) {
      pid_t ret;

      if ((ret = waitpid(childPID, &status, WNOHANG)) == (pid_t)-1) {
        int error = errno;

        if (error == EINTR) {
          LOG_DEBUG_STR(itsLogPrefix << "Storage writer on " << hostName << " : waitpid() was interrupted -- retrying");
          continue;
        }

        // error
        LOG_WARN_STR(itsLogPrefix << "Storage writer on " << hostName << " : waitpid() failed with errno " << error);
        return;
      } else if (ret == 0) {
        // child still running
        if (timeout == 0) {
          break;
        }

        timeout--;
        sleep(1);
      } else {
        // child exited
        if (WIFSIGNALED(status) != 0)
          LOG_WARN_STR(itsLogPrefix << "Storage writer on " << hostName << " was killed by signal " << WTERMSIG(status));
        else if (WEXITSTATUS(status) != 0)
          LOG_WARN_STR(itsLogPrefix << "Storage writer on " << hostName << " exited with exit code " << WEXITSTATUS(status));
        else
          LOG_INFO_STR(itsLogPrefix << "Storage writer on " << hostName << " terminated normally");

        return;  
      }
    }

    // child did not exit within the given timeout

    LOG_WARN_STR(itsLogPrefix << "Storage writer on " << hostName << " : sending SIGTERM");
    kill(childPID, SIGTERM);

    if (waitpid(childPID, &status, 0) == -1) {
      LOG_WARN_STR(itsLogPrefix << "Storage writer on " << hostName << " : waitpid() failed");
    }

    LOG_WARN_STR(itsLogPrefix << "Storage writer on " << hostName << " terminated after sending SIGTERM");
  }
}


void Job::startStorageProcesses()
{
  //itsStorageHostNames = itsParset.getStringVector("Observation.VirtualInstrument.storageNodeList");
// use IP addresses, since the I/O node cannot resolve host names
  itsStorageHostNames = itsParset.getStringVector("OLAP.OLAP_Conn.IONProc_Storage_ServerHosts");

  std::string userName   = itsParset.getString("OLAP.Storage.userName");
  std::string sshKey     = itsParset.getString("OLAP.Storage.sshIdentityFile");
  std::string executable = itsParset.getString("OLAP.Storage.msWriter");
  std::string parset     = itsParset.name();

  itsStoragePIDs.resize(itsStorageHostNames.size());

  for (unsigned rank = 0; rank < itsStorageHostNames.size(); rank ++)
    forkSSH(sshKey.c_str(),
	    userName.c_str(),
	    itsStorageHostNames[rank].c_str(),
	    executable.c_str(),
	    boost::lexical_cast<std::string>(rank).c_str(),
	    parset.c_str(),
#if defined WORDS_BIGENDIAN
	    "1",
#else
	    "0",
#endif
	    itsStoragePIDs[rank]);
}


void Job::stopStorageProcesses()
{
  unsigned timeleft = 10;

  for (unsigned rank = 0; rank < itsStorageHostNames.size(); rank ++)
    joinSSH(itsStoragePIDs[rank], itsStorageHostNames[rank], timeleft);
}


void Job::waitUntilCloseToStartOfObservation(time_t secondsPriorToStart)
{
  time_t closeToStart = static_cast<time_t>(itsParset.startTime()) - secondsPriorToStart;
  char   buf[26];

  ctime_r(&closeToStart, buf);
  buf[24] = '\0';
  
  LOG_DEBUG_STR(itsLogPrefix << "Waiting for job to start: sleeping until " << buf);

  itsWallClockTime.waitUntil(closeToStart);
}


void Job::cancel()
{
  // note that JobQueue holds lock, so that this function executes atomically

  if (itsDoCancel) {
    LOG_WARN_STR(itsLogPrefix << "Observation already cancelled");
  } else {
    itsDoCancel = true;
    jobQueue.itsReevaluate.broadcast();

    if (itsParset.realTime())
      itsWallClockTime.cancelWait();
  }
}


void Job::claimResources()
{
  ScopedLock scopedLock(jobQueue.itsMutex);

  while (!itsDoCancel) {
    bool conflict = false;

    for (std::vector<Job *>::iterator job = jobQueue.itsJobs.begin(); job != jobQueue.itsJobs.end(); job ++) {
      std::stringstream error;

      if ((*job)->itsIsRunning && (*job)->itsParset.conflictingResources(itsParset, error)) {
	conflict = true;
	LOG_WARN_STR(itsLogPrefix << "Postponed due to resource conflict with job " << (*job)->itsObservationID << ": " << error.str());
      }
    }

    if (!conflict) {
      itsIsRunning = true;
      return;
    }

    jobQueue.itsReevaluate.wait(jobQueue.itsMutex);
  }
}


void Job::jobThread()
{
  if (myPsetNumber == 0 || itsHasPhaseOne || itsHasPhaseTwo || itsHasPhaseThree) {
    createCNstreams();
    createIONstreams();

    bool storageStarted = false;

    if (myPsetNumber == 0) {
      if (itsParset.realTime())
	waitUntilCloseToStartOfObservation(10);

      claimResources();

      if (itsIsRunning && itsParset.hasStorage()) {
	startStorageProcesses();
	storageStarted = true;
      }
    }

    broadcast(itsIsRunning);

    if (itsIsRunning) {
      if (itsHasPhaseOne || itsHasPhaseTwo || itsHasPhaseThree)
	switch (itsParset.nrBitsPerSample()) {
	  case  4 : doObservation<i4complex>();
		    break;

	  case  8 : doObservation<i8complex>();
		    break;

	  case 16 : doObservation<i16complex>();
		    break;
	}
      else // force pset 0 to broadcast itsIsRunning periodically
	for (unsigned i = 0; i < itsNrRuns && !isCancelled(); i ++)
	  ;

      barrier();

      // all InputSections and OutputSections have finished their processing, so
      // Storage should be done any second now.

      if (storageStarted)
	stopStorageProcesses();
    }

    deleteIONstreams();
  }

  delete this;
}


void Job::createCNstreams()
{
  std::vector<unsigned> usedCoresInPset = itsParset.usedCoresInPset();

  itsCNstreams.resize(usedCoresInPset.size());

  for (unsigned core = 0; core < usedCoresInPset.size(); core ++)
    itsCNstreams[core] = allCNstreams[usedCoresInPset[core]];

  if (itsHasPhaseOne || itsHasPhaseTwo) {
    std::vector<unsigned> phaseOneTwoCores = itsParset.phaseOneTwoCores();

    itsPhaseOneTwoCNstreams.resize(phaseOneTwoCores.size());

    for (unsigned core = 0; core < phaseOneTwoCores.size(); core ++)
      itsPhaseOneTwoCNstreams[core] = allCNstreams[phaseOneTwoCores[core]];
  }

  if (itsHasPhaseThree) {
    std::vector<unsigned> phaseThreeCores = itsParset.phaseThreeCores();

    itsPhaseThreeCNstreams.resize(phaseThreeCores.size());

    for (unsigned core = 0; core < phaseThreeCores.size(); core ++)
      itsPhaseThreeCNstreams[core] = allCNstreams[phaseThreeCores[core]];
  }
}


template <typename SAMPLE_TYPE> void Job::attachToInputSection()
{
  ScopedLock scopedLock(theInputSectionMutex);

  if (theInputSectionRefCount == 0) {
    theInputSection = new InputSection<SAMPLE_TYPE>(itsParset, myPsetNumber);
    ++ theInputSectionRefCount;
  }
}


template <typename SAMPLE_TYPE> void Job::detachFromInputSection()
{
  ScopedLock scopedLock(theInputSectionMutex);

  if (-- theInputSectionRefCount == 0)
    delete static_cast<InputSection<SAMPLE_TYPE> *>(theInputSection);
}


bool Job::configureCNs()
{
  bool success = true;

  CN_Command	   command(CN_Command::PREPROCESS);
  CN_Configuration configuration(itsParset);
  
  LOG_DEBUG_STR(itsLogPrefix << "Configuring cores " << itsParset.usedCoresInPset() << " ...");

  for (unsigned core = 0; core < itsCNstreams.size(); core ++) {
    command.write(itsCNstreams[core]);
    configuration.write(itsCNstreams[core]);
  }

  for (unsigned core = 0; core < itsCNstreams.size(); core ++) {
    char failed;
    itsCNstreams[core]->read(&failed, sizeof failed);

    if (failed) {
      LOG_ERROR_STR(itsLogPrefix << "Core " << core << " failed to initialise");
      success = false;
    }
  }
  
  LOG_DEBUG_STR(itsLogPrefix << "Configuring cores " << itsParset.usedCoresInPset() << " done");

  return success;
}


void Job::unconfigureCNs()
{
  CN_Command command(CN_Command::POSTPROCESS);

  LOG_DEBUG_STR(itsLogPrefix << "Unconfiguring cores " << itsParset.usedCoresInPset() << " ...");

  for (unsigned core = 0; core < itsCNstreams.size(); core ++)
    command.write(itsCNstreams[core]);

  LOG_DEBUG_STR(itsLogPrefix << "Unconfiguring cores " << itsParset.usedCoresInPset() << " done");
}


bool Job::isCancelled()
{
  if (-- itsNrRunTokens == 0) {
    itsNrRunTokens = itsNrRunTokensPerBroadcast;
    itsIsRunning   = !itsDoCancel;
    broadcast(itsIsRunning);
  }

  return !itsIsRunning;
}


template <typename SAMPLE_TYPE> void Job::doObservation()
{
  CN_Configuration configuration(itsParset);
  CN_ProcessingPlan<> plan(configuration);
  plan.removeNonOutputs();
  unsigned nrOutputTypes = plan.nrOutputTypes();
  std::vector<OutputSection *> outputSections(nrOutputTypes, 0);

  LOG_INFO_STR(itsLogPrefix << "----- Observation start");

  // first: send configuration to compute nodes so they know what to expect
  if (!configureCNs()) {
    unconfigureCNs();

    LOG_INFO_STR(itsLogPrefix << "----- Observation finished");
    return;
  }

  if (itsHasPhaseOne)
    attachToInputSection<SAMPLE_TYPE>();

  // start output process threads
  for (unsigned output = 0; output < nrOutputTypes; output ++) {
    ProcessingPlan::planlet &p = plan.plan[output];

    unsigned phase, maxlistsize;
    int psetIndex;
    std::vector<unsigned> list; // list of subbands or beams
    std::vector<unsigned> cores;
    std::string type;

    unsigned nrsubbeams = 0;

    if (itsParset.outputBeamFormedData())
      nrsubbeams = NR_POLARIZATIONS;
    else if (itsParset.outputCoherentStokes())
      nrsubbeams = itsParset.nrStokes();

    unsigned nrbeams = (itsParset.flysEye() ? itsParset.nrMergedStations() : itsParset.nrPencilBeams()) * nrsubbeams;

    switch (p.distribution) {
      case ProcessingPlan::DIST_SUBBAND:
        phase = 2;
        type = "subbands";
        cores = itsParset.phaseOneTwoCores();
        psetIndex = itsParset.phaseTwoPsetIndex(myPsetNumber);

        if (psetIndex < 0) {
          // this pset does not participate for this output
          continue;
        }

        maxlistsize = itsParset.nrSubbandsPerPset();

        for (unsigned sb = 0; sb < itsParset.nrSubbandsPerPset(); sb ++) {
          unsigned subbandNumber = psetIndex * itsParset.nrSubbandsPerPset() + sb;

          if (subbandNumber < itsParset.nrSubbands())
            list.push_back(subbandNumber);
        }

        break;

      case ProcessingPlan::DIST_BEAM:
        phase = 3;
        type = "beams";
        cores = itsParset.phaseThreeCores();
        psetIndex = itsParset.phaseThreePsetIndex(myPsetNumber);

        if (psetIndex < 0) {
          // this pset does not participate for this output
          continue;
        }

        if (itsParset.phaseThreeDisjunct()) {
          // simplification: each core produces at most 1 beam
          assert( itsParset.nrBeamsPerPset() <= itsParset.phaseThreeCores().size() );

          maxlistsize = itsParset.nrBeamsPerPset();
        } else {
          // simplification: each core produces at most 1 beam
          assert( itsParset.nrBeamsPerPset() <= itsParset.nrSubbandsPerPset() );

          // simplification: also each core which processes a beam also processes a subband
          assert( nrbeams <= itsParset.nrSubbands() );

          maxlistsize = itsParset.nrSubbandsPerPset();
        }

        for (unsigned beam = 0;  beam < itsParset.nrBeamsPerPset(); beam ++) {
          unsigned beamNumber = psetIndex * itsParset.nrBeamsPerPset() + beam;

          if (beamNumber < nrbeams)
            list.push_back(beamNumber);
        }

        break;

      default:
        continue;
    }

    LOG_DEBUG_STR(itsLogPrefix << "Setting up output " << p.outputNr << " (" << p.name << ") for " << type << " " << list);

    outputSections[output] = new OutputSection(itsParset, cores, list, maxlistsize, p, &createCNstream);
  }

  LOG_DEBUG_STR(itsLogPrefix << "doObservation processing input start");

  { // separate scope to ensure that the beamletbuffertocomputenode objects
    // only exist if the beamletbuffers exist in the inputsection
    unsigned				    run;
    std::vector<BeamletBuffer<SAMPLE_TYPE> *> noInputs;
    BeamletBufferToComputeNode<SAMPLE_TYPE>   beamletBufferToComputeNode(&itsParset, itsPhaseOneTwoCNstreams, itsPhaseThreeCNstreams, itsHasPhaseOne ? static_cast<InputSection<SAMPLE_TYPE> *>(theInputSection)->itsBeamletBuffers : noInputs, myPsetNumber);

    for (run = 0; run < itsNrRuns && !isCancelled(); run ++) {
      for (unsigned output = 0; output < nrOutputTypes; output ++) {
        outputSections[output]->addIterations( 1 );
      }

      beamletBufferToComputeNode.process();
    }
  }

  LOG_DEBUG_STR(itsLogPrefix << "doObservation processing input done");

  for (unsigned output = 0; output < nrOutputTypes; output ++)
    outputSections[output]->noMoreIterations();

  for (unsigned output = 0; output < nrOutputTypes; output ++)
    delete outputSections[output];

  if (itsHasPhaseOne)
    detachFromInputSection<SAMPLE_TYPE>();

  unconfigureCNs();

  LOG_INFO_STR(itsLogPrefix << "----- Observation finished");
}


void Job::checkParset() const
{
  if (itsParset.nrCoresPerPset() > nrCNcoresInPset) {
    LOG_ERROR_STR(itsLogPrefix << "nrCoresPerPset (" << itsParset.nrCoresPerPset() << ") cannot exceed " << nrCNcoresInPset);
    exit(1);
  }
}


void Job::printInfo() const
{
  LOG_INFO_STR(itsLogPrefix << "JobID = " << itsJobID << ", " << (itsIsRunning ? "running" : "not running"));
}


} // namespace RTCP
} // namespace LOFAR
