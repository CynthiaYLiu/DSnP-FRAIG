/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <string>
#include <bitset>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;
unsigned CirGate::_globalRef=0;

/*******************************************************/
/*   class CirGate member functions for basic access   */
/*******************************************************/
CirGate* 
CirGate::getfanin(int i){ 
   if(_faninList.size()>i) return _faninList[i].gate();
   else return 0;
}

CirGate*
CirGate::getfanout(int i) { 
   if(_fanoutList.size()>i) return _fanoutList[i].gate();
   else return 0; 
}

bool 
CirGate::faninIsInv(int i) { 
   if(_faninList.size()>i) return _faninList[i].isInv();
   else return 0;
}

bool
CirGate::fanoutIsInv(int i) { 
   if(_fanoutList.size()>i) return _fanoutList[i].isInv();
   else return 0; 
}

size_t 
CirGate::getfaninV(int i){
	if(_faninList.size()>i) return _faninList[i]._gateV;
   else return 0; 
}

size_t 
CirGate::getfanoutV(int i){
	if(_fanoutList.size()>i) return _fanoutList[i]._gateV;
   else return 0; 
}


/*******************************************************/
/*    class CirGate member functions for printing      */
/*******************************************************/
void
CirGate::reportGate() const
{
	cout<<"================================================================================"<<endl;
	cout<<"= " << getTypeStr() << "(" << to_string(_id) << ")";
	if(getName()!=""){ 
		cout<<"\"" << getName() << "\"";
	}
	cout<<", line "<<to_string(_lineNo)<<endl;
	cout<<"= FECs:";
	if(getGrp()!=SIZE_MAX){
		vector<unsigned> grp = cirMgr->sendGrp(getGrp());
		for(size_t i=0 ; i<grp.size() ; ++i){
			if(grp[i] == _id) continue;
			cout<<" ";
			if(cirMgr->getGate(grp[i])->getValue()!=_value){ cout<<"!"; }
			cout<<grp[i];
		}
	}
	cout<<endl;
	bitset<SIZE_T> b = _value;
	cout<<"= Value: ";
	for(size_t i=0 ; i<SIZE_T ; ++i){
		if(i%8==0 && i!=0){cout<<"_";}
		cout<<b[SIZE_T-1-i];
	}
	cout<<endl<<"================================================================================"<<endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   printFanin(level, 0, false);
}

void
CirGate::printFanin(int level, int space, bool isInvert) const
{
	for(int i=0 ; i<space ; ++i){
		cout<<"  ";
	}
	if(isInvert) cout<<"!";
	cout<<getTypeStr()<<" "<<_id;
	if(_ref!=_globalRef){
		cout<<endl;
		if(level>0){
			for(int i=0 ; i<_faninList.size() ; ++i){
				_ref=_globalRef;
				_faninList[i].gate()->printFanin(level-1, space+1, _faninList[i].isInv());
			}
		}
	}
	else{
		cout<<" (*)"<<endl;
	}
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   printFanout(level, 0, false);
}

void
CirGate::printFanout(int level, int space, bool isInvert) const
{
	for(int i=0 ; i<space ; ++i){
		cout<<"  ";
	}
	if(isInvert) cout<<"!";
	cout<<getTypeStr()<<" "<<_id;
	if(_ref!=_globalRef){
		cout<<endl;
		if(level>0){
			for(int i=0 ; i<_fanoutList.size() ; ++i){
				_ref=_globalRef;
				_fanoutList[i].gate()->printFanout(level-1, space+1, _fanoutList[i].isInv());
			}
		}
	}
	else{
		cout<<" (*)"<<endl;
	}
}


/*******************************************************/
/*    class CirGate member functions for changing      */
/*******************************************************/
void 
CirGate::deletefanin(int i){ _faninList.erase(_faninList.begin()+i); }
void
CirGate::deletefanin(CirGate * faningate){
	for(int i=0 ; i<_faninList.size() ; ++i){
		if(_faninList[i].gate()==faningate){
			deletefanin(i);
			break;
		}
	}
}

void 
CirGate::deletefanout(int i){ _fanoutList.erase(_fanoutList.begin()+i); }
void
CirGate::deletefanout(CirGate * fanoutgate){
	for(int i=0 ; i<_fanoutList.size() ; ++i){
		if(_fanoutList[i].gate()==fanoutgate){
			deletefanout(i);
			break;
		}
	}
}

void
CirGate::chagefanin(CirGate* oldfanin, CirGate* newfanin, size_t phase){
	for(int i=0 ; i<_faninList.size() ; ++i){
		if(_faninList[i].gate()==oldfanin){
			_faninList[i]=GateV(newfanin, phase);
			break;
		}
	}
}

void
CirGate::chagefanout(CirGate* oldfanout, CirGate* newfanout, size_t phase){
	for(int i=0 ; i<_fanoutList.size() ; ++i){
		if(_fanoutList[i].gate()==oldfanout){
			_fanoutList[i]=GateV(newfanout, phase);
			break;
		}
	}
}

/*******************************************************/
/*       class CirGate member functions for DFS        */
/*******************************************************/
void 
CirGate::dfsTraversal(vector<CirGate *>& dfsList)
{
	if(getTypeStr()!="UNDEF"){
		if(getTypeStr()!="PI" && getTypeStr()!="CONST"){
			for(int i=0 ; i<_faninList.size() ; ++i){
				if(getfanin(i)->_ref!=_globalRef){
					getfanin(i)->_ref=_globalRef;
					getfanin(i)->dfsTraversal(dfsList);
				}
			}
		}
		_indfs = true;
		dfsList.push_back(this);
	}
	else{ _indfs = true; }
}

/*******************************************************/
/*    class CirGate member functions for Simulation    */
/*******************************************************/
bool 
CirGate::simulate(bool& _simFirst)
{
	size_t simValue = 0;
	bool faninIsChange = false;
	for(size_t i=0 ; i<_faninList.size() ; ++i){
		string faninType = _faninList[i].gate()->getTypeStr();
		if(faninType!="CONST" && faninType!="PI" && _faninList[i].gate()->_ref!=_globalRef){
			if(_faninList[i].gate()->simulate(_simFirst)){ faninIsChange = true; }
		}
		else{ faninIsChange = true; }
	}
	if(!_simFirst){ if(!faninIsChange){ _ref = _globalRef; return false; }}
	if(_faninList[0].isInv()){
		simValue = ~_faninList[0].gate()->getValue();
	}
	else{
		simValue = _faninList[0].gate()->getValue();
	}

	if(_faninList[1].isInv()){
		_value = simValue & (~_faninList[1].gate()->getValue());
	}
	else{
		_value =  simValue & (_faninList[1].gate()->getValue());
	}
	_ref = _globalRef;
	if(!_simFirst){ if(simValue == _value){ return false; }}
	return true;
}
