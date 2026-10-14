#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <stdexcept>

using namespace std;

// Record: Represents a message

class Record{
    public:
        string key;
        string value;
        long long offset = -1;
        Record(const string &key, const string &value, const long long & offset):
        key(key),
        value(value),
        offset(offset){
        }

        string getKey(){
            return key;
        }

        string getValue(){
            return value;
        }

        long long getOffset(){
            return offset;
        }
};

// Replica: Stores records in a log, and it is the copy of partition data of the leader

class Replica{
    private:
        vector<shared_ptr<Record>> records;
        int brokerId; // replica id
    public:
    Replica(int brokerId) : brokerId(brokerId) {}

    int getBrokerId(){
        return brokerId;
    }

    long long addRecord(shared_ptr<Record> record){

        shared_ptr<Record> newRecord = record;

        newRecord->offset = records.size();

        records.push_back(newRecord);

        cout<<"[Replica] Record added with details:\n";
        cout << "    Offset: " << newRecord->offset << "\n";
        cout << "    Key   : " << newRecord->key << "\n";
        cout << "    Value : " << newRecord->value << "\n";

        return newRecord->offset;
    }
    
    vector<shared_ptr<Record>> readRecords(const long long &offset, const long long & maximumRecords){
        
        vector<shared_ptr<Record>> result;
        
        if(offset<0 || offset>=(long long)records.size()){
            cout<<"[Replica] Invalid details, maximum records available: "<<(long long)records.size()<<endl;
        }
        
        long long end = min(offset + maximumRecords, (long long)records.size());
        
        for(long long i = offset; i<end;i++){
            result.push_back(records[i]);
        }

        return result;
    }

    long long endOfOffset(){
        return records.size();
    }
};

class Broker{
    private:
        int brokerId;
        bool alive;
    public:
        Broker(int brokerId, bool alive = true):
        brokerId(brokerId),
        alive(alive){
            cout<<"[Broker] Broker with id: "<<brokerId<<" created"<<endl;
        }

        int getBrokerId(){
            return brokerId;
        }

        int getId(){
            return brokerId;
        }

        bool isAlive(){
            return alive;
        }

        void kill(){
            alive = false;
            cout<<"[Broker] Broker "<<brokerId<<" is now DEAD\n";
        }
};



class TopicPartition{
    public:
    string topicName;
    int partitionNumber;
    
    explicit TopicPartition(const string &topicName, const int &partitionNumber):
    topicName(topicName),
    partitionNumber(partitionNumber){}

    bool operator==(const TopicPartition &other){
        return ((topicName == other.topicName) && (partitionNumber == other.partitionNumber));
    }

    string getTopicName() const {
        return topicName;
    }
    
    int getPartitionNumber() const{
        return partitionNumber;
    }
};

class Partition{
    private:
        TopicPartition topicPartition;
        int leaderBrokerId; // leader
        vector<shared_ptr<Replica>> replicas;
        unordered_set<int> inSyncReplicaBrokerIds;
    
    public:
        Partition(const TopicPartition & topicPartition, vector<shared_ptr<Replica>> & replicas):
        topicPartition(topicPartition),
        leaderBrokerId(-1),
        replicas(replicas){
            cout << "\n[Partition] Created: " << topicPartition.topicName << "-" << topicPartition.partitionNumber << "\n";

            cout << "[Partition] Creating replicas:\n";

            for(auto & replica : replicas){
                inSyncReplicaBrokerIds.insert(replica->getBrokerId());
            }
            
            if(!replicas.empty()){
                leaderBrokerId = replicas[0]->getBrokerId();
            }
            
            cout << "\n[Partition] Initial ISR:\n";
            
            for (int brokerId : inSyncReplicaBrokerIds) {
                cout << "    Broker "<< brokerId<< "\n";
            }
            
            cout << "[Partition] Initial Leader: Broker "<< leaderBrokerId<< "\n";
        }
        
        TopicPartition getTopicPartition(){
            return topicPartition;
        }
        
        int getLeaderBrokerId(){
            return leaderBrokerId;
        }
        
