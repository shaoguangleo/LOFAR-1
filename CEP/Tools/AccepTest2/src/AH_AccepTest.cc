//#  AH_AccepTest.cc: Application holder for acceptance test 2
//#
//#  Copyright (C) 2000, 2001
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, swe@astron.nl
//#
//#
//#  $Id$

//# Always #include <lofar_config.h> first!
#include <lofar_config.h>

#include <AccepTest2/WH_Random.h>
#include <AccepTest2/WH_Transpose.h>
#include <AccepTest2/WH_Dump.h>
#include <AccepTest2/WH_Heat.h>
#include <AccepTest2/WH_Split.h>
#include <AccepTest2/WH_Join.h>
#include <Common/LofarLogger.h>
#include <AH_AccepTest.h>
#include <Transport/TH_Mem.h>
#include <Transport/TH_MPI.h>
#include <Transport/Connection.h>


namespace LOFAR
{
  AH_AccepTest::AH_AccepTest():
    itsFileOutput(NULL)
  {};

  AH_AccepTest::~AH_AccepTest()
  {};

  void AH_AccepTest::define(const KeyValueMap& kvm)
  {
    undefine();

    // These variables can be chosen just the way you like them, EXCEPT:
    //     MatrixY*NoRndNodes must be a multiple of NoSplitsPerSplitter
    //     and of course NoSplitters <= NoHeatLines
    int NoRndNodes = kvm.getInt("RandomNodes", 16);
    int NoHeatLines = kvm.getInt("HeatLines", 16);
    int NoSplitters = kvm.getInt("Splitters", 2);
    int NoSplitsPerSplitter = kvm.getInt("HeatsPerSplit", 8);
    int MatrixX = kvm.getInt("RandomMatrixX", 13);
    int MatrixY = kvm.getInt("RandomMatrixY", 17);
    ASSERTSTR(NoSplitters<NoHeatLines, "There should be more heat lines than splitted heat lines.");

#ifdef HAVE_MPI

    int totalNumberOfNodes = ( NoSplitsPerSplitter + 2 ) * NoSplitters + 3 * NoHeatLines + NoRndNodes;
    int MPINodes = TH_MPI::getNumberOfNodes();
    if (totalNumberOfNodes>MPINodes) {
      cerr<<"This program was started with "<<TH_MPI::getNumberOfNodes()
	  <<" processes, but we need "<<totalNumberOfNodes<<" processes. Aborting"<<endl;
      exit(1);
    } else if (totalNumberOfNodes<MPINodes) {
      cerr<<"This program was started with "<<TH_MPI::getNumberOfNodes()
	  <<" processes, but we need only "<<totalNumberOfNodes<<"."<<endl;
    };
#endif
    
    // you can choose the output for the WH_Dumps here
    // of course this can also be done per workholder
    ostream& myOutput = cout;
    itsFileOutput = new fstream("/dev/null", fstream::out);
    //ostream& myOutput = *itsFileOutput;

    vector<WH_Random*> RandomNodes;
    vector<WH_Transpose*> TransposeNodes;
    vector<WH_Dump*> DumpNodes;
    vector<WH_Heat*> Heat1Nodes;
    vector<WH_Split*> SplitNodes;
    vector<WH_Heat*> Heat2Nodes;
    vector<WH_Join*> JoinNodes;

    LOG_TRACE_FLOW("Start construction of WorkHolders");
    int lowestFreeNode = 0;
    // create workholders
    char WH_DH_Name[128];
    for (int i=0; i<NoRndNodes; i++) {
      // random nodes
      sprintf(WH_DH_Name, "Random_generator_%d_of_%d", i, NoRndNodes);
      RandomNodes.push_back(new WH_Random(WH_DH_Name,
					  NoHeatLines,
					  MatrixX,
					  MatrixY));
      itsWHs.push_back((WorkHolder*) RandomNodes[i]);
      itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      
    };
    for (int i=0; i<NoHeatLines; i++) {
      // transpose nodes
      sprintf(WH_DH_Name, "Transpose_%d_of_%d", i, NoHeatLines);
      TransposeNodes.push_back(new WH_Transpose(WH_DH_Name,
						NoRndNodes,
						MatrixX,
						MatrixY));
      itsWHs.push_back((WorkHolder*) TransposeNodes[i]);
      itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      

      // heat nodes
      sprintf(WH_DH_Name, "Heat_%d_of_%d", i, NoHeatLines);
      Heat1Nodes.push_back(new WH_Heat(WH_DH_Name,
				       MatrixY*NoRndNodes,
				       MatrixX));
      itsWHs.push_back((WorkHolder*) Heat1Nodes[i]);
      itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      
    };

    for (int Si=0; Si<NoSplitters; Si++) {
      // split nodes
      sprintf(WH_DH_Name, "Split_%d_of_%d", Si, NoSplitters);
      SplitNodes.push_back(new WH_Split(WH_DH_Name,
					NoSplitsPerSplitter,
					MatrixY*NoRndNodes/NoSplitsPerSplitter,
					MatrixX));
      itsWHs.push_back((WorkHolder*) SplitNodes[Si]);
      itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      

      for (int Di=0; Di<NoSplitsPerSplitter; Di++) {
	// heat2 nodes
	sprintf(WH_DH_Name, "Heat2_%d_of_%d_on_line_%d_of_%d", Di, NoSplitsPerSplitter, Si, NoSplitters);
	Heat2Nodes.push_back(new WH_Heat(WH_DH_Name,
					 MatrixY*NoRndNodes/NoSplitsPerSplitter,
					 MatrixX,
					 false));
	itsWHs.push_back((WorkHolder*) Heat2Nodes[Si*NoSplitsPerSplitter+Di]);
	itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      
      }
      // join nodes
      sprintf(WH_DH_Name, "Join_%d_of_%d", Si, NoSplitters);
      JoinNodes.push_back(new WH_Join(WH_DH_Name,
				      NoSplitsPerSplitter,
				      MatrixY*NoRndNodes/NoSplitsPerSplitter,
				      MatrixX));
      itsWHs.push_back((WorkHolder*) JoinNodes[Si]);
      itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      
    }
    // create dump nodes last so they process will be called on them last
    for (int i=0; i<NoHeatLines; i++) {
      // dump nodes
      sprintf(WH_DH_Name, "Dump_%d_of_%d", i, NoHeatLines);
      DumpNodes.push_back(new WH_Dump(WH_DH_Name,
				      NoRndNodes*MatrixY,
				      MatrixX,
				      myOutput));
      itsWHs.push_back((WorkHolder*) DumpNodes[i]);
      itsWHs[itsWHs.size()-1]->runOnNode(lowestFreeNode++);      
    }



    //connect WorkHolders
    for (int Hi = 0; Hi < NoHeatLines; Hi++) {
      // random nodes to transpose
      for (int Ri = 0; Ri < NoRndNodes; Ri++) {
	connectWHs(RandomNodes[Ri], Hi, TransposeNodes[Hi], Ri);
      }
      // transpose nodes to heat1
      connectWHs(TransposeNodes[Hi], 0, Heat1Nodes[Hi], 0);
    }
    for (int Si = 0; Si < NoSplitters; Si++) {
      // heat nodes to split
      connectWHs(Heat1Nodes[Si], 0, SplitNodes[Si], 0);
      for (int Hi = 0; Hi < NoSplitsPerSplitter; Hi++) {
	// split nodes to heat2
	connectWHs(SplitNodes[Si], Hi, Heat2Nodes[Si*NoSplitsPerSplitter+Hi], 0);
	// heat2 nodes to join
	connectWHs(Heat2Nodes[Si*NoSplitsPerSplitter+Hi], 0, JoinNodes[Si], Hi);
      }
      // dump nodes to the join
      connectWHs(JoinNodes[Si], 0, DumpNodes[Si], 0);
    }
    // connect the remaining Dump nodes directly to Heat1
    for (int Di = NoSplitters; Di < NoHeatLines; Di++) {
      connectWHs(Heat1Nodes[Di], 0, DumpNodes[Di], 0);
    }
    


  };

