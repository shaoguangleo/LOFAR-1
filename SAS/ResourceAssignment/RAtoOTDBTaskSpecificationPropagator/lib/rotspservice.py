#!/usr/bin/env python

# rotspservice.py: RAtoOTDBTaskSpecificationPropagator listens on the lofar ?? bus and calls onTaskScheduled
#
# Copyright (C) 2015
# ASTRON (Netherlands Institute for Radio Astronomy)
# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
#
# This file is part of the LOFAR software suite.
# The LOFAR software suite is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# The LOFAR software suite is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with the LOFAR software suite. If not, see <http://www.gnu.org/licenses/>.
#
# $Id: rotspservice.py 1580 2015-09-30 14:18:57Z loose $

"""
RATaskStatusChangedListener listens to a bus on which tasks handled by the ResourceAssigner get published.
It will then try to propagate the changes to OTDB as Scheduled or Conflict.
"""

import qpid.messaging
import logging
from datetime import datetime
import time

from lofar.messaging.RPC import RPC, RPCException

import lofar.sas.resourceassignment.resourceassignmentservice.rpc as rarpc ## RA DB
from lofar.sas.resourceassignment.database.radbbuslistener import RADBBusListener
from lofar.sas.resourceassignment.database.config import DEFAULT_NOTIFICATION_BUSNAME as RA_NOTIFICATION_BUSNAME
from lofar.sas.resourceassignment.database.config import DEFAULT_NOTIFICATION_PREFIX as RA_NOTIFICATION_PREFIX
from lofar.sas.resourceassignment.ratootdbtaskspecificationpropagator.propagator import RAtoOTDBPropagator

logger = logging.getLogger(__name__)

class RATaskStatusChangedListener(RADBBusListener):
    def __init__(self,
                 busname=RA_NOTIFICATION_BUSNAME,
                 subject=RA_NOTIFICATION_PREFIX + 'TaskUpdated',
                 broker=None,
                 propagator=None, ## TODO also give translator?
                 **kwargs):
        """
        RATaskScheduledListener listens on the lofar ?? bus and calls onTaskScheduled or onTaskConclict
        :param busname: valid Qpid address (default: lofar.ra.notification)
        :param broker: valid Qpid broker host (default: None, which means localhost)
        additional parameters in kwargs:
                options=     <dict>    Dictionary of options passed to QPID
                exclusive= <bool>    Create an exclusive binding so no other services can consume duplicate messages (default: False)
                numthreads= <int>    Number of parallel threads processing messages (default: 1)
                verbose=     <bool>    Output extra logging over stdout (default: False)
        """
        super(RATaskStatusChangedListener, self).__init__(busname=busname, subjects=subject, broker=broker, **kwargs)

        self.propagator = propagator
        if not self.propagator:
            self.propagator =  RAtoOTDBPropagator()

    def onTaskUpdated(self, old_task, new_task):
        # override super onTaskUpdated
        # check for status change, and call either onTaskScheduled or onTaskScheduled
        if old_task['status_id'] != new_task['status_id']:
            if new_task['status'] == 'scheduled':
                self.onTaskScheduled(new_task['id'], new_task['otdb_id'], new_task['mom_id'])
            elif new_task['status'] == 'conflict':
                self.onTaskConflict(new_task['id'], new_task['otdb_id'], new_task['mom_id'])

    def onTaskInserted(self, new_task):
        # override super onTaskInserted
        # check for status, and call either onTaskScheduled or onTaskScheduled
        if new_task['status'] == 'scheduled':
            self.onTaskScheduled(new_task['id'], new_task['otdb_id'], new_task['mom_id'])
        elif new_task['status'] == 'conflict':
            self.onTaskConflict(new_task['id'], new_task['otdb_id'], new_task['mom_id'])

    def onTaskScheduled(self, ra_id, otdb_id, mom_id):
        logger.info('onTaskScheduled: ra_id=%s otdb_id=%s mom_id=%s' % (ra_id, otdb_id, mom_id))

        self.propagator.doTaskScheduled(ra_id, otdb_id, mom_id)

    def onTaskConflict(self, ra_id, otdb_id, mom_id):
        logger.info('onTaskConflict: ra_id=%s otdb_id=%s mom_id=%s' % (ra_id, otdb_id, mom_id))

        self.propagator.doTaskConflict(ra_id, otdb_id, mom_id)


