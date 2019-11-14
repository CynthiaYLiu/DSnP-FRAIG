/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <sstream>
#include <fstream>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   CirGate::resetGlobalRef();
   ifstream ifs;
   istringstream iss;
   string temp_line = "";
   string temp_data = "";
   int temp_int[5];
   CirGate* temp_gate;

   //open
   ifs.open(fileName.c_str());
   if (!ifs) {
      cerr << "Cannot open design \"" << fileName << "\"!!" << endl;
      return false;
   }

   //SECTION1: read first line, set M I L O A
   getline(ifs,temp_line,'\n'); 
   iss = istringstream(temp_line);
   getline(iss,temp_data,' ');
   for(int i=0 ; i<5 ; ++i){
      if(!getline(iss,temp_data,' ')) { return false; }
      if(!myStr2Int(temp_data, temp_int[i])){ return false; }
   }
   _Max = temp_int[0];
   _Pi = temp_int[1];
   _Latch = temp_int[2];
   _Po = temp_int[3];
   _Andgate = temp_int[4];

   //set totalList length
   _totalList.resize(_Max+_Po+1,0);;

   //const 0 gate
   temp_gate = new ConstGate(0, 0);
   _totalList[0] = temp_gate;

   //SECTION2: creat PI
   int line=1;
   for(int i =0 ; i<_Pi ; ++i){
      getline(ifs,temp_line,'\n');
      line++;
      myStr2Int(temp_line, temp_int[0]);
      temp_gate = new PiGate(temp_int[0]/2, line);
      _totalList[temp_int[0]/2] = temp_gate;
      _piList.push_back(temp_gate);
   }

   //SECTION3: creat PO
   for(int i =0 ; i<_Po ; ++i){
      getline(ifs,temp_line,'\n');
      line++;
      temp_gate = new PoGate(_Max+i+1, line);
      _totalList[_Max+i+1] = temp_gate;
   }

   //SECTION4: creat AIG
   while( getline(ifs,temp_line,'\n') ){
      line++;
      if(temp_line=="c") break; // Next SECTION
      iss = istringstream(temp_line);
      int i=0;
      getline(iss,temp_data,' ');
      if(!myStr2Int(temp_data, temp_int[i])) break; // Next SECTION
      i++;
      while(getline(iss,temp_data,' ')){ myStr2Int(temp_data, temp_int[i]); i++; }
      temp_gate = new AigGate(temp_int[0]/2, line);
      _totalList[temp_int[0]/2] = temp_gate;
   }

   //SECTION3+: connect po
   ifs.close();
   ifs.open(fileName.c_str());//Open file again
   for(int i=0 ; i<_Pi+1 ; ++i){ getline(ifs,temp_line,'\n'); }
   for(int i=0 ; i<_Po ; ++i){ 
      getline(ifs,temp_line,'\n'); 
      myStr2Int(temp_line, temp_int[0]);
      if(_totalList[temp_int[0]/2]==0){ // if the fanin gate not exist, creat a UNDEF gate
         temp_gate = new UndefGate(temp_int[0]/2, 0);
         _totalList[temp_int[0]/2] = temp_gate;
      }
      temp_gate = _totalList[_Max+i+1];
      temp_gate->addfanin(_totalList[temp_int[0]/2], temp_int[0]%2);
      _totalList[temp_int[0]/2]->addfanout(temp_gate, temp_int[0]%2); 
   }

   //SECTION4+: connect AIG
   while( getline(ifs,temp_line,'\n') ){
      if(temp_line=="c"){ break; }  // End readCircuit
      iss = istringstream(temp_line);
      int i=0;
      getline(iss,temp_data,' ');
      if(!myStr2Int(temp_data, temp_int[i])) break; // Next SECTION
      i++;
      while(getline(iss,temp_data,' ')){ myStr2Int(temp_data, temp_int[i]); i++; }
      for(int i=1 ; i<=2 ; i++){  // for two fanin
         if(_totalList[temp_int[i]/2]==0){  // If the fanin gate not exist
            temp_gate = new UndefGate(temp_int[i]/2, 0);
            _totalList[temp_int[i]/2] = temp_gate;
         }
         temp_gate = _totalList[temp_int[0]/2];
         temp_gate->addfanin(_totalList[temp_int[i]/2], temp_int[i]%2);
         _totalList[temp_int[i]/2]->addfanout(temp_gate, temp_int[i]%2);
      }
   }

   //SECTION5: Set PI PO name
   do{
      if(temp_line=="c"){ break; }// End readCircuit
      iss = istringstream(temp_line);
      getline(iss,temp_data,' '); //get pi po number
      if(temp_data[0]=='i'){
         temp_data.erase(temp_data.begin());
         myStr2Int(temp_data, temp_int[0]);
         temp_line.erase(0,temp_data.size()+2);
         _piList[temp_int[0]]->addName(temp_line);
      }
      else if(temp_data[0]=='o'){
         temp_data.erase(temp_data.begin());
         myStr2Int(temp_data, temp_int[0]);
         temp_line.erase(0,temp_data.size()+2);
         _totalList[_Max+1+temp_int[0]]->addName(temp_line);
      }
   }while(getline(ifs,temp_line,'\n'));

   //Sort every gate
   for(int i=0 ; i<_Max+1 ; ++i){
      if(_totalList[i]!=0) _totalList[i]->sortFanout();
   }

   dfsTraversal();
   findFltInandNotUse();
   return true;
}

