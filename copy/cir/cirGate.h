/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stddef.h>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class GateV;
class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class GateV {
public:
   #define NEG 0x1
   GateV(CirGate* g, size_t phase) : _gateV(size_t(g) + phase) { } 

   CirGate* gate() const { return (CirGate*)(_gateV & ~size_t(NEG)); } 
   bool isInv() const { return (_gateV & NEG); }

   size_t _gateV;
};

class CirGate
{
public:
   CirGate(unsigned d, unsigned l=0) : _id(d), _lineNo(l), _ref(0), _indfs(false), _value(0){}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const=0;
   unsigned getLineNo() const { return _lineNo; }
   unsigned getId() const { return _id; }
   virtual bool isAig() const { return false; }
   virtual string getName() const { return ""; }
   virtual bool haveName() const { return false; }
   size_t fanoutNum() const { return _fanoutList.size(); }
   size_t faninNum() const { return _faninList.size(); }
   CirGate* getfanin(int i);
   CirGate* getfanout(int i);
   bool faninIsInv(int i);
   bool fanoutIsInv(int i);
   size_t getfaninV(int i);
   size_t getfanoutV(int i);


   // Printing functions
   virtual void printGate() const = 0;  // Using by printNetlist
   void reportGate() const;
   void reportFanin(int level) const;
   void printFanin(int level, int space, bool isInvert) const;
   void reportFanout(int level) const;
   void printFanout(int level, int space, bool isInvert) const;

   // Read aag functions
   void addfanin(CirGate* faninG, size_t phase){ _faninList.push_back(GateV(faninG, phase));}
   void addfanout(CirGate* fanoutG, size_t phase){ _fanoutList.push_back(GateV(fanoutG, phase));}
   virtual void addName(string n) {}

   // Fanin fanout chage functions
   void deletefanin(int i);
   void deletefanin(CirGate * faningate);
   void deletefanout(int i);
   void deletefanout(CirGate * fanoutgate);
   void chagefanin(CirGate* oldfanin, CirGate* newfanin, size_t phase);
   void chagefanout(CirGate* oldfanout, CirGate* newfanout, size_t phase);

   // Sort Fanout List
   void sortFanout(){ sort(_fanoutList.begin(), _fanoutList.end(), compareFanout); }
   static bool compareFanout(GateV & V1, GateV & V2) { return (V1.gate()->getId() < V2.gate()->getId()); }

   //DFS
   void dfsTraversal(vector<CirGate *>& dfsList);
   static void setGlobalRef(){ _globalRef++; }
   static void resetGlobalRef(){ _globalRef=0; }
   bool isindfs(){ return(_indfs); }

   //Simulation
   void setValue(size_t v){ _ref=_globalRef; _value = v; }
   size_t getValue(){ return _value; }
   virtual bool simulate(bool& _simFirst);
   virtual void setGrp(size_t g){}
   virtual size_t getGrp() const {return SIZE_MAX;}
   virtual void removeGrp(){}


protected:
   vector<GateV> _faninList;
   vector<GateV> _fanoutList;
   unsigned _id;
   unsigned _lineNo;
   bool _indfs;
   static unsigned _globalRef;
   mutable unsigned _ref;
   size_t _value;

};


class UndefGate : public CirGate{
public:
   UndefGate(unsigned d, unsigned l) : CirGate(d, 0){}

   virtual string getTypeStr() const { return "UNDEF";}
   virtual void printGate() const{}
   virtual bool simulate(bool& _simFirst){ 
      if(_simFirst) return true; 
      else return false;
   }

};

class PiGate : public CirGate{
public:
   PiGate(unsigned d, unsigned l) : CirGate(d, l), _name(""){}

   virtual string getTypeStr() const { return "PI";}
   virtual string getName() const { return _name; }
   virtual bool haveName() const { return (_name!=""); }
   virtual void addName(string n){ _name = n; }
   virtual void printGate() const{
      cout<<"PI  "<<_id;
      if(_name!=""){ cout<<" ("<<_name<<")"; }
      cout<<endl;
   }
   virtual bool simulate(bool& _simFirst){ return true; }

protected:
   string _name;

};

class PoGate : public CirGate{
public:
   PoGate(unsigned d, unsigned l) : CirGate(d, l), _name(""){}

   virtual string getTypeStr() const { return"PO"; }
   virtual string getName() const { return _name; }
   virtual bool haveName() const { return (_name!=""); }
   virtual void addName(string n){ _name = n; }
   virtual void printGate() const{
      cout<<"PO  "<<_id<<" ";
      if(_faninList[0].gate()->getTypeStr()=="UNDEF") cout<<"*";
      if(_faninList[0].isInv()) cout<<"!";
      cout<<_faninList[0].gate()->getId();
      if(_name!=""){ cout<<" ("<<_name<<")"; }
      cout<<endl;
   }
   virtual bool simulate(bool& _simFirst){
      string faninType = _faninList[0].gate()->getTypeStr();
      if(faninType!="CONST" && faninType!="PI"){
         if(!_faninList[0].gate()->simulate(_simFirst)){ if(!_simFirst) return false; }
      }
      if(_faninList[0].isInv()){ _value = ~_faninList[0].gate()->getValue(); }
      else{ _value = _faninList[0].gate()->getValue(); }
      return true;
   }

protected:
   string _name;

};

class AigGate : public CirGate{
public:
   AigGate(unsigned d, unsigned l) : CirGate(d, l), _grp(SIZE_MAX){}

   virtual string getTypeStr() const { return"AIG"; }
   virtual bool isAig() const { return true; }
   virtual void printGate() const{
      cout<<"AIG "<<_id;
      for(int i=0 ; i<_faninList.size() ; ++i){
         cout<<" ";
         if(_faninList[i].gate()->getTypeStr()=="UNDEF") cout<<"*";
         if(_faninList[i].isInv()) cout<<"!";
         cout<<_faninList[i].gate()->getId();
      }
      cout<<endl;
   }
   virtual void setGrp(size_t g){ _grp = g; }
   virtual size_t getGrp() const { return _grp;}
   virtual void removeGrp(){ _grp = SIZE_MAX; }

protected:
   size_t _grp;
};

class ConstGate : public CirGate{
public:
   ConstGate(unsigned d, unsigned l) : CirGate(0, 0){ _value=0; }

   virtual string getTypeStr() const { return"CONST";}
   virtual void printGate() const{ cout<<"CONST0"<<endl; }
   virtual bool simulate(bool& _simFirst){ 
      if( _simFirst) return true;
      else return false;
   }

};

#endif // CIR_GATE_H
