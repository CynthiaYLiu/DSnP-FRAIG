/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep(){
   for(int i=0 ; i<_totalList.size() ; ++i){
      if( _totalList[i]==0 ) continue;
      if(!_totalList[i]->isindfs()){
         if(_totalList[i]->getTypeStr()=="AIG"){ 
            _Andgate--;
            cout<<"Sweeping: AIG("<<i<<") removed..."<<endl;
            for(int j=0, n=_totalList[i]->faninNum(); j<n ; ++j){
               _totalList[i]->getfanin(j)->deletefanout(_totalList[i]);
            }
            for(int j=0, n=_totalList[i]->fanoutNum(); j<n ; ++j){
               _totalList[i]->getfanout(j)->deletefanin(_totalList[i]);
            }
            delete _totalList[i];
            _totalList[i]=0;
            continue;
         }
         if(_totalList[i]->getTypeStr()=="UNDEF"){ 
            cout<<"Sweeping: UNDEF("<<i<<") removed..."<<endl;
            for(int j=0, n=_totalList[i]->fanoutNum(); j<n ; ++j){
               _totalList[i]->getfanout(j)->deletefanin(_totalList[i]);
            }
            delete _totalList[i];
            _totalList[i]=0;
         }
      }
   }
   findFltInandNotUse();
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize(){
   for(size_t i=0 ; i<_dfsList.size() ; ++i){
      if(_dfsList[i]->getTypeStr()!="AIG")continue;

      //CASE(a)(b) : one of the fanins is CONST
      if(_dfsList[i]->getfanin(0)->getTypeStr()=="CONST"){
         if(!_dfsList[i]->faninIsInv(0)){
            opt_merge(_dfsList[i], _dfsList[i]->getfanin(0), false);
            continue;
         }
         else{
            opt_merge(_dfsList[i], _dfsList[i]->getfanin(1), _dfsList[i]->faninIsInv(1));
            continue;
         }
      }
      if(_dfsList[i]->getfanin(1)->getTypeStr()=="CONST"){
         if(!_dfsList[i]->faninIsInv(1)){
            opt_merge(_dfsList[i], _dfsList[i]->getfanin(1), false);
            continue;
         }
         else{
            opt_merge(_dfsList[i], _dfsList[i]->getfanin(0), _dfsList[i]->faninIsInv(0));
            continue;
         }
      }

      //CASE(c)(d) : both fanins are the same or inverse
      if(_dfsList[i]->getfanin(0)->getId()==_dfsList[i]->getfanin(1)->getId()){
         if(_dfsList[i]->faninIsInv(0)==_dfsList[i]->faninIsInv(1)){
            opt_merge(_dfsList[i], _dfsList[i]->getfanin(0), _dfsList[i]->faninIsInv(0));
            continue;
         }
         else{
            opt_merge(_dfsList[i], _totalList[0], false);
            continue;
         }
      }
   }
   dfsTraversal();
   clearFltInandNotUse();
   findFltInandNotUse();
}


void
CirMgr::opt_merge(CirGate * mgate, CirGate * newgate, bool inv){
   if(mgate->getTypeStr() == "AIG") _Andgate--;
   _totalList[mgate->getId()]=0;
   for(int i=0, n=mgate->faninNum(); i<n ; ++i){
      mgate->getfanin(i)->deletefanout(mgate);
   }
   for(int i=0, n=mgate->fanoutNum(); i<n ; ++i){
      if((!inv && !mgate->fanoutIsInv(i)) || (inv && mgate->fanoutIsInv(i))){
         mgate->getfanout(i)->chagefanin(mgate, newgate, 0);
         newgate->addfanout(mgate->getfanout(i), 0);
      }
      else{
         mgate->getfanout(i)->chagefanin(mgate, newgate, 1);
         newgate->addfanout(mgate->getfanout(i), 1);
      }
   }
   cout<<"Simplifying: "<<newgate->getId()<<" merging ";
   if(inv){ cout<<"!"; }
   cout<<mgate->getId()<<"..."<<endl;
   delete mgate;
}


/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