        void removeFromSync(int brokerId){
            
            cout << "\n[Partition] Removing Broker " << brokerId << " from ISR\n";
            
            inSyncReplicaBrokerIds.erase(brokerId);
            
            cout << "\n[Partition] Current ISR:\n";
            
            for (int brokerId : inSyncReplicaBrokerIds) {
                cout << "    Broker "<< brokerId<< "\n";
            }
            
            if (inSyncReplicaBrokerIds.empty()) {
                cout << "[Partition] WARNING: ISR is empty\n";
            }
            
        }
        
        int electLeader(){
            
            cout << "\n[Partition] Starting leader election...\n";
            
            if(inSyncReplicaBrokerIds.empty()){
                leaderBrokerId = -1;
                cout << "[Partition] No broker available "<< "for leader election\n";
                return leaderBrokerId;
            }
            
            leaderBrokerId = *inSyncReplicaBrokerIds.begin();
            
            cout << "[Partition] Broker " << leaderBrokerId << " elected as new Leader\n";
            
            return leaderBrokerId;
        }
        
        long long addRecord(shared_ptr<Record> record){
            long long offset = -1;
            
            for(auto & replica : replicas){
                
                if(inSyncReplicaBrokerIds.count(replica->getBrokerId())){
                    offset = replica->addRecord(record);
                }
            }
            
            return offset;
        }
        
        vector<shared_ptr<Record>> readRecords(const long long &offset, const long long & maximumRecords){
        
            if(leaderBrokerId == -1){
                electLeader();
            }
            
            if(leaderBrokerId == -1){
                cout<<"[Partition] leader not available";
                return {};
            }
            
            for(auto &replica : replicas){
                if(replica->getBrokerId() == leaderBrokerId){
                    return replica->readRecords(offset,maximumRecords);
                }
            }
            
            return {};
        }
        
};

// Topic
class Topic{
    private:
        string topicName;
        vector<shared_ptr<Partition>> partitions;

    public:
        Topic(const string &topicName):
        topicName(topicName){
            cout << "[Topic] Topic " << topicName << " created\n";
        }

        string getTopicName(){
            return topicName;
        }
        
        int getPartitionCount(){
            return (int)partitions.size();
        }

        void addPartition(shared_ptr<Partition> &partition){
            partitions.push_back(partition);
        }

        shared_ptr<Partition> getPartition(const int & partitionNumber){
            if(partitionNumber>=(int)partitions.size() || partitionNumber<0){
                cout<<"[Topic] Invalid partition number";
                return nullptr;
            }
            
            return partitions[partitionNumber];
        }
};

class Consumer{
    private:
        string consumerId;
    public:
        explicit Consumer(const string & consumerId):
        consumerId(consumerId){}
        
        string getConsumerId(){
            return consumerId;
        }
};



class ConsumerGroup{
    private:
        string consumerGroupId;
        vector<shared_ptr<Consumer>> members;
        unordered_map<string,long long> partitionAssigned; // consumer id ,partition number
    public:
        ConsumerGroup(const string & consumerGroupId):
        consumerGroupId(consumerGroupId){
            
        }
        
        void join(shared_ptr<Consumer> consumer){
            cout<<"[Consumer Group] member "<<consumer->getConsumerId()<<" added";
            members.push_back(consumer);
        }  

        string getConsumerGroupId() const{
            return consumerGroupId;
        }
};

class OffsetStore{
    private:
        unordered_map<string,int> offsets;
        
        string makeKey(const ConsumerGroup & consumerGroup, const TopicPartition & topicPartition){
            return consumerGroup.getConsumerGroupId() + ":" + topicPartition.getTopicName() + ":" + to_string(topicPartition.getPartitionNumber());
        }
        
    public:
        
        void commitOffset(const ConsumerGroup & consumerGroup, const TopicPartition & topicPartition, const long long & offset){
            
            auto key = makeKey(consumerGroup,topicPartition);
            
            offsets[key] = offset;
            
            cout << "[OffsetStore] Offset successfully stored\n";
        }
        
        long long getCommittedOffset(const ConsumerGroup & consumerGroup, const TopicPartition & topicPartition){
            auto key = makeKey(consumerGroup,topicPartition);
            
            if(offsets.count(key)){
                return offsets[key];
            }
            
            return 0; // This consumer group has never committed an offset, so start reading from offset 0
        }
};

class ClusterListener{
    public:
    
        explicit ClusterListener(){
            cout<<"[Cluster Listener] Listener created"<<endl;
        }
    
