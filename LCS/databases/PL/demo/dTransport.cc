//#  dTransport.cc: demo program showing the use of PL with complex classes.
//#
//#  Copyright (C) 2002-2004
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
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
//#  $Id$

#include "Transport.h"
#include "PO_Transport.h"
#include <PL/TPersistentObject.h>
#include <PL/PersistenceBroker.h>
#include <PL/Query.h>
#include <PL/Attrib.h>
#include <Common/Debug.h>
#include <iostream>

using namespace LOFAR::PL;
using namespace std;

//
// This demo program shows how the Persistence Layer can be used to save and
// restore complex classes. By complex we mean: classes that contain one or
// more non-primitive data members and/or are derived from a base class.
//
int main (int argc, const char* argv[]) 
{
  Debug::initLevels(argc, argv);

  Collection<Tractor> tractors;
  tractors.add(Tractor("Yamaha", true, 3.0));
  tractors.add(Tractor("Suzuki", true, 2.4));
  tractors.add(Tractor("Yamaha", false, 2.0));

  Collection<MotorCycle> motorCycles;
  motorCycles.add(MotorCycle("Honda", 12, 2.0));
  motorCycles.add(MotorCycle("Suzuki", 8, 0.75));
  motorCycles.add(MotorCycle("Yamaha", 16, 1.6));
  motorCycles.add(MotorCycle("Suzuki", 12, 1.0));

  // This is the STL way of printing the contents of a container into an
  // output stream; you don't need a for-loop.
  cout << endl << "Here are our Vehicle objects:" << endl;
  copy(tractors.begin(), tractors.end(), 
       ostream_iterator<Tractor>(cout,"\n"));
  copy(motorCycles.begin(), motorCycles.end(), 
       ostream_iterator<MotorCycle>(cout,"\n"));

  cout << "Press <Return> to continue";
  cin.get();

  // Add the tractors to the TPO container for tractors.
  Collection<TPersistentObject<Tractor> > poTractors;
  for(Collection<Tractor>::iterator it = tractors.begin(); 
      it != tractors.end(); ++it) {
    poTractors.add(TPersistentObject<Tractor>(*it));
  }

  // Add the motorcycles to the TPO container for motorcycles.
  Collection<TPersistentObject<MotorCycle> > poMotorCycles;
  for(Collection<MotorCycle>::iterator it = motorCycles.begin(); 
      it != motorCycles.end(); ++it) {
    poMotorCycles.add(TPersistentObject<MotorCycle>(*it));
  }

  // Create a PersistenceBroker object. We will communicate with the database
  // through this broker object.
  PersistenceBroker broker;

  // Connect to the database. 
  try {
    broker.connect ("test", "postgres");
    cout << "Connected to database." << endl;
  }
  catch (LOFAR::Exception& e) {
    cerr << e << endl;
    return 1;
  }
  
  // Save the tractors and motorcycles to the database, using their "TPO"
  // containers.
  try {
    broker.save (poTractors);
    broker.save(poMotorCycles);
    cout << "Saved data." << endl;
  }
  catch (LOFAR::Exception & e) {
    cerr << e << endl;
    return 1;
  }

  // Retrieve all MotorCycle objects that have a 12-valve engine; we should
  // modify the valve-count, because they're actually 16-valve engines.
  try {

    cout << endl << "Retrieving all MotorCycles with a 12-valve engine ..."
         << endl;

    // The result of a query is returned in a Collection of "TPOs".
    Collection< TPersistentObject<MotorCycle> > results;

    // Construct the query expression.
    Query::Expr expr = attrib<MotorCycle>("engine.valveCount") == 12;
    
    // Make the query and store the result in \a results.
    results = broker.retrieve<MotorCycle> (expr);

    // Show the retrieved MotorCyles on standard output.
    cout << "Retrieved " << results.size () << " object(s)." << endl;
    Collection< TPersistentObject<MotorCycle> >::const_iterator cit;
    for (cit = results.begin(); cit != results.end(); ++cit) {
      cout << cit->data() << endl;
    }

    // Oops! The Honda motorcyle actually has a 16-valves engine. Let's update
    // the database, using the Collection that we just retrieved.
    cout << "Oops! The Honda motorcycle actually has a 16-valves engine" 
         << endl << "Let's update the database ..." << endl
         << " -- Press <Return> to continue";
    cin.get();

    // We can easily access our MotorCycle objects, using the data() method of
    // the TPO.
    Collection< TPersistentObject<MotorCycle> >::iterator it;
    for (it = results.begin(); it != results.end(); ++it) {
      if (it->data().getMake() == "Honda") {
        it->data().setValveCount(16);
        broker.save(*it);
      }
    }
    cout << "Updated database" << endl;

    // Let's check if all our 12-valve engines have been properly upgraded. We
    // do this by searching for 12-valve engines; this query should return
    // zero matches.
    cout << endl << "Retrieving all MotorCycles with a 12-valve engine ..."
         << endl;

    // Re-execute the query for 12-valve engines
    results = broker.retrieve<MotorCycle> (expr);
    
    // Show the retrieved MotorCyles on standard output.
    cout << "Retrieved " << results.size () << " object(s)." << endl;
    for (cit = results.begin(); cit != results.end(); ++cit) {
      cout << cit->data() << endl;
    }

  }
  catch (LOFAR::Exception & e) {
    cerr << e << endl;
    return 1;
  }

  cout << "Press <Return> to continue";
  cin.get();

  // Retrieve all Tractors that were produced by Yamaha and have an engine
  // volume of more than 2.0 litres and erase them from the database.
  try {
    cout << endl << "Retrieving all tractors produced by Yamaha, " << endl
         << "  with an enigne volume of more than 2.0 litres" << endl;

    // The result of a query is returned in a Collection of "TPOs".
    Collection< TPersistentObject<Tractor> > results;

    // Construct the query expression.
    Query::Expr expr = 
      attrib<Tractor>("Vehicle::make") == "Yamaha" &&
      attrib<Tractor>("engine.Engine::volume") > 2.0;

    // Make the query and store the result in \a results.
    results = broker.retrieve<Tractor> (expr);

    // Show the retrieved Tractors on standard output.
    cout << "Retrieved " << results.size () << " object(s)." << endl;
    Collection< TPersistentObject<Tractor> >::const_iterator cit;
    for (cit = results.begin(); cit != results.end(); ++cit) {
      cout << cit->data() << endl;
    }

    cout << "Press <Return> to continue";
    cin.get();

    cout << "Erase all the tractors that matched the previous query ..." 
         << endl;

    // Erase all the tractors in the Collection.
    broker.erase(results);

    // Check that they were really erased from the database by re-executing
    // the query.
    results = broker.retrieve<Tractor> (expr);

    cout << "Retrieved " << results.size() << " object(s)." << endl;
    Assert(results.empty());

  }
  catch (LOFAR::Exception & e) {
    cerr << e << endl;
    return 1;
  }

return 0;
}