  void AH_AccepTest::undefine()
  {
    vector<WorkHolder*>::iterator it1 = itsWHs.begin();
    for (; it1!=itsWHs.end(); it1++) {
      delete *it1;
    }
    itsWHs.clear();

    vector<Connection*>::iterator it2 = itsConns.begin();
    for (; it2!=itsConns.end(); it2++) {
      delete *it2;
    }
    itsConns.clear();

    vector<TransportHolder*>::iterator it3 = itsTHs.begin();
    for (; it3!=itsTHs.end(); it3++) {
      delete *it3;
    }
    itsTHs.clear();

    if (itsFileOutput) delete itsFileOutput;
  }

  void AH_AccepTest::init() {
    // call preprocess method on all WH's
    vector<WorkHolder*>::iterator it = itsWHs.begin();
    for (; it!=itsWHs.end(); it++) {
      (*it)->basePreprocess();
    }
  }
  
  void AH_AccepTest::run(int nsteps) {
#ifdef HAVE_MPI
    TH_MPI::synchroniseAllProcesses();
#endif
    // call process method on all WH's
    int NoWHs = itsWHs.size();
    for (int s = 0; s < nsteps; s++) {
      for (int i = 0; i < NoWHs; i++) {
	itsWHs[i]->baseProcess();
      }
    }
  }

  void AH_AccepTest::connectWHs(WorkHolder* srcWH, int srcDH, WorkHolder* dstWH, int dstDH) {
#ifdef HAVE_MPI
    itsTHs.push_back( new TH_MPI(srcWH->getNode(), dstWH->getNode()) );
    itsConns.push_back( new Connection("conn", 
				       srcWH->getDataManager().getOutHolder(srcDH),
				       dstWH->getDataManager().getInHolder(dstDH),
				       itsTHs.back(), true) );
#else
    itsTHs.push_back( new TH_Mem ); 
    itsConns.push_back( new Connection("conn", 
				       srcWH->getDataManager().getOutHolder(srcDH),
				       dstWH->getDataManager().getInHolder(dstDH),
				       itsTHs.back(), false) );
#endif

    srcWH->getDataManager().setOutConnection(srcDH, itsConns.back());
    dstWH->getDataManager().setInConnection(dstDH, itsConns.back());

  }
  
  void AH_AccepTest::quit() {
#ifdef HAVE_MPI
    TH_MPI::finalize();
#endif
  }
}
