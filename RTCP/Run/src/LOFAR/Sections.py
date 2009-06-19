#!/usr/bin/env python

from util.Commands import SyncCommand,AsyncCommand,mpikill
from Locations import Locations
import os

DEBUG=False

def debug( str ):
  if DEBUG:
    print "%s" % (str,)

class Section:
  """ A 'section' is a set of commands which together perform a certain function. """

  def __init__(self, parset):
    self.parset = parset
    self.commands = []

  def __str__(self):
    return self.__class__.__name__

  def run(self):
    pass

  def postProcess(self):
    pass

  def abort(self,soft=True):
    for c in self.commands:
      c.abort([9,2][bool(soft)])

  def wait(self):
    for c in self.commands:
      c.wait()

class SectionSet(list):
  def run(self):
    for s in self:
      debug( "Starting %s." % (s,) )
      s.run()

  def postProcess(self):
    for s in self:
      debug( "Post processing %s." % (s,) )
      s.postProcess()

  def abort(self,soft=True):
    for s in self:
      debug( "Killing %s [%s]." % (s,["hard","soft"][bool(soft)]) )
      s.abort(soft)

    self.wait()

  def wait(self):
    for s in self:
      debug( "Waiting for %s." % (s,) )
      s.wait()

class CNProcSection(Section):
  def run(self):
    logfile = "%s/run.CNProc.%s.log" % (Locations.files["logdir"],self.parset.partition)

    # CNProc is started on the Blue Gene, which has BG/P mpirun 1.65
    # NOTE: This mpirun needs either stdin or stdout to be line buffered,
    # otherwise it will not provide output from CN_Processing (why?).
    mpiparams = [
     # where
      "-mode VN",
      "-partition %s" % (self.parset.partition,),

      # environment
      "-env DCMF_COLLECTIVES=0",
      "-env BG_MAPPING=XYZT",
      "-env LD_LIBRARY_PATH=/bgsys/drivers/ppcfloor/comm/lib:/bgsys/drivers/ppcfloor/runtime/SPI:/cephome/romein/lib.bgp",

      # working directory
      "-cwd %s" % (Locations.files["rundir"],),

      # executable
      "-exe %s" % (Locations.files["cnproc"],),

      # arguments
      "-args %s" % (Locations.files["parset"],),
    ]

    self.commands.append( AsyncCommand( "mpirun %s" % (" ".join(mpiparams),), logfile, killcmd=mpikill ) )

class IONProcSection(Section):
  def run(self):
    ionodes = self.parset.psets

    # change working directory to the rundir, and execute IONProc
    cmd = "bash -c '(cd %s; %s %s)'" % (Locations.files["rundir"],Locations.files["ionproc"],Locations.files["parset"])

    for node in ionodes:
      nodenr = node.split(".")[-1]
      logfile = "%s/run.IONProc.%s.%s" % (Locations.files["logdir"],self.parset.partition,nodenr)

      self.commands.append( AsyncCommand( "ssh %s %s" % (node,cmd,), logfile ) )

class StorageSection(Section):
  def run(self):
    self.logfile = "%s/run.Storage.%s.log" % (Locations.files["logdir"],self.parset.partition)

    # the PID of mpirun
    self.pidfile = "%s/Storage-%s.pid" % (Locations.files["rundir"],self.parset.partition)

    # unique identifier to locate the mpi processes
    self.universe = "OLAP-%s" % (self.parset.getObsID(),)

    # create the target directories
    for n in Locations.nodes["storage"]:
      self.commands.append( SyncCommand( "ssh %s mkdir %s" % (n,os.path.dirname(self.parset.parseMask()),), self.logfile ) )

    # Storage is started on LIIFEN, which has mpirun (Open MPI) 1.1.1
    mpiparams = [
      # provide this run with an unique name to identify the corresponding
      # processes on the storage nodes
      "-universe %s" % (self.universe,),

      # where
      "-host %s" % (",".join(Locations.nodes["storage"]),),

      "-np %s" % (len(Locations.nodes["storage"]),),

      # environment

      # working directory
      "-wdir %s" % (Locations.files["rundir"],),

      # executable
      "%s" % (Locations.files["storage"],),

      # arguments
      "%s" % (Locations.files["parset"],),
    ]

    self.commands.append( AsyncCommand( "ssh %s echo $$ > %s;exec mpirun %s" % (Locations.nodes["storagemaster"],self.pidfile," ".join(mpiparams),), self.logfile ) )

  def abort( self, soft = True ):
    signal = [9,2][bool(soft)]

    # kill mpirun using the pid we stored locally
    SyncCommand( "ssh %s kill -%s `cat %s`" % (Locations.nodes["storagemaster"],signal,self.pidfile) )

    # kill Storage and orted processes on storage nodes
    for node in Locations.nodes["storage"]:
      # We kill the process group rooted at the orted process
      # (kill -PID) belonging to our MPI universe. This takes Storage with it.
      SyncCommand( "ssh %s kill -%s -`ps --no-heading -o pid,cmd -ww -C orted | grep %s | awk '{ print $1; }'`" % (
          node, signal, self.universe) )

