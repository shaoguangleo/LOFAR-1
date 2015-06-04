#!/usr/bin/env python
import xmlrpclib, socket, os, logging, logging.handlers, time
import sitecustomize
try:
  #from wsrt_webdavlib import wsrt_webdavlib # just in here so we can test it exists
  import SOAPpy, mom_http #Old deprecated stuff, should be replaced?
except:
  SOAPpy = None

exportClient = None
momClient    = None
momServer    = None
ltaClient    = None

## Determine host
host      = socket.gethostname()
## Using netifaces module or something similar would be nicer, but it doesn't want to install in a custom dir
## So we use this hack
if 'lexar' in host:
  host = host + '.offline.lofar'
if 'gridftp01.target.rug.nl' in host:
  host = 'lotar2.staging.lofar'
if 'gridftp02.target.rug.nl' in host:
  host = 'lotar4.staging.lofar'

ipaddress = socket.gethostbyname(host)
ltacpport = 8802

## Ingest Master server settings
masterAddress = '10.178.1.2'
masterPort    = 2012
masterAuth    = 'lta'
maxMasterTalkerQueue=0

def isOpen(ip,port):
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  try:
    s.connect((ip, int(port)))
    s.shutdown(2)
    return True
  except:
    return False

#We might need a more elegant solution for this, but for now the convenience of having one config for everything is nice
if ipaddress == masterAddress:
  if not isOpen(masterAddress, masterPort): #No master is running on this machine
    master = True
  else:
    master = False
else:
  master = False

## Make directories if needed, should include trailing /
#default (local) logroot
logroot = '/tmp/ingest/'

#special cases for production lexar and lotart systems
if 'lexar' in host:
  logroot = '/log/ingest/'
if 'lotar' in host:
  logroot = '/home/lofarlocal/log/ingest'

jobsdir   = logroot + '/jobs/'
retrydir  = logroot + '/jobs_retry/'
faileddir = logroot + '/log/failed/'
logdir    = logroot + '/log/'
donedir   = logroot + '/log/done/'

if not os.path.exists(jobsdir):
  os.makedirs(jobsdir)
if not os.path.exists(retrydir):
  os.makedirs(retrydir)
if not os.path.exists(faileddir):
  os.makedirs(faileddir)
if not os.path.exists(logdir):
  os.makedirs(logdir)
if not os.path.exists(donedir):
  os.makedirs(donedir)

## Set logger
#use WatchedFileHandler which creates a new log file if the OS moves it with logrotate
log_handler = logging.handlers.WatchedFileHandler(logdir + host + '_' + ('master' if master else 'slave') + '.log')
formatter = logging.Formatter("%(asctime)-15s %(levelname)s %(message)s")
formatter.converter = time.gmtime
log_handler.setFormatter(formatter)
logger = logging.getLogger('Master' if master else 'Slave')
logger.addHandler(log_handler)
logger.setLevel(logging.DEBUG)
logger.info('--------- Logger initialized ---------')

## Now we can check if the legacy WSRT code loaded and report it to the logger
if not SOAPpy:
  logger.critical('SOAPpy, http_login or wsrt_webdavlib not found, set your PYTHONPATH !')
  exit(2)

## Ingest Slave setting, might become dependent on host name
parallelJobs  = 20
maxSlaveTalkerQueue = 80

pipelineRetry = 3

## LTA Catalog settings
# application53 (de ingest xml-rpc services)
#
#      lofar-ingest.target.rug.nl:9443
#      lofar-ingest-test.target.rug.nl:19443
#
LTAurl = 'https://@lofar-ingest.target.rug.nl:9443/'
try:
  ltaClient = xmlrpclib.ServerProxy(LTAurl)
except:
  ltaClient = None
  logger.excepion('Configuration failed on LTA client')
  raise
ltaRetry    = 5

## MoM client settings
exportLogin  = 'https://lcs029.control.lofar:8443/export/systemprograms/login.jsp'
exportStatus = 'https://lcs029.control.lofar:8443/export/interface/pipeline/setStatus.do'
exportLogout = 'https://lcs029.control.lofar:8443/export/processLogout.do'

momURLlogin  = 'https://lcs029.control.lofar:8443/useradministration/user/systemlogin.do'
momURLgetSIP = 'https://lcs029.control.lofar:8443/mom3/interface/importXML2.do'
momURLlogout = 'https://lcs029.control.lofar:8443/useradministration/user/logout.do'
momRetry     = 3

srmRetry     = 2
if 'lexar' in host:
  srmInit      = '/globalhome/ingest/service/bin/init.sh'
if 'lotar' in host:
  srmInit      = '/home/lofarlocal/ltacp/bin/init.sh'

try:
  exportClient= mom_http.client(exportLogin, exportStatus, exportLogout)
  exportClient.username = '' #fill in during build/install
  exportClient.password = '' #fill in during build/install
  momClient = mom_http.client(momURLlogin, momURLgetSIP, momURLlogout)
  momClient.username = '' #fill in during build/install
  momClient.password = '' #fill in during build/install
except:
  momClient    = None
  exportClient = None
  logger.exception('Configuration failed on MoM export')
  raise

## Initialize MoM listener
if master:
  try:
    momPort   = 2010
    ## move to xmlrpc?: momServer = xmlrpclib.Server(ipaddress:momPort)
    momServer = SOAPpy.SOAPServer((ipaddress, momPort), )
  except:
    momServer = None
    logger.exception('Configuration failed on MoM listener')
    raise

# Used by Master to send error and (re) start messages
mailCommand = '-c renting@astron.nl,holties@astron.nl,wjvriend@astro.rug.nl,observer@astron.nl sciencesupport@astron.nl'
# Used by Slave to send SendStatus warnings
mailSlCommand = '-c holties@astron.nl,wjvriend@astro.rug.nl renting@astron.nl'
 
logger.info("Configuration complete")
