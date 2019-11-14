/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
void
CirMgr::strash()
{
	HashMap<faninKey, CirGate*> hashMap(_dfsList.size());
	for(size_t i=0 ; i<_dfsList.size() ; ++i){
		if(_dfsList[i]->getTypeStr()!="AIG") continue; 
		faninKey key(_dfsList[i]->getfaninV(0), _dfsList[i]->getfaninV(1));
		if(!hashMap.insert(key, _dfsList[i])){
			CirGate* mergeGate;
			hashMap.query(key, mergeGate);
			str_merge(_dfsList[i], mergeGate);
		}
	}
	dfsTraversal();
}

void
CirMgr::str_merge(CirGate * dgate, CirGate * mgate){
   _Andgate--;
   _totalList[dgate->getId()]=0;
   for(int i=0, n=dgate->faninNum(); i<n ; ++i){
      dgate->getfanin(i)->deletefanout(dgate);
   }
   for(int i=0, n=dgate->fanoutNum(); i<n ; ++i){
      if(!dgate->fanoutIsInv(i)){
         dgate->getfanout(i)->chagefanin(dgate, mgate, 0);
         mgate->addfanout(dgate->getfanout(i), 0);
      }
      else{
         dgate->getfanout(i)->chagefanin(dgate, mgate, 1);
         mgate->addfanout(dgate->getfanout(i), 1);
      }
   }
   cout<<"Strashing: "<<mgate->getId()<<" merging "<<dgate->getId()<<"..."<<endl;
   delete dgate;
}

void
CirMgr::fraig()
{
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
