    // This file is generated automatically -- do not edit
    // Generated by /home/oms/LOFAR/autoconf_share/../DMI/src/build_aid_maps.pl
    #include <DMI/AtomicID.h>
    #include <DMI/TypeInfo.h>
    #include <DMI/DynamicTypeManager.h>
    #include <DMI/Packer.h>
    
#include "Add.h"
BlockableObject * __construct_MeqAdd (int n) { return n>0 ? new Meq::Add [n] : new Meq::Add; }
#include "Conj.h"
BlockableObject * __construct_MeqConj (int n) { return n>0 ? new Meq::Conj [n] : new Meq::Conj; }
#include "Cos.h"
BlockableObject * __construct_MeqCos (int n) { return n>0 ? new Meq::Cos [n] : new Meq::Cos; }
#include "Divide.h"
BlockableObject * __construct_MeqDivide (int n) { return n>0 ? new Meq::Divide [n] : new Meq::Divide; }
#include "Exp.h"
BlockableObject * __construct_MeqExp (int n) { return n>0 ? new Meq::Exp [n] : new Meq::Exp; }
#include "Function.h"
BlockableObject * __construct_MeqFunction (int n) { return n>0 ? new Meq::Function [n] : new Meq::Function; }
#include "Multiply.h"
BlockableObject * __construct_MeqMultiply (int n) { return n>0 ? new Meq::Multiply [n] : new Meq::Multiply; }
#include "Node.h"
BlockableObject * __construct_MeqNode (int n) { return n>0 ? new Meq::Node [n] : new Meq::Node; }
#include "ParmPolcStored.h"
BlockableObject * __construct_MeqParmPolcStored (int n) { return n>0 ? new Meq::ParmPolcStored [n] : new Meq::ParmPolcStored; }
#include "Pow.h"
BlockableObject * __construct_MeqPow (int n) { return n>0 ? new Meq::Pow [n] : new Meq::Pow; }
#include "Sin.h"
BlockableObject * __construct_MeqSin (int n) { return n>0 ? new Meq::Sin [n] : new Meq::Sin; }
#include "Sqr.h"
BlockableObject * __construct_MeqSqr (int n) { return n>0 ? new Meq::Sqr [n] : new Meq::Sqr; }
#include "Sqrt.h"
BlockableObject * __construct_MeqSqrt (int n) { return n>0 ? new Meq::Sqrt [n] : new Meq::Sqrt; }
#include "Subtract.h"
BlockableObject * __construct_MeqSubtract (int n) { return n>0 ? new Meq::Subtract [n] : new Meq::Subtract; }
#include "ToComplex.h"
BlockableObject * __construct_MeqToComplex (int n) { return n>0 ? new Meq::ToComplex [n] : new Meq::ToComplex; }
  
    int aidRegistry_Meq ()
    {
      static int res = 

        AtomicID::registerId(-1472,"MeqAdd")+
        TypeInfoReg::addToRegistry(-1472,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1472,__construct_MeqAdd)+
        AtomicID::registerId(-1144,"Domain")+
        AtomicID::registerId(-1484,"Nfreq")+
        AtomicID::registerId(-1483,"Times")+
        AtomicID::registerId(-1485,"TimeSteps")+
        AtomicID::registerId(-1487,"MeqConj")+
        TypeInfoReg::addToRegistry(-1487,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1487,__construct_MeqConj)+
        AtomicID::registerId(-1476,"MeqCos")+
        TypeInfoReg::addToRegistry(-1476,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1476,__construct_MeqCos)+
        AtomicID::registerId(-1474,"MeqDivide")+
        TypeInfoReg::addToRegistry(-1474,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1474,__construct_MeqDivide)+
        AtomicID::registerId(-1475,"MeqExp")+
        TypeInfoReg::addToRegistry(-1475,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1475,__construct_MeqExp)+
        AtomicID::registerId(-1491,"MeqFunction")+
        TypeInfoReg::addToRegistry(-1491,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1491,__construct_MeqFunction)+
        AtomicID::registerId(-1466,"MeqMultiply")+
        TypeInfoReg::addToRegistry(-1466,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1466,__construct_MeqMultiply)+
        AtomicID::registerId(-1427,"Node")+
        AtomicID::registerId(-1481,"Class")+
        AtomicID::registerId(-1163,"Name")+
        AtomicID::registerId(-1052,"State")+
        AtomicID::registerId(-1423,"Child")+
        AtomicID::registerId(-1431,"Children")+
        AtomicID::registerId(-1351,"Request")+
        AtomicID::registerId(-1421,"Result")+
        AtomicID::registerId(-1482,"Rider")+
        AtomicID::registerId(-1457,"Id")+
        AtomicID::registerId(-1470,"MeqNode")+
        TypeInfoReg::addToRegistry(-1470,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1470,__construct_MeqNode)+
        AtomicID::registerId(-1488,"Tablename")+
        AtomicID::registerId(-1327,"Default")+
        AtomicID::registerId(-1471,"MeqParmPolcStored")+
        TypeInfoReg::addToRegistry(-1471,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1471,__construct_MeqParmPolcStored)+
        AtomicID::registerId(-1465,"MeqPow")+
        TypeInfoReg::addToRegistry(-1465,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1465,__construct_MeqPow)+
        AtomicID::registerId(-1478,"Cells")+
        AtomicID::registerId(-1486,"ReqId")+
        AtomicID::registerId(-1480,"CalcDeriv")+
        AtomicID::registerId(-1396,"Values")+
        AtomicID::registerId(-1479,"ParmValues")+
        AtomicID::registerId(-1490,"Spids")+
        AtomicID::registerId(-1473,"Perturbations")+
        AtomicID::registerId(-1469,"MeqSin")+
        TypeInfoReg::addToRegistry(-1469,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1469,__construct_MeqSin)+
        AtomicID::registerId(-1489,"MeqSqr")+
        TypeInfoReg::addToRegistry(-1489,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1489,__construct_MeqSqr)+
        AtomicID::registerId(-1477,"MeqSqrt")+
        TypeInfoReg::addToRegistry(-1477,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1477,__construct_MeqSqrt)+
        AtomicID::registerId(-1467,"MeqSubtract")+
        TypeInfoReg::addToRegistry(-1467,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1467,__construct_MeqSubtract)+
        AtomicID::registerId(-1468,"MeqToComplex")+
        TypeInfoReg::addToRegistry(-1468,TypeInfo(TypeInfo::DYNAMIC,0))+
        DynamicTypeManager::addToRegistry(-1468,__construct_MeqToComplex)+
    0;
    return res;
  }
  
  int __dum_call_registries_for_Meq = aidRegistry_Meq();

