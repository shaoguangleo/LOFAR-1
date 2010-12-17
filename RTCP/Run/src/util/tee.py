from threading import Thread
from select import select
import os,fcntl
import sys

def setNonBlock( fd ):  
  # get the file's current flag settings
  fl = fcntl.fcntl(fd, fcntl.F_GETFL)
  
  # update the file's flags to put the file into non-blocking mode.
  fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)


class Tee(Thread):
  def __init__(self,inputfd,outputfiles,logger=None):
    Thread.__init__(self)

    self.inputfd = inputfd
    self.outputfiles = filter( outputfiles, lambda x: x is not None )
    self.logger = logger

    setNonBlock( self.inputfd )

    self.setDaemon( True )
    self.start()

  def run(self):
    fileno = self.inputfd
    prevline = "" # last incomplete line

    while True:
      rlist,_,xlist = select( [fileno], [], [fileno] )

      if fileno in xlist:
        # exceptional condition
        break

      if fileno in rlist:
        data = os.read( self.inputfd, 4096 )

        if len(data) == 0:
          # eof
          break

        lines = data.split("\n")
        lines[0] = "%s%s" % (prevline,lines[0])

        for line in lines[:-1]: 
          if self.logger:
            self.logger.info( "%s", line )

          for f in self.outputfiles:
            try:
              f.write( "%s\n" % (line,) )
            except OSError,e:
              pass
            except ValueError,e: # ValueError: I/O operation on closed file
              pass

        prevline = lines[-1]      