void 
CirMgr::findFltInandNotUse()
{
   //PI : NotUse
   for(int i=1 ; i<=_Pi ; ++i){
      if(_totalList[i]!=0){
         if(_totalList[i]->fanoutNum()==0)
            _notusedList.push_back(_totalList[i]);
      }
   }
   //AIG : Floating Input & Not Use
   for(int i=_Pi+1 ; i<=_Max ; ++i){
      if(_totalList[i]!=0){
         if(_totalList[i]->fanoutNum()==0){ _notusedList.push_back(_totalList[i]); }
         for(int j=0 ; j<_totalList[i]->faninNum() ; ++j){
            if(_totalList[i]->getfanin(j)->getTypeStr()=="UNDEF"){
               _flfaninList.push_back(_totalList[i]);
               break;
            }
         }
      }
   }
   //PO : Floating Input
   for(int i=_Max+1 ; i<_totalList.size() ; ++i){
      if(_totalList[i]!=0){
         for(int j=0 ; j<_totalList[i]->faninNum() ; ++j){
            if(_totalList[i]->getfanin(j)->getTypeStr()=="UNDEF"){
               _flfaninList.push_back(_totalList[i]);
               break;
            }
         }
      }
   }
}


/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout<<endl<<"Circuit Statistics"<<endl<<"=================="<<endl;
   cout<<"  PI "<<setw(11)<<right<<_Pi<<endl;
   cout<<"  PO "<<setw(11)<<right<<_Po<<endl;
   cout<<"  AIG"<<setw(11)<<right<<_Andgate<<endl;
   cout<<"------------------"<<endl;
   cout<<"  Total"<<setw(9)<<right<<_Pi+_Po+_Andgate<<endl;
}

