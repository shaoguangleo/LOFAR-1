# $Id$

include(LofarPackageVersion)

lofar_add_library(lmwcommon
  Package__Version.cc
  ClusterDesc.cc
  ControllerBase.cc
  DomainShape.cc
  MPIConnection.cc
  MPIConnectionSet.cc
  MWBlobIO.cc
  MWConnection.cc
  MWConnectionSet.cc
  MWGlobalStep.cc
  MWIos.cc
  MWLocalStep.cc
  MWMultiStep.cc
  MWStep.cc
  MWStepFactory.cc
  MWStepVisitor.cc
  MasterControl.cc
  MemConnection.cc
  MemConnectionSet.cc
  NodeDesc.cc
  ObsDomain.cc
  ParameterHandler.cc
  SocketConnection.cc
  SocketConnectionSet.cc
  SocketListener.cc
  VdsDesc.cc
  VdsPartDesc.cc
  WorkDomainSpec.cc
  WorkerControl.cc
  WorkerFactory.cc
  WorkerInfo.cc
  WorkerProxy.cc
  WorkersDesc.cc)

lofar_add_bin_program(versionlmwcommon versionlmwcommon.cc)
lofar_add_bin_program(finddproc finddproc.cc)

lofar_add_bin_scripts(
  cexecms
  cexecms-part
  expandparameter
  rundist
  rundist-part
  startdistproc
  socketrun
  runlogpid)
