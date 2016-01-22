#!/usr/bin/python

import logging
import datetime
from lofar.messaging.RPC import RPC, RPCException

''' Simple RPC client for Service lofarbus.RAS.*Z
'''

logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s', level=logging.INFO)
logger = logging.getLogger(__name__)

class RARPCException(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return "RARPCException: " + str(self.message)


class RARPC:
    def __init__(self, busname='lofarbus'):
        self.busname = busname
        self._serviceRPCs = {} #cache of rpc's for each service

    def __enter__(self):
        """
        Internal use only. (handles scope 'with')
        """
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Internal use only. (handles scope 'with')
        """
        for servicename, rpc in self._serviceRPCs.items():
            rpc.__exit__(None, None, None)

    def _rpc(self, service, timeout=10, **kwargs):
        try:
            rpckwargs = {'timeout': timeout}

            if service not in self._serviceRPCs:
                rpc = RPC(service, busname=self.busname, ForwardExceptions=True, **rpckwargs)
                rpc.Request.__enter__()
                self._serviceRPCs[service] = rpc

            rpc = self._serviceRPCs[service]

            if kwargs:
                res, status = rpc(**kwargs)
            else:
                res, status = rpc()

            if status != 'OK':
                logger.error('status: %s' % status)
                logger.error('result: %s' % res)
                raise RARPCException("%s %s" % (status, res))

            return res
        except RPCException as e:
            logger.error(str(e))
            raise RARPCException(str(e))

    def getResourceClaimStatuses(self):
        return self._rpc('RAS.GetResourceClaimStatuses')

    def getResourceClaims(self):
        claims = self._rpc('RAS.GetResourceClaims')
        for claim in claims:
            claim['starttime'] = claim['starttime'].datetime()
            claim['endtime'] = claim['endtime'].datetime()
        return claims

    def getResourceClaim(self, id):
        resource_claim = self._rpc('RAS.GetResourceClaim', id=id)
        if resource_claim:
            resource_claim['starttime'] = resource_claim['starttime'].datetime()
            resource_claim['endtime'] = resource_claim['endtime'].datetime()
        return resource_claim


    def insertResourceClaim(self, resource_id, task_id, starttime, endtime, status, session_id, claim_size, username, user_id):
        return self._rpc('RAS.InsertResourceClaim', resource_id=resource_id,
                                                    task_id=task_id,
                                                    starttime=starttime,
                                                    endtime=endtime,
                                                    status=status,
                                                    session_id=session_id,
                                                    claim_size=claim_size,
                                                    username=username,
                                                    user_id=user_id)

    def deleteResourceClaim(self, id):
        return self._rpc('RAS.DeleteResourceClaim', id=id)

    def updateResourceClaim(self, id, resource_id=None, task_id=None, starttime=None, endtime=None, status=None, session_id=None, claim_size=None, username=None, user_id=None):
        return self._rpc('RAS.UpdateResourceClaim', id=id,
                                                    resource_id=resource_id,
                                                    task_id=task_id,
                                                    starttime=starttime,
                                                    endtime=endtime,
                                                    status=status,
                                                    session_id=session_id,
                                                    claim_size=claim_size,
                                                    username=username,
                                                    user_id=user_id)

    def getResourceGroupTypes(self):
        return self._rpc('RAS.GetResourceGroupTypes')

    def getResourceGroups(self):
        return self._rpc('RAS.GetResourceGroups')

    def getResourceTypes(self):
        return self._rpc('RAS.GetResourceTypes')

    def getResources(self):
        return self._rpc('RAS.GetResources')

    def getTask(self, id):
        task = self._rpc('RAS.GetTask', id=id)
        if task:
            task['starttime'] = task['starttime'].datetime()
            task['endtime'] = task['endtime'].datetime()
        return task

    def insertTask(self, mom_id, otdb_id, status, type, specification_id):
        return self._rpc('RAS.InsertTask', mom_id=mom_id,
                                           otdb_id=otdb_id,
                                           status=status,
                                           type=type,
                                           specification_id=specification_id)

    def deleteTask(self, id):
        return self._rpc('RAS.DeleteTask', id=id)

    def updateTask(self, task_id, mom_id=None, otdb_id=None, status=None, task_type=None, specification_id=None):
        return self._rpc('RAS.UpdateTask', task_id=task_id,
                                           mom_id=mom_id,
                                           otdb_id=otdb_id,
                                           status=status,
                                           task_type=task_type,
                                           specification_id=specification_id)

    def getTasks(self):
        tasks = self._rpc('RAS.GetTasks')
        for task in tasks:
            task['starttime'] = task['starttime'].datetime()
            task['endtime'] = task['endtime'].datetime()
        return tasks

    def getTaskTypes(self):
        return self._rpc('RAS.GetTaskTypes')

    def getTaskStatuses(self):
        return self._rpc('RAS.GetTaskStatuses')

    def getUnits(self):
        return self._rpc('RAS.GetUnits')

if __name__ == '__main__':
    with RARPC() as rpc:
        #claims = rpc.getResourceClaims()
        #for c in claims:
            #rpc.deleteResourceClaim(c['id'])
            #print rpc.getResourceClaims()

        for i in range(0, 10):
            taskId = rpc.insertTask(1234, 5678, 'active', 'OBSERVATION', 1)['id']
            rcId = rpc.insertResourceClaim(1, taskId, datetime.datetime.utcnow(), datetime.datetime.utcnow() + datetime.timedelta(hours=1), 'CLAIMED', 1, 10, 'einstein', -1)['id']
            print rpc.getResourceClaim(rcId)
            rpc.updateResourceClaim(rcId, starttime=datetime.datetime.utcnow(), endtime=datetime.datetime.utcnow() + datetime.timedelta(hours=2), status='ALLOCATED')
            print rpc.getResourceClaim(rcId)
            print

        tasks = rpc.getTasks()
        for t in tasks:
            print rpc.getTask(t['id'])
            print rpc.deleteTask(t['id'])
            #print rpc.getTasks()
            #print rpc.getResourceClaims()

        #print rpc.getTasks()
        #print rpc.getResourceClaims()

        #rpc.deleteTask(taskId)

        #print rpc.getTasks()
        #print rpc.getResourceClaims()



