#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <list>
#include <chrono>
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>
#include <climits>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <cassert>
#define ll long long
using namespace std;

/*
   NOTE - When to use std::move():
   -----------------------------------------------------------------------------------------------------
   Type                       | Examples                   | Use move()? | Why?
   ---------------------------+----------------------------+-------------+------------------------------
   Primitives                 | int, long long, bool       | ❌ No       | Already instantaneous (8 bytes in CPU register). Nothing to steal.
   Containers / Heap Objects  | string, vector, map, list  | ✅ Yes      | Steals heap buffer pointer without allocating new memory.
   Move-Only Types            | unique_ptr, thread         | ⚡ Required  | Cannot be copied at all; move is the only way.
   
*/

// Utility

using Clock = chrono::steady_clock;

ll getCurrentTimeInMiliSec(){
    auto currentTime = Clock::now();
    auto durationSinceEpoch = currentTime.time_since_epoch();
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(durationSinceEpoch);

    ll miliSecs = milliseconds.count();

    return miliSecs;
}

class CacheEntry{
    private:
        string value;
        ll expiryTimeMs;
    public:
        CacheEntry(const string value="", const ll ttlMs=-1):
        value(move(value)){
            if(ttlMs==-1){
                expiryTimeMs = -1;
            }
            else{
                expiryTimeMs = getCurrentTimeInMiliSec() + ttlMs;
            }
        }

        void setValue(const string &value){
            this->value = value;
        };

        string getValue(){
            return value;
        }

        ll getExpiryTimeMs(){
            return expiryTimeMs;
        }

        void setExpiryTimeMs(ll &expiryTimeMs){
            this->expiryTimeMs = expiryTimeMs;
        }
        
        bool isExpired(){
            
            if (expiryTimeMs == -1) return false;
            ll now = getCurrentTimeInMiliSec();
            return (now >= expiryTimeMs);
        }
};

class EvictionPolicy{
    public:
    virtual void onInsert(const string & key) = 0;
    virtual void onRemove(const string & key) = 0;
    virtual void onAcess(const string & key) = 0;
    virtual string getVictim() = 0;
    
    virtual ~EvictionPolicy() = default;
    
    // 1. virtual ~EvictionPolicy()
    // This says:
    // "Make the destructor virtual so that derived classes are destroyed correctly."
    
    // 2. = default
    // This says:
    // "C++, you create the normal destructor for me. I don't need to write any code inside it."
};


class LRUPolicy : public EvictionPolicy{
    private:
        list<string> order;
        
        unordered_map<string, list<string>::iterator> position;
        
    public:
        
        void onInsert(const string &key) override{
            order.push_front(key);
            position[key] = order.begin();
        }
        
        void onRemove(const string &key) override{
            
            auto it = position.find(key);
            
            if(it==position.end()){
                return;
            }
            
            order.erase(it->second);
            
            position.erase(it);
        }
        
        void onAcess(const string &key) override{
            auto it = position.find(key);
            
            if(it==position.end()){
                return;
            }
            
            order.erase(it->second);
            
            order.push_front(key);
            
            position[key] = order.begin();
        }
        
        string getVictim() override {
            return order.back();
        }
};

class LFUPolicy : EvictionPolicy{};

// Cache Node - where cache is present 

class CacheNode{
    string nodeId;
    int capacity;
    unordered_map<string,CacheEntry> cache; // key, user data
    unique_ptr<EvictionPolicy> policy;
    mutex mtx;
    
    public:
    
        CacheNode(const string &nodeId, const int &capacity, unique_ptr<EvictionPolicy> policy):
        nodeId(move(nodeId)),
        policy(move(policy)),
        capacity(capacity){};
        
        optional<string> get(const string &key){
            lock_guard<mutex> lock(mtx);
            
            if(!cache.count(key)){
                return nullopt;
            }
            
            auto cacheData = cache[key];
            
            // TTL check
            if(cacheData.isExpired()){
                cache.erase(key);
                
                policy->onRemove(key);
                
                return nullopt;
            }
            
            policy->onAcess(key);
            
            return cacheData.getValue();
        }
        