void
CirMgr::printNetlist() const
{
   cout<<endl;
   for(int i=0 ; i<_dfsList.size() ; ++i){
      cout<<"["<<i<<"] ";
      _dfsList[i]->printGate();
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for(int i=0 ; i<_Pi ; ++i){
      cout << " "<<_piList[i]->getId();
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for(int i=_Max+1 ; i<=_Max+_Po ; ++i){
      cout << " "<<_totalList[i]->getId();
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   if(!_flfaninList.empty()){
      cout<<"Gates with floating fanin(s):";
      for(int i=0 ; i<_flfaninList.size() ; ++i){
         cout<<" "<<_flfaninList[i]->getId();
      }
      cout<<endl;
   }
   if(!_notusedList.empty()){
      cout<<"Gates defined but not used  :";
      for(int i=0 ; i<_notusedList.size() ; ++i){
         cout<<" "<<_notusedList[i]->getId();
      }
      cout<<endl;
   }
}

void
CirMgr::printFECPairs() const
{
   for(size_t i=0 ; i<_FECGrps.size() ; ++i){
      size_t firstValue = _totalList[_FECGrps[i][0]]->getValue();
      cout<<"["<<i<<"]";
      for(size_t j=0 ; j<_FECGrps[i].size() ; ++j){
         cout<<" ";
         if(_totalList[_FECGrps[i][j]]->getValue() != firstValue) cout<<"!";
         cout<<_FECGrps[i][j];
      }
      cout<<endl;
   }
}

void
CirMgr::writeAag(ostream& outfile) const
{
   int _NewAIG=0;
   vector<int> NewAIGList;
   for(int i=0 ; i<_dfsList.size() ; ++i){
      if(_dfsList[i]->getTypeStr()=="AIG"){
         _NewAIG++;
         NewAIGList.push_back(_dfsList[i]->getId());
      }
   }
   outfile<<"aag "<<_Max<<" "<<_Pi<<" "<<_Latch<<" "<<_Po<<" "<<_NewAIG<<endl;
   for(int i=0 ; i<_Pi ; ++i){ outfile<<(_piList[i]->getId())*2<<endl; }
   for(int i=_Max+1 ; i<=_Max+_Po ; ++i){ 
      if(_totalList[i]->faninIsInv(0)){ outfile<<(_totalList[i]->getfanin(0)->getId())*2+1<<endl; }
      else{ outfile<<(_totalList[i]->getfanin(0)->getId())*2<<endl; }
   }
   CirGate* now_gate;
   for(int i=0 ; i<NewAIGList.size() ; ++i){
      now_gate = _totalList[NewAIGList[i]];
      outfile<<(now_gate->getId())*2;
      for(int i=0 ; i<2 ; ++i){
         if(now_gate->faninIsInv(i)){ outfile<<" "<<(now_gate->getfanin(i)->getId())*2+1; }
         else{ outfile<<" "<<(now_gate->getfanin(i)->getId())*2; }
      }
      outfile<<endl;
   }
   for(int i=0 ; i<_Pi ; ++i){ 
      if(_piList[i]->haveName()) 
         outfile<<"i"<<i<<" "<<_piList[i]->getName()<<endl; 
   }
   for(int i=_Max+1 ; i<=_Max+_Po ; ++i){ 
      if(_totalList[i]->haveName()) 
         outfile<<"o"<<i<<" "<<_totalList[i]->getName()<<endl; 
   }
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   vector<CirGate *> _List;
   CirGate::setGlobalRef();
   g->dfsTraversal(_List);

   int _NewAIG=0;
   vector<int> NewAIGList;
   for(int i=0 ; i<_List.size() ; ++i){
      if(_List[i]->getTypeStr()=="AIG"){
         _NewAIG++;
         NewAIGList.push_back(_List[i]->getId());
      }
   }
   outfile<<"aag "<<_Max<<" "<<_Pi<<" "<<_Latch<<" "<<1<<" "<<_NewAIG<<endl;
   for(int i=0 ; i<_Pi ; ++i){ outfile<<(_piList[i]->getId())*2<<endl; }
   outfile<<g->getId()*2<<endl;
   CirGate* now_gate;
   for(int i=0 ; i<NewAIGList.size() ; ++i){
      now_gate = _totalList[NewAIGList[i]];
      outfile<<(now_gate->getId())*2;
      for(int i=0 ; i<2 ; ++i){
         if(now_gate->faninIsInv(i)){ outfile<<" "<<(now_gate->getfanin(i)->getId())*2+1; }
         else{ outfile<<" "<<(now_gate->getfanin(i)->getId())*2; }
      }
      outfile<<endl;
   }
   for(int i=0 ; i<_Pi ; ++i){ 
      if(_piList[i]->haveName()) 
         outfile<<"i"<<i<<" "<<_piList[i]->getName()<<endl; 
   }
   outfile<<"o"<<0<<" "<<g->getId()<<endl; 
}

vector<unsigned>
CirMgr::sendGrp(size_t i)
{
   if(i>=_FECGrps.size()) { vector<unsigned> a; return a; }
   return _FECGrps[i];
}



/**********************************************************/
/*   class CirMgr member functions for circuit DFS        */
/**********************************************************/

void 
CirMgr::dfsTraversal()
{
   _dfsList.clear();
   CirGate::setGlobalRef();
   for(int i=_Max+1 ; i<=_Max+_Po ; ++i){
      _totalList[i]->dfsTraversal(_dfsList);
   }
}

