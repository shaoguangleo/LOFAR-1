//# TH_ShMem.cc: In-memory transport mechanism
//#
//# Copyright (C) 2000, 2001
//# ASTRON (Netherlands Foundation for Research in Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This program is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//# $Id$


#include <Transport/TH_ShMem.h>

#ifdef HAVE_MPI

#include <Transport/BaseSim.h>
#include <Transport/Transporter.h>
#include <Transport/DataHolder.h>
#include <Common/BlobStringType.h>
#include <Common/Debug.h>
#include <Common/shmem/shmem_alloc.h>
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sched.h>

namespace LOFAR
{

// use semaphore in shared memory to synchronize the sender and receiver
#define SEM_SYNC

#define TH_SHMEM_MAGIC_COOKIE 0xfefabade

// magic cookie for TH_ShMem communication acknowledgement
#define TH_SHMEM_ACK_MAGIC       777
#define TH_SHMEM_SENDREADY_MAGIC 345

// The hostNames array contains the hostnames of
// all the ranks in the MPI_COMM_WORLD group.
// This is needed to be able to detect when two ranks
// are on the same host so they can use the ShMem
// primitives to communicate efficiently.
#define TH_SHMEM_MAX_HOSTNAME_SIZE 256
char* TH_ShMem::hostNames = 0;


TH_ShMem::TH_ShMem(int sourceNode, int targetNode)
  : itsSourceNode(sourceNode),
    itsTargetNode(targetNode),
    itsFirstCall (true),
    itsSendBuf   (0),
    itsRecvBuf   (0)
{
}

TH_ShMem::~TH_ShMem()
{
  // Disconnect if itsSendBuf was created using connect in recv,
  // which is the case if there is also a itsRecvBuf.
  if (itsSendBuf && itsRecvBuf)
  {
    // disconnect from shared memory
    TRACER2 ("shmem disconnect " << itsSendBuf << ' '
	     << itsRecvContext.handle.getOffset());
    shmem_disconnect(itsSendBuf,
		     itsRecvContext.handle.getOffset());
  }
}

TH_ShMem* TH_ShMem::make() const
{
  return new TH_ShMem(itsSourceNode, itsTargetNode);
}

string TH_ShMem::getType() const
{
  return "TH_ShMem";
}

BlobStringType TH_ShMem::blobStringType() const
{
  return BlobStringType (false, ShMemAllocator());
}

bool TH_ShMem::canDataGrow() const
{
  return false;
}

TH_ShMem::ShMemAllocator::~ShMemAllocator()
{}

TH_ShMem::ShMemAllocator* TH_ShMem::ShMemAllocator::clone() const
{
  return new TH_ShMem::ShMemAllocator();
}

void* TH_ShMem::ShMemAllocator::allocate(size_t size)
{
  ShMemBuf* buf = 0;

  // allocate buffer header plus data space
  buf = (TH_ShMem::ShMemBuf*)shmem_malloc(sizeof(TH_ShMem::ShMemBuf) + size);

  TRACER2 (TH_ShMem::getCurrentRank() << " alloc " << (void*)buf
	   << ' ' << size);
  // initialize header
  buf->init();

  // return address of first buffer element
  void* ptr = buf->getDataAddress();
  TRACER2 (TH_ShMem::getCurrentRank() << " alloc ptr " << ptr);
  return buf->getDataAddress();
}

void TH_ShMem::ShMemAllocator::deallocate(void* ptr)
{
  if (ptr) {
    void* buf = TH_ShMem::ShMemBuf::toBuf(ptr);
    TRACER2 ("shmem dealloc " << ptr << ' ' << buf);
    shmem_free(buf);
  }
}


bool TH_ShMem::connectionPossible(int srcRank, int dstRank) const
{
  cdebug(3) << "TH_ShMem::connectionPossible between " 
	    << srcRank << " and " 
	    << dstRank << "?" << endl;

  AssertStr(   srcRank >= 0
	    && dstRank >= 0
	    && srcRank < getNumberOfNodes()
	    && dstRank < getNumberOfNodes(), "srcRank or dstRank invalid");

  AssertStr(0 != hostNames, "required call to TH_ShMem::init missing");

  // return true when hostname for srcRank and dstRank are the same
  // indicating that source and destination are on the same host
  return !strncmp(&hostNames[srcRank*TH_SHMEM_MAX_HOSTNAME_SIZE],
		  &hostNames[dstRank*TH_SHMEM_MAX_HOSTNAME_SIZE],
		  TH_SHMEM_MAX_HOSTNAME_SIZE);
}

void TH_ShMem::initRecv(void* buf, int tag)
{
  itsRecvBuf = ShMemBuf::toBuf(buf);
  itsRecvContext.setArgs(buf, itsSourceNode, tag);
    
  // only do this if buffer was allocated with shmem_malloc
  Assert (itsRecvBuf->matchMagicCookie());

  int        result;
  MPI_Status status;
    
  // recv info about message
  TRACER2 ("recvinit mpi from " << itsRecvContext.remote
	   << ' ' << itsRecvContext.tag);
  result = MPI_Recv(&(itsRecvContext.handle), sizeof(ShMemHandle), MPI_BYTE,
		    itsRecvContext.remote, itsRecvContext.tag,
		    MPI_COMM_WORLD, &status);
  TRACER2 ("recvinit mpi done " << result);
  int cnt;
  MPI_Get_count(&status, MPI_BYTE, &cnt);
  TRACER2 ("count " << cnt << ' '<<itsRecvContext.handle.getShmid()
	   <<' ' <<itsRecvContext.handle.getOffset());
	
  DbgAssertStr(status.MPI_SOURCE == itsSourceNode
	       && status.MPI_TAG == tag,
	       "incorrect status");
  DbgAssertStr(MPI_SUCCESS == result, "MPI_Recv failed");
  
  itsSendBuf = (ShMemBuf*)shmem_connect(itsRecvContext.handle.getShmid(),
					itsRecvContext.handle.getOffset());
  TRACER2 ("shmem connected " << itsSendBuf << ' '
	   << itsRecvContext.handle.getOffset());
	    
  Assert (itsSendBuf != 0);
  TRACER2 ("recvinit mpi fully done");
}

bool TH_ShMem::recvBlocking(void* buf, int nbytes, int tag)
{ 
  if (itsFirstCall)
  {
    TRACER2 (TH_ShMem::getCurrentRank() << " recv firstCall");
    itsFirstCall = false;
    initRecv(buf, tag);
    /* initRecv sets itsSendBuf (remote, connected) and itsRecvBuf (local) */
  }

  // all calls must have the same arguments except for nbytes
  DbgAssertStr(itsRecvContext.matchArgs(buf, itsSourceNode, tag),
	       "Arguments don't match");

  // check whether allocated with TH_ShMem::allocate
  Assert (itsRecvBuf->matchMagicCookie());

#ifdef SEM_SYNC
  // wait until sender is in send routine
  shmem_cond_wait(itsSendBuf->getSendReadyCondition());

  // do the memcpy
  memcpy(itsRecvBuf->getDataAddress(), itsSendBuf->getDataAddress(), nbytes);

  // signal send that transfer is complete
  shmem_cond_signal(itsSendBuf->getRecvCompleteCondition());

#else
  int sendReady;
  int ack = TH_SHMEM_ACK_MAGIC;

  int mpi_result = MPI_Recv(&sendReady, 1, MPI_INT, itsSourceNode, tag,
			    MPI_COMM_WORLD, &mpi_status);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Recv(sync) failed");
  DbgAssertStr(mpi_status.MPI_SOURCE == itsSourceNode &&
	       mpi_status.MPI_TAG == tag,
	       "incorrect status");
  DbgAssertStr(TH_SHMEM_SENDREADY_MAGIC == sendReady,
	       "sendReady mismatch");

  // do the memcpy
  memcpy(itsRecvBuf->getDataAddress(), itsSendBuf->getDataAddress(),
	 nbytes);
  mpi_result = MPI_Rsend(&ack, 1, MPI_INT, itsSourceNode, tag,
			 MPI_COMM_WORLD);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Send(sync) failed");	
#endif

  return true;
}

bool TH_ShMem::recvVarBlocking(int tag)
{ 
  if (itsFirstCall)
  {
    TRACER2 (TH_ShMem::getCurrentRank() << " recv firstCall");
    itsFirstCall = false;
    initRecv(getTransporter()->getDataHolder()->getDataPtr(), tag);
    /* initRecv sets itsSendBuf (remote, connected) and itsRecvBuf (local) */
  }

  // all calls must have the same arguments except for nbytes
  DbgAssertStr(itsRecvContext.matchArgs
	       (getTransporter()->getDataHolder()->getDataPtr(),
		itsSourceNode, tag),
	       "Arguments don't match");

  // check whether allocated with TH_ShMem::allocate
  Assert (itsRecvBuf->matchMagicCookie());

#ifdef SEM_SYNC
  // wait until sender is in send routine
  shmem_cond_wait(itsSendBuf->getSendReadyCondition());

  // Get the size of the data and resize the buffer.
  int size = DataHolder::getDataLength (itsSendBuf->getDataAddress());
  getTransporter()->getDataHolder()->resizeBuffer (size);
  DbgAssert (getTransporter()->getDataHolder()->getDataPtr() ==
	     itsRecvBuf->getDataAddress());
  // do the memcpy
  memcpy(itsRecvBuf->getDataAddress(), itsSendBuf->getDataAddress(), size);

  // signal send that transfer is complete
  shmem_cond_signal(itsSendBuf->getRecvCompleteCondition());

#else
  int sendReady;
  int ack = TH_SHMEM_ACK_MAGIC;
  MPI_Status mpi_status;

  int mpi_result = MPI_Recv(&sendReady, 1, MPI_INT, itsSourceNode, tag,
			    MPI_COMM_WORLD, &mpi_status);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Recv(sync) failed");
  DbgAssertStr(mpi_status.MPI_SOURCE == itsSourceNode &&
	       mpi_status.MPI_TAG == tag,
	       "incorrect status");
  DbgAssertStr(TH_SHMEM_SENDREADY_MAGIC == sendReady,
	       "sendReady mismatch");

  // Get the size of the data and resize the buffer.
  int size = DataHolder::getDataLength (itsSendBuf->getDataAddress());
  getTransporter()->getDataHolder()->resizeBuffer (size);
  DbgAssert (getTransporter()->getDataHolder()->getDataPtr() ==
	     itsRecvBuf->getDataAddress());
  // do the memcpy
  memcpy(itsRecvBuf->getDataAddress(), itsSendBuf->getDataAddress(), size);
  mpi_result = MPI_Rsend(&ack, 1, MPI_INT, itsSourceNode, tag,
			 MPI_COMM_WORLD);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Send(sync) failed");	
#endif

  return true;
}

bool TH_ShMem::recvNonBlocking(void* buf, int nbytes, int tag)
{
  cerr << "**Warning** TH_ShMem::recvNonBlocking() is not implemented. " 
       << "recvBlocking() is used instead." << endl;    
  return recvBlocking(buf, nbytes, tag);
} 

bool TH_ShMem::recvVarNonBlocking(int tag)
{
  cerr << "**Warning** TH_ShMem::recvVarNonBlocking() is not implemented. " 
       << "recvVarBlocking() is used instead." << endl;    
  return recvVarBlocking(tag);
} 

bool TH_ShMem::waitForReceived(void*, int, int)
{
  return true;
}

void TH_ShMem::initSend(void* buf, int tag)
{
  void* sendBufPtr = 0;

  itsSendBuf = ShMemBuf::toBuf(buf);
  sendBufPtr = (void*)itsSendBuf;
  itsSendContext.setArgs(buf, itsTargetNode, tag);

  Assert (itsSendBuf->matchMagicCookie());

  int         result;

  itsSendContext.handle.set(shmem_id(sendBufPtr),
			    shmem_offset(sendBufPtr));

  // send info about buffer
  TRACER2 ("sendinit mpi to " << itsTargetNode << ' ' << tag);
  result = MPI_Send(&(itsSendContext.handle),
		    sizeof(ShMemHandle), MPI_BYTE, itsTargetNode, tag,
		    MPI_COMM_WORLD);
  DbgAssertStr(MPI_SUCCESS == result, "MPI_Send failed");
  TRACER2 ("sendinit mpi done");
  TRACER2 ("handle " <<itsSendContext.handle.getShmid()
	   <<' '<<itsSendContext.handle.getOffset());
}

bool TH_ShMem::sendBlocking(void* buf, int nbytes, int tag)
{
  DbgAssert (nbytes == getTransporter()->getDataHolder()->getDataSize()
	     && buf == getTransporter()->getDataHolder()->getDataPtr());
  return sendVarBlocking (tag);
}

bool TH_ShMem::sendVarBlocking(int tag)
{
  if (itsFirstCall)
  {
    TRACER2 (TH_ShMem::getCurrentRank() << " send firstcall");
    itsFirstCall = false;

    initSend(getTransporter()->getDataHolder()->getDataPtr(), tag);
    /* sets itsSendContext, itsSendBuf (local); itsRecvBuf == 0 */

    DbgAssertStr(0 == itsRecvBuf, "itsRecvBuf not 0");
  }

  // all calls must have the same arguments except for nbytes
  DbgAssertStr(itsSendContext.matchArgs
	       (getTransporter()->getDataHolder()->getDataPtr(),
		itsTargetNode, tag),
	       "arguments don't match");

  Assert (itsSendBuf->matchMagicCookie());

#ifdef SEM_SYNC
  TRACER2 (TH_ShMem::getCurrentRank() << " send matchcookie sem");

  // signal that the send is ready
  shmem_cond_signal(itsSendBuf->getSendReadyCondition());
	
  // wait for receiver to complet
  shmem_cond_wait(itsSendBuf->getRecvCompleteCondition());

#else
  TRACER2 (TH_ShMem::getCurrentRank() << " send matchcookie mpi");
	
  int         sendReady = TH_SHMEM_SENDREADY_MAGIC;
  int         ack;
  MPI_Status  mpi_status;
  MPI_Request mpi_request;

  // post the receive
  int mpi_result = MPI_Irecv(&ack, 1, MPI_INT,
			     itsTargetNode, tag, MPI_COMM_WORLD, &mpi_request);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Irecv failed");

  mpi_result = MPI_Send (&sendReady, 1, MPI_INT,
			 itsTargetNode, tag, MPI_COMM_WORLD);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Send failed");

  mpi_result = MPI_Wait (&mpi_request, &mpi_status);
  DbgAssertStr(MPI_SUCCESS == mpi_result, "MPI_Wait failed");
  DbgAssertStr(TH_SHMEM_ACK_MAGIC == ack, "ack mismatch");
  DbgAssertStr(mpi_status.MPI_SOURCE == itsTargetNode
	       && mpi_status.MPI_TAG == tag,
	       "incorrect status");
#endif

  TRACER2 (TH_ShMem::getCurrentRank() << " sendVarBlocking done");
  return true;
}

bool TH_ShMem::sendNonBlocking(void* buf, int nbytes, int tag)
{
  cerr << "**Warning** TH_ShMem::sendNonBlocking() is not implemented. " 
       << "The sendBlocking() method is used instead." << endl;    
  return sendBlocking(buf, nbytes, tag);
}

bool TH_ShMem::sendVarNonBlocking(int tag)
{
  cerr << "**Warning** TH_ShMem::sendVarNonBlocking() is not implemented. " 
       << "The sendVarBlocking() method is used instead." << endl;    
  return sendVarBlocking(tag);
}

bool TH_ShMem::waitForSent(void*, int, int)
{
  return true;
}

void TH_ShMem::waitForBroadCast()
{
#ifdef HAVE_MPI
  /// Wait for a broadcast message with MPI
  unsigned long timeStamp;
  MPI_Bcast(&timeStamp, 1, MPI_UNSIGNED_LONG,
	    CONTROLLER_NODE, MPI_COMM_WORLD);
  TRACER2 ("Broadcast received timestamp " <<timeStamp
	   << " rank=  " << getCurrentRank());
#endif
}

void TH_ShMem::waitForBroadCast(unsigned long& aVar)
{
#ifdef HAVE_MPI
  TRACER2 ("wait for broadcast");
  /// Wait for a broadcast message with MPI
    MPI_Bcast(&aVar, 1, MPI_UNSIGNED_LONG, CONTROLLER_NODE, MPI_COMM_WORLD);
    TRACER2 ("Broadcast received timestamp " << aVar
	     << " rank=" << getCurrentRank());
#endif
}


void TH_ShMem::sendBroadCast(unsigned long timeStamp)
{
#ifdef HAVE_MPI
  TRACER2 ("send broadcast");
  /// Send a broadcast timestamp
  MPI_Bcast(&timeStamp, 1, MPI_UNSIGNED_LONG, CONTROLLER_NODE, MPI_COMM_WORLD);
  TRACER2 ("Broadcast sent timestamp " <<timeStamp); 
#endif
}

int TH_ShMem::getCurrentRank()
{
#ifdef HAVE_MPI
  int rank;

  ///  Get the current node 
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  
  return rank;
#else
  return -1;
#endif
}

int TH_ShMem::getNumberOfNodes()
{
#ifdef HAVE_MPI
  int size;

  /// get the Number of nodes
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  return size;
#else
  return 1;
#endif
}

#ifdef HAVE_MPI
void TH_ShMem::init(int argc, const char* argv[])
{
  int initialized = 0;
  char myHostName[TH_SHMEM_MAX_HOSTNAME_SIZE];
    
  // initialize use of share memory allocator
  shmem_init();

  /// Initialize the MPI communication
  MPI_Initialized(&initialized);
  if (!initialized) {
    MPI_Init (&argc,(char***)&argv);
  }

  hostNames   = (char*)malloc(TH_SHMEM_MAX_HOSTNAME_SIZE*getNumberOfNodes());

  // get hostname of this host
  if (gethostname(myHostName, TH_SHMEM_MAX_HOSTNAME_SIZE) < 0)
  {
    perror("gethostname");

    // set hostname equal to rank (backup method)
    snprintf(myHostName, TH_SHMEM_MAX_HOSTNAME_SIZE,
	     "%d", getCurrentRank());
  }
    
  // Send myHostName to all other nodes
  // and receive all their hostnames.
  MPI_Allgather(&myHostName, TH_SHMEM_MAX_HOSTNAME_SIZE, MPI_BYTE,
		hostNames,   TH_SHMEM_MAX_HOSTNAME_SIZE, MPI_BYTE,
		MPI_COMM_WORLD);
    
#else
void TH_ShMem::init(int, const char* [])
{
#endif
}

void TH_ShMem::finalize()
{
#ifdef HAVE_MPI
  free(hostNames);
  /// finalize the MPI communication
  MPI_Finalize();
#endif
}

void TH_ShMem::synchroniseAllProcesses()
{
#ifdef HAVE_MPI
  TRACER2 ("Synchronise all");
  MPI_Barrier(MPI_COMM_WORLD);
  TRACER2 ("Synchronised...");
#endif
}

string TH_ShMem::getHostName(int rank)
{
  DbgAssertStr(rank >= 0 && rank < getNumberOfNodes(),
	       "invalid rank");
  DbgAssertStr(0 != hostNames, "hostNames == 0");

  return string(&hostNames[rank*TH_SHMEM_MAX_HOSTNAME_SIZE]);
}

TH_ShMem::ShMemHandle::ShMemHandle()
  : itsShmid(0), itsOffset(0)
{}

void TH_ShMem::ShMemHandle::set(int shmid, size_t offset)
{
  itsShmid = shmid;
  itsOffset = offset;
}

void TH_ShMem::CommContext::setArgs(void* theBuf, int theRemote, int theTag)
{
  buf    = theBuf;
  remote = theRemote;
  tag    = theTag;
}

bool TH_ShMem::CommContext::matchArgs(void* theBuf, int theRemote, int theTag)
{
  return (buf == theBuf && remote == theRemote && tag == theTag);
}

TH_ShMem::ShMemBuf::ShMemBuf()
{}

void TH_ShMem::ShMemBuf::init(void)
{
  itsMagicCookie = TH_SHMEM_MAGIC_COOKIE;
  shmem_cond_init(&itsRecvCompleteCondition);
  shmem_cond_init(&itsSendReadyCondition);
  itsBuf = 0;
}

TH_ShMem::ShMemBuf* TH_ShMem::ShMemBuf::toBuf(void* ptr)
{
  ShMemBuf *result = 0;
  ShMemBuf b;

  result = (TH_ShMem::ShMemBuf*)((char*)ptr - ((char*)&(b.itsBuf) - (char*)&b));

  return result;
}

int TH_ShMem::ShMemBuf::getMagicCookie(void)
{
  return itsMagicCookie;
}

bool TH_ShMem::ShMemBuf::matchMagicCookie(void)
{
  return TH_SHMEM_MAGIC_COOKIE == itsMagicCookie;
}

shmem_cond_t* TH_ShMem::ShMemBuf::getRecvCompleteCondition(void)
{
  return &itsRecvCompleteCondition;
}

shmem_cond_t* TH_ShMem::ShMemBuf::getSendReadyCondition(void)
{
  return &itsSendReadyCondition;
}

void* TH_ShMem::ShMemBuf::getDataAddress()
{
  // return the address of the itsBuf field since
  // that is where the buffer should start
  return &itsBuf;
}


}

#endif
