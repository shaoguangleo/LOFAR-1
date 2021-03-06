#!/usr/bin/env python

# Be able to find service python file
import sys, os
sys.path.insert(0, "{srcdir}/../src".format(**os.environ))

from RATaskSpecified import *
from RABusListener import RATaskSpecifiedBusListener
from lofar.parameterset import PyParameterSet
from lofar.messaging import EventMessage, Service
from lofar.common.methodtrigger import MethodTrigger

import unittest
from glob import glob
import uuid
import datetime

import logging
logging.basicConfig(stream=sys.stdout, level=logging.INFO)

def setUpModule():
  pass

def tearDownModule():
  pass


class TestGetPredecessors(unittest.TestCase):
  def test_0_predecessors(self):
    parset = { PARSET_PREFIX + "Observation.Scheduler.predecessors": "[]" }

    self.assertEqual(predecessors(parset), [])

  def test_1_predecessor(self):
    parset = { PARSET_PREFIX + "Observation.Scheduler.predecessors": "[L426528]" }

    self.assertEqual(predecessors(parset), [426528])

  def test_2_predecessors(self):
    parset = { PARSET_PREFIX + "Observation.Scheduler.predecessors": "[L426528,L1]" }

    self.assertEqual(sorted(predecessors(parset)), [1,426528])


def parset_as_dict(filename):
    parset = PyParameterSet(filename, False)
    d = {}
    for k in parset.keywords():
      d[k] = parset.getString(k)

    return d


class TestResourceIndicators(unittest.TestCase):
  """
    The spec for the resource indicators is a draft at this point,
    and the output is quite extensive (many parset keys), so
    verification of the output is pending.
  """

  def test_preprocessing_pipeline(self):
    parset = parset_as_dict("tRATaskSpecified.in_preprocessing")
    r = resourceIndicatorsFromParset(parset)

  def test_correlator_observation(self):
    parset = parset_as_dict("tRATaskSpecified.in_correlator")
    r = resourceIndicatorsFromParset(parset)


class TestService(unittest.TestCase):
  def setUp(self):
    # Create a random bus
    self.busname = "%s-%s" % (sys.argv[0], str(uuid.uuid4())[:8])
    self.bus = ToBus(self.busname, { "create": "always", "delete": "always", "node": { "type": "topic" } })
    self.bus.open()
    self.addCleanup(self.bus.close)

    # Define the services we use
    self.status_service = "%s/TaskStatus" % (self.busname,)

    # ================================
    # Setup mock parset service
    # ================================

    # Nr of parsets requested, to detect multiple requests for the same parset, or of superfluous parsets
    self.requested_parsets = 0

    def TaskSpecificationService( OtdbID ):
      if OtdbID == 1:
        predecessors = "[2,3]"
      elif OtdbID == 2:
        predecessors = "[3]"
      elif OtdbID == 3:
        predecessors = "[]"
      else:
        raise Exception("Invalid OtdbID: %s" % OtdbID)

      self.requested_parsets += 1

      return {
        "Version.number":                                     "1",
        PARSET_PREFIX + "Observation.ObsID":                  str(OtdbID),
        PARSET_PREFIX + "Observation.Scheduler.predecessors": predecessors,
      }

    parset_service = Service("TaskSpecification", TaskSpecificationService, busname=self.busname)
    parset_service.start_listening()
    self.addCleanup(parset_service.stop_listening)

    # ================================
    # Setup listener to catch result
    # of our service
    # ================================

    listener = RATaskSpecifiedBusListener(busname=self.busname)
    listener.start_listening()
    self.addCleanup(listener.stop_listening)

    self.trigger = MethodTrigger(listener, "onTaskSpecified")

  def testNoPredecessors(self):
    """
      Request the resources for a simulated obsid 3, with the following predecessor tree:

        3 requires nothing
    """
    with RATaskSpecified("OTDB.TaskSpecified", otdb_busname=self.busname, my_busname=self.busname) as jts:
      # Send fake status update
      with ToBus(self.status_service) as tb:
        msg = EventMessage(content={
          "treeID": 3,
          "state": "prescheduled",
          "time_of_change": datetime.datetime(2016,1,1),
        })
        tb.send(msg)

      # Wait for message to arrive
      self.assertTrue(self.trigger.wait())

      # Verify message
      self.assertEqual(self.trigger.args[0], 3)
      resourceIndicators = self.trigger.args[2]
      self.assertNotIn("1", resourceIndicators)
      self.assertNotIn("2", resourceIndicators)
      self.assertIn("3", resourceIndicators);

      # Make sure we only requested one parset
      self.assertEqual(self.requested_parsets, 1)

  def testPredecessors(self):
    """
      Request the resources for a simulated obsid 1, with the following predecessor tree:

        1 requires 2, 3
        2 requires 3
        3 requires nothing
    """

    with RATaskSpecified("OTDB.TaskSpecified", otdb_busname=self.busname, my_busname=self.busname) as jts:
      # Send fake status update
      with ToBus(self.status_service) as tb:
        msg = EventMessage(content={
          "treeID": 1,
          "state": "prescheduled",
          "time_of_change": datetime.datetime(2016,1,1),
        })
        tb.send(msg)

      # Wait for message to arrive
      self.assertTrue(self.trigger.wait())

      # Verify message
      self.assertEqual(self.trigger.args[0], 1)
      resourceIndicators = self.trigger.args[2]
      self.assertIn("1", resourceIndicators);
      self.assertIn("2", resourceIndicators);
      self.assertIn("3", resourceIndicators);

      # Make sure we only requested exactly three parsets
      self.assertEqual(self.requested_parsets, 3)

def main(argv):
  unittest.main(verbosity=2)

if __name__ == "__main__":
  # run all tests
  import sys
  main(sys.argv[1:])
