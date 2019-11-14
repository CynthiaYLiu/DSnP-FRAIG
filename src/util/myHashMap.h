/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>
#include <bitset>
#include <stddef.h>

using namespace std;

// TODO: (Optionally) Implement your own HashMap and Cache classes.
 
//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.

class faninKey
{
public:
   faninKey(size_t f1, size_t f2) : _fanin1(f1), _fanin2(f2){}
   size_t operator() () const { return(_fanin1+_fanin2); } 
   bool operator == (const faninKey& k) const { 
      return ((_fanin1==k._fanin1) && (_fanin2==k._fanin2) || (_fanin1==k._fanin2) && (_fanin2==k._fanin1)); 
   }

private:
   size_t _fanin1;
   size_t _fanin2;
};


class simValueKey
{
public:
   simValueKey(size_t v) : _simvalue(v){}
   size_t operator() () const { 
      if(_simvalue > (SIZE_MAX/2)) return ~_simvalue;
      else return _simvalue;
   }
   bool operator == (const simValueKey& k) const { 
      return ((_simvalue == k._simvalue) || (_simvalue == ~k._simvalue));
   }
private:
   size_t _simvalue;
};


template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap(size_t b=0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashMap() { reset(); }

   // [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the HashMap
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashMap<HashKey, HashData>;

   public:
      iterator(vector<HashNode>* begin , vector<HashNode>* end): _bucket(begin), _end(end), _num(0) {}
      iterator(){}
      ~iterator() {}

      const HashNode& operator * () const { return (*_bucket)[_num]; }
      iterator& operator ++ () {
         if(_bucket == _end){ return (*this); }
         if((*_bucket).size()-1==_num || (*_bucket).size()==0){
            do{
               if(_bucket == _end){ return (*this); }
               _bucket++;
               _num = 0;
            }while((*_bucket).size()==0);
         }
         else{ _num++; }
         return (*this);
      }
      iterator operator ++ (int) { iterator result = *(this); ++*(this); return result; }
      iterator& operator -- () {
         if((_num==0 || (*_bucket).size()==0)){
            do{
               _bucket--;
               _num = 0;
            }while((*_bucket).size()==0);
         }
         else{ _num--; }
         return (*this);
      }
      iterator operator -- (int) { iterator result = *(this); --*(this); return result; }
      bool operator != (const iterator& i) const { return (_bucket!=i._bucket)||(_num!=i._num); }
      bool operator == (const iterator& i) const { return (_bucket==i._bucket)&&(_num==i._num); }
      iterator& operator = (const iterator& i) { 
         _bucket = i._bucket; 
         _num = i._num; 
         _end = i._end;
         return *(this); 
      }

   private:
      vector<HashNode>*     _bucket;
      vector<HashNode>*     _end;
      size_t                _num;
   };

   void init(size_t b) {
      reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      iterator itr( _buckets, _buckets+_numBuckets );
      if(_buckets[0].size()==0) ++itr;
      return itr; 
   }
   // Pass the end
   iterator end() const {
      iterator itr( _buckets+_numBuckets, _buckets+_numBuckets ); 
      return itr;
   }
   // return true if no valid data
   bool empty() const {
      for(size_t i=0 ; i< _numBuckets ; ++i){
         if(_buckets[i].size()!=0) return false;
      }
      return true;
   }
   // number of valid data
   size_t size() const {
      size_t s = 0;
      for(size_t i=0 ; i< _numBuckets ; ++i){
         s += _buckets[i].size();
      }
      return s;
   }

   // check if k is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const HashKey& k) const {
      for(size_t i=0 ; i< _buckets[bucketNum(k)].size() ; ++i){
         if(k == _buckets[bucketNum(k)][i].first){ return true; }
      }
      return false;
   }

   // query if k is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(const HashKey& k, HashData& d) const {
      for(size_t i=0 ; i< _buckets[bucketNum(k)].size() ; ++i){
         if(k == _buckets[bucketNum(k)][i].first){
            d = _buckets[bucketNum(k)][i].second;
            return true;
         }
      }
      return false;
   }

   // update the entry in hash that is equal to k (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const HashKey& k, HashData& d) {
      for(size_t i=0 ; i< _buckets[bucketNum(k)].size() ; ++i){
         if(k == _buckets[bucketNum(k)][i].first){
            _buckets[bucketNum(k)][i].second = d;
            return true;
         }
      }
      HashNode n(k,d);
      _buckets[bucketNum(k)].push_back(n);
      return false;   
   }

   // return true if inserted d successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) {
      if(check(k)){ return false; }
      else{ 
         HashNode n(k,d);
         _buckets[bucketNum(k)].push_back(n);
         return true;
      }
   }

   // return true if removed successfully (i.e. k is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const HashKey& k) {
      for(size_t i=0 ; i< _buckets[bucketNum(k)].size() ; ++i){
         if(k == _buckets[bucketNum(k)][i]){
            _buckets[bucketNum(k)].erase(_buckets[bucketNum(k)].begin()+i);
            return true;
         }
      }
      return false;
   }

   // query if k is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else, insert d in the hash and return false;
   bool queryAndInsert(const HashKey& k, HashData*& d){
      size_t bn = bucketNum(k);
      for(size_t i=0 ; i< _buckets[bn].size() ; ++i){
         if(k == _buckets[bn][i].first){
            d = & (_buckets[bn][i].second);
            return true;
         }
      }
      HashNode n(k,*d);
      _buckets[bn].push_back(n);
      return false;
   }



private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H
