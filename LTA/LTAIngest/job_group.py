#!/usr/bin/python
#This is a class for managing a group of jobs, usually a MoM export, but it can also be from other sources

try:
  import MySQLdb, datetime, os
except:
  import datetime, os
from job_parser import JobRetry, JobError, JobHold, JobScheduled, JobProducing, JobProduced
import job_parser as parser

corr_type   = 0
bf_type     = 1
img_type    = 2
unspec_type = 3
pulp_type   = 4

class job_group():
  """Class to keep track of a collection of jobs with the same lofar_export.exports.id in MoM
  or the equivalent in the manual ingest script."""
  def __init__(self, logger, Id, Type, mailCommand):
    self.logger = logger
    self.Id = Id
    self.jobs = {}
    self.scheduled = {'Total':0} ##(, 'xxxxx' : 0) for each obsId
    self.active    = {'Total':0}
    self.inactive  = {'Total':0}
    self.retry     = {'Total':0}
    self.failed    = {'Total':0}
    self.done      = {'Total':0}

    self.corr      = 0
    self.bf        = 0
    self.img       = 0
    self.unspec    = 0
    self.pulp      = 0
    self.Type      = Type
    self.parser    = parser.parser(logger)
    self.mailCommand = mailCommand
    self.get_db_info()
    self.logger.info('New job_group %i initialzed' % self.Id)

  def read_old_jobs(self, faileddir, donedir):
    done_files   = []
    failed_files = []
    if self.Type == 'MoM':
      Dir = '/A_%s/' % self.Id  
    else: ## tier0-ingest
      Dir = '/B_%s/' % self.Id
    if os.path.isdir(donedir + Dir):
      done_files = os.listdir(donedir + Dir)
      for f in done_files:
        job = self.parser.parse(donedir + Dir + f)
        job['filename'] = f
        if job['Status'] == JobScheduled:
          job['Status'] = JobProduced
          self.add_job(job)
          self.update_job(job, JobScheduled, JobProduced, None)
    if os.path.isdir(faileddir + Dir):
      failed_files = os.listdir(faileddir + Dir)
      for f in failed_files:
        job = self.parser.parse(faileddir + Dir + f)
        job['filename'] = f
        if job['Status'] == JobScheduled:
          job['Status'] = JobError
          self.add_job(job)
          self.update_job(job, JobScheduled, JobError, None)
    self.logger.info('New job_group %i has read %i old jobs' % (self.Id, len(done_files) + len(failed_files)))

  def add_job(self, job):
    self.jobs[job['ExportID']] = job
    self.scheduled['Total'] += 1
    if job['ObservationId'] in self.scheduled:
      self.scheduled[job['ObservationId']] += 1
    else:
      self.scheduled[job['ObservationId']] = 1
      self.active[job['ObservationId']]    = 0
      self.inactive[job['ObservationId']]  = 0
      self.retry[job['ObservationId']]     = 0
      self.failed[job['ObservationId']]    = 0
      self.done[job['ObservationId']]      = 0
 
  def update_file_type(self, fileType):
    if fileType == corr_type: self.corr     += 1
    if fileType == bf_type:   self.bf       += 1
    if fileType == img_type: self.img       += 1
    if fileType == unspec_type: self.unspec += 1      
    if fileType == pulp_type: self.pulp     += 1      
 
  def update_job(self, job, old_status, new_status, fileType):
    self.jobs[job['ExportID']] = job
    if old_status == JobScheduled:
      self.scheduled[job['ObservationId']] -= 1
      self.scheduled['Total']              -= 1
    if old_status == JobProducing:
      self.active[job['ObservationId']]    -= 1
      self.active['Total']                 -= 1
    if old_status == JobHold:
      self.inactive[job['ObservationId']]  -= 1
      self.inactive['Total']               -= 1
    if old_status == JobRetry:
      self.retry[job['ObservationId']]     -= 1
      self.retry['Total']                  -= 1
    if old_status == JobError:
      self.failed[job['ObservationId']]    -= 1
      self.failed['Total']                 -= 1
    if old_status == JobProduced:
      self.done[job['ObservationId']]      -= 1
      self.done['Total']                   -= 1
    ##some of these status transitions should not happen.
    if new_status == JobScheduled:
      self.scheduled[job['ObservationId']] += 1
      self.scheduled['Total']              += 1
    if new_status == JobProducing:
      self.active[job['ObservationId']]    += 1
      self.active['Total']                 += 1
    if new_status == JobHold:
      self.inactive[job['ObservationId']]  += 1
      self.inactive['Total']               += 1
    if new_status == JobRetry:
      self.retry[job['ObservationId']]     += 1
      self.retry['Total']                  += 1
    if new_status == JobError:
      self.failed[job['ObservationId']]    += 1
      self.failed['Total']                 += 1
      self.update_file_type(fileType)
    if new_status == JobProduced:
      self.done[job['ObservationId']]      += 1
      self.done['Total']                   += 1
      self.update_file_type(fileType)

  def check_finished(self):
    total = len(self.jobs)
    finished = (self.scheduled['Total'] == 0) and (self.active['Total'] == 0) and (self.inactive['Total'] == 0) and (self.retry['Total'] == 0)
    if finished and (total == self.failed['Total'] + self.done['Total']): ## sanity check, somewhat redundant
      self.logger.info('job_group %i is finished, total of %i files' % (self.Id, total))
      return True
    return False

  def get_destination(self): # hack to support Target slaves, requires Target to be in the Resource name for the project
    if 'Target' in self.job_info['ltalocation']:
      return 'lotar'
    return 'lexar'

  def get_db_info(self):
    now = datetime.datetime.today().replace(microsecond=0)
    self.job_info = {'id': self.Id, 'start_time': now, 'update_time': now,
                     'user': "unknown", 'status': "unknown", 'name': 'unknown',
                      'project': 'unknown', 'ltalocation': "unknown", 'eid': "unknown"}
    if self.Type == 'MoM':
      try:
        ## should be read from config file
        #m = "lofartest_sas099_mom3_two"
        m = "lofar_mom3"
        #e = "lofartest_sas099_export"
        e = "lofar_export"
        db = MySQLdb.connect(host="mysql1.control.lofar", user="momreadonly", passwd="daub673(ming", db=m)
        c = db.cursor()
        c.execute("SELECT a.id,toexportdate as started,a.status_date as last_update,a.exportername,b.name as state,d.name,e.name as projectname,g.name as location,a.data_location,d.mom2id FROM %s.exports AS a, %s.status AS b, %s.mom_references AS c, %s.mom2object AS d, %s.mom2object AS e, %s.resource AS f, %s.resourcetype AS g WHERE status_id=b.id AND a.mom_reference_id = c.id AND c.mom_id=d.mom2id AND d.ownerprojectid = e.id AND e.id=f.projectid AND f.resourcetypeid > 1 AND f.resourcetypeid = g.id and a.id = %i;" % (e,e,e,m,m,m,m,self.Id))
        db_job = c.fetchone()
        ## (1137L, datetime.datetime(2014, 2, 6, 8, 30, 34), datetime.datetime(2014, 2, 8, 18, 41, 2), 'toribio', 'running', 'Orion', 'LC1_055', 'Lofar Storage (J\xfclich)', 'LC1_055/358795', 358795L)
        self.job_info = {'id': db_job[0], 'start_time': db_job[1], 'update_time': db_job[2],
                         'user': db_job[3].rjust(8), 'status': db_job[4], 'name': db_job[5].rjust(9),
                         'project': db_job[6], 'ltalocation': db_job[7], 'eid': db_job[8]}
      except Exception as e:
        self.logger.warning('Caught an exception trying to talk to the Mom database: %s' % str(e))
    else:
      try:
        if len(self.jobs): #get_db_info now also happens in __init__
          job = self.jobs.values()[0]
          self.job_info['name']    = job['Source']
          self.job_info['project'] = job['Project']
      except:
        self.logger.warning('Caught an exception trying to create job info: %s' % str(e))
    self.job_info['run_time'] = now - self.job_info['update_time']
    self.job_info['duration'] = now - self.job_info['start_time']

  def make_sub_report(self, Input, name):
    if Input['Total'] > 0:
      message = "Total %(name)s: %(total)i\nObsId : #files\n" % {'name': name, 'total': Input['Total']}
      for (k,v) in Input.iteritems():
        if k == 'Total': continue
        message += "L%(ObservationId)s: %(count)i\n" % {'ObservationId': k, 'count': v}
        message += "\n"
    else: 
      message = ""
    return message

  def make_report(self):
    header = """=== Report on your ingest Job "%(name)s" (%(id)i) ===

Status: %(status)s
User: %(user)s
Project: %(project)s
Start: %(start_time)s
Last update: %(update_time)s
Time in queue: %(duration)s
Time since last update: %(run_time)s
Stored at: %(ltalocation)s""" % {'name': self.job_info['name'], 'id': self.job_info['id'], 'status': self.job_info['status'],
                               'user': self.job_info['user'], 'project': self.job_info['project'], 'start_time': self.job_info['start_time'].isoformat(),
                               'update_time': self.job_info['update_time'].isoformat(), 'duration': str(self.job_info['duration']),
                               'run_time': str(self.job_info['run_time']), 'ltalocation': self.job_info['ltalocation'].decode('latin1').encode('utf-8')}

    summary = """\n\n=== Summary ===
Total Files Success: %(done)i
- Interferometer: %(corr)i
- Beamformed: %(bf)i
- SkyImages: %(img)i
- Unspecified: %(unspec)i
- Pulsar Pipeline: %(pulp)i

Total Failed: %(failed)i""" % {'done': self.done['Total'], 'corr': self.corr, 'bf': self.bf, 'img': self.img, 'unspec': self.unspec, 'pulp': self.pulp, 'failed': self.failed['Total']}

    error_list = {}
    failed_files = "\n\n==== Failed files: =====\n"
    ##L169235_SB180_uv.dppp.MS, locus071: ssh connection failed
    for j in self.jobs.values():
      if j['Status'] == JobError:
        failed_files += "%(file_name)s, %(host)s: %(error)s\n" % {'file_name': j['DataProduct'], 'host': j['Location'].split(':')[0], 'error': j['errors']}
        for e in j['errors']:
          if e in error_list:
            error_list[e] += 1
          else:
            error_list[e] = 1

    errors = "\n"  ##"- ssh connection failed: 20"
    for (k, v) in error_list.iteritems():
      errors += "- %s: %i\n" % (k, v)

    details = """\n\n===== Details =====\n"""
    details += self.make_sub_report(self.scheduled, "files scheduled")
    details += self.make_sub_report(self.active, "files running")
    details += self.make_sub_report(self.inactive, "files on hold")
    details += self.make_sub_report(self.retry, "files on retry")
    details += self.make_sub_report(self.failed, "files failed")
    details += self.make_sub_report(self.done, "files success")


    message = header + summary + errors + failed_files + details
    return message

  def send_mail(self):
    self.get_db_info()
    message = self.make_report()
    os.system('echo "%s"|mailx -s "Ingest job of %s, %s(%i) has ended" ' % (message, self.job_info['user'], self.job_info['name'], self.Id) + self.mailCommand)
    self.logger.info('job_group %i sent an email to %s' % (self.Id, self.mailCommand))

  
## Stand alone execution code ------------------------------------------
if __name__ == '__main__':

  ##This test code might need updating.
  import logging
  logging.basicConfig()
  l = logging.getLogger()
  l.setLevel(10)
  Id = 476
  standalone = job_group(l, Id, 'MoM', 'renting@astron.nl')
  jobs = []
  for i in range(2):
    k = {'Status': JobScheduled, 'ExportID': 'jA_%s_%s_%i_L12345_SAP000_SB000_uv.MS' % (Id, Id, i), 'ObservationId': 12345, 'MoMId': 654321, 'DataProduct': 'L12345_SAP000_SB000_uv.MS', 'Location': 'locus123:/data/L12345/L12345_SAP000_SB000_uv.MS', 'errors': []}
    jobs.append(k)
  for job in jobs:
    standalone.add_job(job)
  for job in jobs:
    standalone.update_job(job, JobScheduled, JobProducing, None)
  for job in jobs:
    standalone.update_job(job, JobProducing, JobProduced, corr_type)
  standalone.get_db_info()
  message = standalone.make_report()
  print(message)
