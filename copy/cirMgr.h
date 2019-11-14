/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <bitset>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.
#define SIZE_T  (8*sizeof(size_t))

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
typedef  vector<unsigned>  FECGrp;

public:
   CirMgr() : _Max(0), _Pi(0), _Latch(0), _Po(0), _Andgate(0),_simFirst(true) {}
   ~CirMgr() {
      int totelsize = _totalList.size();
      for(int i=0 ; i<totelsize ; ++i){
         delete _totalList[i];
      }
   } 

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { if(gid<=_Max+_Po){ return _totalList[gid]; } return 0; } 

   // Member functions about circuit construction
   bool readCircuit(const string&);
   void findFltInandNotUse();

   // Member functions about circuit optimization
   void sweep();
   void optimize();
   void opt_merge(CirGate * mgate, CirGate * newgate, bool inv);

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }
   void simulate(bitset<SIZE_T> ParalPattern[]);

   // Member functions about FEC group
   void initFECGrps();
   void splitFECGrps();
   void sortAndRecord();
   static bool sortFECGrp(FECGrp i,FECGrp j){ return (i[0]<j[0]); }
   void writeLogFile(size_t num, bitset<SIZE_T> ParalPattern[]);

   // Member functions about fraig
   void strash();
   void str_merge(CirGate * dgate, CirGate * mgate);
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates()const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;


   // Member functions about DFS
   void dfsTraversal();

private:
   ofstream          *_simLog;

   bool              _simFirst;
   vector<FECGrp>    _FECGrps;
   vector<CirGate *> _totalList;
   vector<CirGate *> _piList;
   vector<CirGate *> _flfaninList;
   vector<CirGate *> _notusedList;
   vector<CirGate *> _dfsList;

   int _Max;
   int _Pi;
   int _Latch;
   int _Po;
   int _Andgate;

};

#endif // CIR_MGR_H