__all__ = ["RATaskStatusChangedListener"]

def main():
    from optparse import OptionParser
    from lofar.messaging import setQpidLogLevel
    from lofar.common.util import waitForInterrupt

    from lofar.sas.resourceassignment.resourceassignmentservice.config import DEFAULT_BUSNAME as RADB_BUSNAME
    from lofar.sas.resourceassignment.resourceassignmentservice.config import DEFAULT_SERVICENAME as RADB_SERVICENAME
    from lofar.sas.otdb.config import DEFAULT_OTDB_SERVICE_BUSNAME, DEFAULT_OTDB_SERVICENAME

    from lofar.mom.momqueryservice.config import DEFAULT_MOMQUERY_BUSNAME, DEFAULT_MOMQUERY_SERVICENAME


    # Check the invocation arguments
    parser = OptionParser("%prog [options]",
                          description='runs the RAtoOTDBTaskSpecificationPropagator service')
    parser.add_option('-q', '--broker', dest='broker', type='string', default=None, help='Address of the qpid broker, default: localhost')
    parser.add_option("--notification_busname", dest="notification_busname", type="string", default=RA_NOTIFICATION_BUSNAME, help="Name of the notification bus on which messages are published, default: %default")
    parser.add_option("--notification_subject", dest="notification_subject", type="string", default=RA_NOTIFICATION_PREFIX+'TaskUpdated', help="Subject of the published messages to listen for, default: %default")
    parser.add_option("--radb_busname", dest="radb_busname", type="string", default=RADB_BUSNAME, help="Name of the bus on which the RADB service listens, default: %default")
    parser.add_option("--radb_servicename", dest="radb_servicename", type="string", default=RADB_SERVICENAME, help="Name of the RADB service, default: %default")
    parser.add_option("--otdb_busname", dest="otdb_busname", type="string", default=DEFAULT_OTDB_SERVICE_BUSNAME, help="Name of the bus on which the OTDB service listens, default: %default")
    parser.add_option("--otdb_servicename", dest="otdb_servicename", type="string", default=DEFAULT_OTDB_SERVICENAME, help="Name of the OTDB service, default: %default")
    parser.add_option("--mom_busname", dest="mom_busname", type="string", default=DEFAULT_MOMQUERY_BUSNAME, help="Name of the bus on which the MoM service listens, default: %default")
    parser.add_option("--mom_servicename", dest="mom_servicename", type="string", default=DEFAULT_MOMQUERY_SERVICENAME, help="Name of the MoM service, default: %default")
    parser.add_option('-V', '--verbose', dest='verbose', action='store_true', help='verbose logging')
    (options, args) = parser.parse_args()

    logging.getLogger('lofar.sas.resourceassignment.database.radbbuslistener').level = logging.WARN
    setQpidLogLevel(logging.INFO)
    logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s',
                        level=logging.DEBUG if options.verbose else logging.INFO)

    with RAtoOTDBPropagator(radb_busname=options.radb_busname,
                            radb_servicename=options.radb_servicename,
                            otdb_busname=options.otdb_busname,
                            otdb_servicename=options.otdb_servicename,
                            mom_busname=options.mom_busname,
                            mom_servicename=options.mom_servicename,
                            broker=options.broker) as propagator:
        with RATaskStatusChangedListener(busname=options.notification_busname,
                                         subject=options.notification_subject,
                                         broker=options.broker,
                                         propagator=propagator) as listener:
            waitForInterrupt()

if __name__ == '__main__':
    main()