        void onLeaderChange( const TopicPartition& topicPartition, int oldLeaderBrokerId, int newLeaderBrokerId) {
    
            cout << "\n[Cluster Listener] " << "Leader change detected\n";
    
            cout << "    Topic     : " << topicPartition.topicName << "\n";
    
            cout << "    Partition : " << topicPartition.partitionNumber << "\n";
    
            cout << "    Old Leader: Broker " << oldLeaderBrokerId << "\n";
    
            cout << "    New Leader: Broker " << newLeaderBrokerId << "\n";
    
            cout << "\n[Cluster Listener] " << "Cluster state updated\n";
        }
};

class Controller{
    public:
        unordered_map<int,shared_ptr<Broker>> brokers; // broker id, broker
        vector<ClusterListener*> listeners;
    
        void registerBroker(shared_ptr<Broker> broker){
            brokers[broker->getBrokerId()] = broker;
            
            cout << "[Controller] Registered Broker "<< broker->getId() << "\n";
        }
        
        void addClusterListner(ClusterListener * listener){
            listeners.push_back(listener);
            
            cout<<"[Controller] listener added"<<endl;
        }
        
        void onBrokerFailure(shared_ptr<Broker> failedBroker, vector<shared_ptr<Partition>>&partitions){
            cout << "\n[Controller] Broker failure detected\n";
            cout << "    Failed Broker: "<< failedBroker->getBrokerId()<< "\n";
            
            for(auto & partition  : partitions){
                if(partition->getLeaderBrokerId()==failedBroker->getBrokerId()){
                    int oldLeaderBrokerId = failedBroker->getBrokerId();
                    
                    partition->removeFromSync(oldLeaderBrokerId);
                    
                    int newLeaderBrokerId = partition->electLeader();
                    
                    
                    for(auto *listener : listeners){
                        listener->onLeaderChange(partition->getTopicPartition(), oldLeaderBrokerId, newLeaderBrokerId);
                    }
                }
            }
        }
    
};



class KafkaCluster{
    private:
        unordered_map<int,shared_ptr<Broker>> brokers;
        unordered_map<string,shared_ptr<Topic>> topics;
        Controller controller;
        OffsetStore offsetStore;
    public:
    
        Controller& getController(){
            return controller;
        }
        
        OffsetStore & getOffsetStore(){
            return offsetStore;
        }
        
        void addBroker(shared_ptr<Broker> broker){
            
            brokers[broker->getBrokerId()] = broker;
            
            controller.registerBroker(broker);
        }
        
        void addTopic(shared_ptr<Topic> topic){
            topics[topic->getTopicName()] = topic;
            cout << "\n[KafkaCluster] Topic '" << topic->getTopicName() << "' added successfully\n";
        }
        
        long long produce(TopicPartition &topicPartition, shared_ptr<Record> record){
            auto topicName = topicPartition.getTopicName();
            
            if(!topics.count(topicName)){
                cout<<"[Kafka Cluster] topic not found\n";
                return -1;
            }
            
            auto topic = topics[topicName];
            
            auto partition = topic->getPartition(topicPartition.getPartitionNumber());
            
            if(partition==nullptr){
                cout<<"[Kafka Cluster] partition not found in the "<<topicName<<" topic\n";
                return -1;
            }
            
            long long offset=partition->addRecord(record);
            return offset;
        }
        
        vector<shared_ptr<Record>> fetch(TopicPartition &topicPartition, long long offset, int maximumRecords){
            string topicName = topicPartition.getTopicName();
            if(!topics.count(topicName)){
                cout<<"[Kafka Cluster] topic not found\n";
                return {};
            }
            
            auto topic = topics[topicName];
            auto partition = topic->getPartition(topicPartition.getPartitionNumber());
            
            if(partition==nullptr){
                cout<<"[Kafka Cluster] partition not found in the "<<topicName<<" topic\n";
                return {};
            }
            
            return partition->readRecords(offset,maximumRecords);
            
        }
        