        void put(const string &key, const string &value, ll ttlMs=-1){
            lock_guard<mutex> lock(mtx);
            
            if(cache.count(key)){
                auto &cacheData = cache[key];
                
                cacheData.setValue(value);
                
                policy->onAcess(key);
                
                return;
            }
            
            if((int)cache.size()>=capacity){
                string victim = policy->getVictim(); // key
                
                cache.erase(victim);
                
                policy->onRemove(victim);
            }
            
            cache[key] = CacheEntry(value,ttlMs);
            
            policy->onInsert(key);
        }
        
        void remove(const string &key){
            lock_guard<mutex> lock(mtx);
            
            if(!cache.count(key)){
                return;
            }
            
            cache.erase(key);
            
            policy->onRemove(key);
        }
        
        string getNodeId() const {
            return nodeId;
        }
};

class ConsistentHash{
    private:
        map<unsigned long, string> ring;
        hash<string> hasher;
    public:
        void addNode(const string &nodeId){
            unsigned long hashValue  = hasher(nodeId);
            
            if(ring.count(hashValue)){
                return;
            }
            
            ring[hashValue] = nodeId;
        }
        
        void removeNode(const string &nodeId){
            unsigned long hashValue  = hasher(nodeId);
            if(!ring.count(hashValue)){
                return;
            }
            
            ring.erase(hashValue);
        }
        
        string getNode(const string &key){

            if(ring.empty()){
                return "";
            }
            
            unsigned long hashValue = hasher(key); // user id se hash nikala
            
            auto it = ring.lower_bound(hashValue);
            
            if(it==ring.end()){
                it = ring.begin();
            }
            
            return it->second;
        }
};

class DistributedCache{
    private:
        unordered_map<string, shared_ptr<CacheNode>> nodes;
        ConsistentHash hashRing;
    
    public:
        void addNode(shared_ptr<CacheNode> node){
            string nodeId = node->getNodeId();
            
            nodes[nodeId] = node;
            
            hashRing.addNode(nodeId);
        }
        
        void removeNode(const string & nodeId){
            nodes.erase(nodeId);
            
            hashRing.removeNode(nodeId);
        }
        
        optional<string> get(const string &key){
            string nodeId = hashRing.getNode(key);
            
            if(nodeId.empty()){
                return nullopt;
            }
            
            return nodes[nodeId]->get(key);
        }
        
        void put(const string & key, const string & value, const ll & ttlMs=-1){
            string nodeId = hashRing.getNode(key);
            
            if(nodeId.empty()){
                return;
            }
            
            nodes[nodeId]->put(key,value,ttlMs);
            
        }
        
        void remove(const string &key){
            string nodeId = hashRing.getNode(key);
            
            if(nodeId.empty()){
                return;
            }
            
            nodes[nodeId]->remove(key);
        }
};

void runDemo(){
    DistributedCache cache;
    
    auto node1 = make_shared<CacheNode>("Node-A",2,make_unique<LRUPolicy>());
    auto node2 = make_shared<CacheNode>("Node-B",2,make_unique<LRUPolicy>());
    auto node3 = make_shared<CacheNode>("Node-C",2,make_unique<LRUPolicy>());
    
    cache.addNode(node1);
    cache.addNode(node2);
    cache.addNode(node3);
    
    cache.put("user:1","Shikshak");
    cache.put("user:2","Ashmita");
    
    auto value = cache.get("user:1");
    
    if (value.has_value()) {
        cout << "Value: " << value.value() << endl;
    } else {
        cout << "Cache Miss" << endl;
    }
    
    cache.remove("user:1");
    
    cout<<"Removed user:1\n";
    
    cout<<"Accessing user:1\n";
    
    value = cache.get("user:1");
    
    
    if(!value.has_value()){
        cout<<"Cache miss after deleted\n";
    }
}


int main(){
    runDemo();
    return 0;
}