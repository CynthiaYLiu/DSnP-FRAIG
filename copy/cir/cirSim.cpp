/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <bitset>
#include <math.h>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

#define SIZE_T  (8*sizeof(size_t))

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
	bitset<SIZE_T> ParalPattern[_Pi];
	size_t patternSim = 0;
	size_t stop = 0;
	while(stop<20){
		for(size_t i=0 ; i<_Pi ; ++i){
			size_t patternSim = rnGen(INT_MAX);
			for(int i=0 ; i<3 ; ++i){
				patternSim = patternSim*pow(2,16);
				patternSim += rnGen(INT_MAX);
			}
			ParalPattern[i] = patternSim;
		}
		if(_simFirst){ initFECGrps(); }
		simulate(ParalPattern);
  		size_t originGrps = _FECGrps.size();
		splitFECGrps();
		int formula = _FECGrps.size()-originGrps;
		if(formula>=originGrps/1000*(-1) && (formula <= (originGrps/1000))) stop++;
		else if(formula == 0)stop++;
		else stop = 0;
		// Write logFile
		if(_simLog!=0){ writeLogFile(SIZE_T, ParalPattern); }
		patternSim += SIZE_T;
	}
	cout <<char(13) << setw(30) << ' ' << char(13);
	cout << patternSim << " patterns simulated."<<endl;
	// Sort the Grps and all Grp and record the Grp number in gate
	sortAndRecord();
}

void
CirMgr::fileSim(ifstream& patternFile)
{
	istringstream iss;
	size_t patternSim = 0;
	size_t num = 0;
	string temp_line = "";
	bitset<SIZE_T> ParalPattern[_Pi];
	// read a pattern in file
	while(getline(patternFile , temp_line , '\n')){
		if(temp_line.empty()) continue;
		iss = istringstream(temp_line);
   		while(getline(iss,temp_line,' ')){
			if(temp_line.empty()) continue;
			if(temp_line.length()!=_Pi){ 
				cerr << "Error: Pattern(" << temp_line << ") length(" << temp_line.length()
					 <<") does not match the number of inputs("<<_Pi<<") in a circuit!!"<<endl;
				cout << patternSim << " patterns simulated."<<endl;
				return;
			}
			for(int i=0 ; i<_Pi ; ++i){
				if(temp_line[i]!='0' && temp_line[i]!='1'){
					cerr << "Error: Pattern(" << temp_line << ") contains a non-0/1 character(\'"
					     << temp_line[i] << "\')." << endl;
					cout << patternSim << " patterns simulated."<<endl;
					return;
				}
				ParalPattern[i][num] = temp_line[i]-'0';
			}
			num++ ;
			// If the parallel pattern is full, do simulatation
			if(num >= SIZE_T){
				if(_simFirst){ initFECGrps(); }
				simulate(ParalPattern);
				splitFECGrps();
				// Write logFile
				if(_simLog!=0){ writeLogFile(num, ParalPattern); }
				patternSim += SIZE_T;
				num = 0;
			}
		}
	}
	// If there are patterns left, do simulatation
	if(num!=0){
		//Pad the parallel pattern with 0
		for(int i=num ; i<SIZE_T ; ++i){
			for(int j=0 ; j<_Pi ; ++j){
				ParalPattern[j][i]=0;
			}
		}
		if(_simFirst){ initFECGrps(); }
		simulate(ParalPattern);
		splitFECGrps();
		// Write logFile
		if(_simLog!=0){ writeLogFile(num, ParalPattern); }
		patternSim += num;
	}
	cout <<char(13) << setw(30) << ' ' << char(13);
	cout << patternSim << " patterns simulated."<<endl;
	// Sort the Grps and all Grp and record the Grp number in gate
	sortAndRecord();
}


void
CirMgr::simulate(bitset<SIZE_T> ParalPattern[])
{
	CirGate::setGlobalRef();
	// Set PI
	for(size_t i=0 ; i<_Pi ; ++i){
		_piList[i]->setValue( ParalPattern[i].to_ullong() );
	}
	// Simulate from every PO
	for(size_t i=0 ; i<_Po ; ++i){
		_totalList[_Max+1+i]->simulate(_simFirst);
	}
	_simFirst = false;
}

void 
CirMgr::initFECGrps()
{
	FECGrp Grp;
	Grp.push_back(0);
	for(size_t i=0 ; i<_dfsList.size() ; ++i){
		if(_dfsList[i]->getTypeStr()!="AIG") continue;
		Grp.push_back(_dfsList[i]->getId());
	}
	_FECGrps.push_back(Grp);
}

void 
CirMgr::splitFECGrps()
{
	size_t _FECGrpsSize=_FECGrps.size();
	for(size_t grpnum=0 ; grpnum<_FECGrpsSize ; ++grpnum){
		HashMap<simValueKey, FECGrp> newFecGrps(_FECGrps[grpnum].size());
		for(size_t i=0 ; i<_FECGrps[grpnum].size() ; ++i){
			simValueKey key(_totalList[_FECGrps[grpnum][i]]->getValue());
			FECGrp grp;
			// If its group already existed
			if(newFecGrps.query(key, grp)){
				grp.push_back(_FECGrps[grpnum][i]);
				newFecGrps.update(key, grp);
			}
			else{
				grp.push_back(_FECGrps[grpnum][i]);
				newFecGrps.insert(key, grp);
			}
		}
		// Collect valid FecGrp in the hash
		HashMap<simValueKey, FECGrp>::iterator itr = newFecGrps.begin();
		while(itr != newFecGrps.end()){
			if((*itr).second.size()>1){
				_FECGrps.push_back((*itr).second);
			}
			if((*itr).second.size()==1){ _totalList[((*itr).second)[0]]->removeGrp();}
			++itr;
		}
	}
	if(_FECGrpsSize!=0){
		_FECGrps.erase(_FECGrps.begin(), _FECGrps.begin()+_FECGrpsSize);
	}
	cout <<char(13) << setw(30) << ' ' << char(13);
	cout << "Total #FEC Group = " << _FECGrps.size() << flush;
}


void
CirMgr::sortAndRecord()
{
	// Sort all Grp
	for(size_t i=0 ; i<_FECGrps.size() ; ++i){
		sort(_FECGrps[i].begin(), _FECGrps[i].end());
	}
	// Sort the Grps
	sort(_FECGrps.begin(), _FECGrps.end(), sortFECGrp);
	// Record the Grp number in gate
	for(size_t i=0 ; i<_FECGrps.size() ; ++i){
		for(size_t j=0 ; j<_FECGrps[i].size() ; ++j)
			_totalList[_FECGrps[i][j]]->setGrp(i);
	}
}


void
CirMgr::writeLogFile(size_t num, bitset<SIZE_T> ParalPattern[])
{
	bitset<SIZE_T> Po[_Po];
	for(size_t i=0 ; i<_Po ; ++i){
		Po[i] = _totalList[_Max+1+i]->getValue();
	}
	for(size_t i=0 ; i<num ; ++i){
		for(size_t j=0 ; j<_Pi ; ++j){
			(*_simLog)<<ParalPattern[j][i];
		}
		(*_simLog)<<" ";
		for(size_t j=0 ; j<_Po ; ++j){
			(*_simLog)<<Po[j][i];
		}
		(*_simLog)<<endl;
	} 
}


/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