        void killBroker(int brokerId){
            cout << "\n[Simulation] Killing Broker " << brokerId << "...\n";
            
            if(!brokers.count(brokerId)){
                cout<<"[Kafka Cluster] broker not found";
            }
            
            auto failedBroker = brokers[brokerId];
            
            failedBroker->kill();
            
            vector<shared_ptr<Partition>> allPartitions;
            
            for(auto & [topicName, topic] : topics){
                int numberOfPartitions = topic->getPartitionCount();
                
                for(int i=0;i<numberOfPartitions;i++){
                    auto partition = topic->getPartition(i);
                    
                    if(partition){
                        allPartitions.push_back(partition);
                    }
                }
            }
            
            controller.onBrokerFailure(failedBroker,allPartitions);
        }
};

void runDemo(){
    
    
    
    KafkaCluster cluster;
    
    cout << "\n[1] Creating Brokers\n";
    cout << "--------------------------------\n";
    
    auto b1 = make_shared<Broker>(1);
    auto b2 = make_shared<Broker>(2);
    auto b3 = make_shared<Broker>(3);
    
    cluster.addBroker(b1);
    cluster.addBroker(b2);
    cluster.addBroker(b3);
    
    cout << "\n[2] Creating Topic and Partition\n";
    cout << "--------------------------------\n";
    
    auto topic = make_shared<Topic>("orders");
    
    TopicPartition tp("orders", 0);
    
    auto replica_1 = make_shared<Replica>(1);
    auto replica_2 = make_shared<Replica>(2);
    auto replica_3 = make_shared<Replica>(3);
    
    vector<shared_ptr<Replica>> replicas = {replica_1, replica_2, replica_3};
    
    auto partition = make_shared<Partition>(tp, replicas);
    
    
    topic->addPartition(partition);
    
    cluster.addTopic(topic);
    
    cout << "Leader: " << partition->getLeaderBrokerId() << endl;
    
    cout << "\n[3] Registering listener\n";
    cout << "--------------------------------\n";
    
    ClusterListener listener;
    
    cluster.getController().addClusterListner(&listener);
    
    cout << "\n[4] Producing Records\n";
    cout << "--------------------------------\n";
    
    auto rec1 = make_shared<Record>("order-1","Phone",-1);
    auto rec2 = make_shared<Record>("order-2","Laptop",-1);
    
    
    long long offset1 = cluster.produce(tp,rec1);
    cout << "[Producer] order-1 written at offset "<< offset1 << "\n";
    long long offset2 = cluster.produce(tp,rec2);
    cout << "[Producer] order-2 written at offset "<< offset2 << "\n";
    
    
    cout << "\n[5] Consumer Fetch\n";
    cout << "--------------------------------\n";
    
    vector<shared_ptr<Record>> records = cluster.fetch(tp,0,10);
    
    for (auto& record : records) {
        cout << "[Consumer] "<< "Offset="<<record->getOffset()<<" Key=" << record->getKey()  << " Value=" << record->getValue() << "\n";
    }
    
    cout << "\n[6] Consumer Group\n";
    cout << "--------------------------------\n";
    
    ConsumerGroup consumerGroup("order-group");
    
    auto consumer1 = make_shared<Consumer>("consumer-1");
    
    consumerGroup.join(consumer1);
    
    cluster.getOffsetStore().commitOffset(consumerGroup,tp,2);
    
    long long committedOffset = cluster.getOffsetStore().getCommittedOffset(consumerGroup,tp);
    
    cout << "[Offset Store] Committed offset: " << committedOffset << endl;
    
    cout << "\n[7] Broker Failure\n";
    cout << "--------------------------------\n";

    cout << "[Before Failure]\n";
    cout << "Leader = Broker " << partition->getLeaderBrokerId() << "\n";
    
    cluster.killBroker(b1->getBrokerId());
    
    cout << "\n[After Failure]\n";
    cout << "New Leader = Broker "<< partition->getLeaderBrokerId() << "\n";
    
    cout << "\n[8] Produce After Failover\n";
    cout << "--------------------------------\n";
    
    auto rec3 = make_shared<Record>("order-3","PC",-1);
    
    long long offset3 = cluster.produce(tp,rec3);
    cout << "[Producer] order-3 written at offset "<< offset3 << "\n";

    cout << "\n[Consumer] Verifying all records after failover:\n";
    
    for (auto& record : cluster.fetch(tp, 0, 10)) {
        cout << "[Consumer] Offset=" << record->getOffset() << " Key=" << record->getKey() << " Value=" << record->getValue() << "\n";
    }
}

int main(){
    runDemo();
    return 0;

}