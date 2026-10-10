#include <iostream>
#include <string>
#include <unordered_map>
#include <deque>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cassert>
#include <atomic>
#include <vector>

using namespace std;

// NOTES -> N1, N2, etc.....

// N1:
// mp[key] = value --> Two steps: Creates a default empty object first, then overwrites it via assignment.
// mp.emplace(key, value) One step: Constructs the object in-place directly inside the map's memory.

// N2:
// using is a keyword in C++ that acts as a tool to create an alias (a nickname) for an existing data type
// Ex:
// Instead of typing unsigned long long everywhere, you can use 'BigInt'
// using BigInt = unsigned long long; 
// BigInt population = 8000000000; 

// N3:
// Clock::now().time_since_epoch() -> "How much time has passed from this clock's starting reference point until now?"
// time_since_epoch() -> returns the duration since the epoch of the clock
// epoch -> a reference point in time from which time is measured
// chrono::duration_cast<chrono::milliseconds>(...) -> converts the time duration into milliseconds


using Clock = chrono::steady_clock;

long long getCurrentTimeInMilliSec(){
    return chrono::duration_cast<chrono::milliseconds>(
        Clock::now().time_since_epoch()
    ).count();
}

enum class RateLimiterType{
    FIXED_WINDOW,
    SLIDING_WINDOW_LOG,
    SLIDING_WINDOW_COUNTER,
    TOKEN_BUCKET,
    LEAKY_BUCKET
};

// Rate Limiter Strategy

class RateLimiterStrategy{
    public:
        virtual bool allowRequest(const string &userId) = 0;
        virtual string getAlgoName() = 0;
        virtual ~RateLimiterStrategy() = default;
};

// Algo-1: Fixed Window Rate Limiter

// DEFINITION & MECHANICS:
// - Time is partitioned into discrete, globally aligned chunks of length `windowSizeMillis`.
// - Example: For a 10-second window, boundaries align at :00, :10, :20, :30.
// - Formula for window start: currentWindowStart = now - (now % windowSizeMillis)
// - A counter tracks requests made in the current window.
// - When the aligned window rolls over, the counter resets to 0.
//
// ADVANTAGES:
// - O(1) time complexity, O(1) space complexity per user.
// - Memory efficient: stores only one integer and one timestamp per user.
//
// DRAWBACKS (THE BOUNDARY BURST PROBLEM):
// - Traffic can double at window edges. If maxRequests = 5 per 10s:
//   Sending 5 requests at t = 9.9s and 5 requests at t = 10.1s yields 10 requests
//   within an actual 0.2s duration (2x burst across boundary).

// -> For every user, allow at most maxRequests requests during a fixed time window.
// For example:
// maxRequests = 5
// windowSizeMillis = 60000
// means:
// Each user can make 5 requests every 60 seconds.
// The class keeps a separate counter for every user.

class FixedWindowRateLimiter :  public RateLimiterStrategy {
    private:

        // Window
        class Window{
            public:
                long long windowStartTime;
                int requestsMadeInCurrentWindow; // Stores how many requests this user has already made in the current window.


                explicit Window(long long startTime=0): 
                windowStartTime(startTime),
                requestsMadeInCurrentWindow(0){
                }

        };

        int maxRequests;
        long long windowSizeInMillis;
        unordered_map<string,Window> userWindows;
        mutex mtx; // Synchronization lock for thread safety

        // Mutex prevents race conditions when multiple threads access shared data.
        // Example:
        // requestCount = 4, maxRequests = 5
        //
        // Thread 1 → 🔒 gets lock → sees 4 → increments to 5 → ALLOW → 🔓
        // Thread 2 → waits for lock → 🔒 gets lock → sees 5 → REJECT → 🔓
        //
        // lock_guard automatically locks the mutex and unlocks it when it goes out of scope.
        // lock_guard<mutex> lock(mtx);

        public:

            FixedWindowRateLimiter(int maxRequests, long long windowSizeInMillis):
            maxRequests(maxRequests),
            windowSizeInMillis(windowSizeInMillis){}

            string getAlgoName() override{
                return "Fixed Window Counter";
            }

            bool allowRequest(const string &userId) override {
                lock_guard<mutex> lock(mtx);

                long long now = getCurrentTimeInMilliSec();
                long long currentWindowStart = now - (now % windowSizeInMillis);

                auto it = userWindows.find(userId);

                if(it==userWindows.end()){
                    userWindows.emplace(userId,Window(currentWindowStart));
                }

                Window &window = userWindows[userId];

                // If time has crossed into a new aligned window, reset the counter
                if(window.windowStartTime != currentWindowStart){
                    window.windowStartTime = currentWindowStart;
                    window.requestsMadeInCurrentWindow = 0;
                }

                if(window.requestsMadeInCurrentWindow < maxRequests){
                    window.requestsMadeInCurrentWindow++;
                    return true;
                }
                return false;
            }
        
};

// Algo-2: Sliding Window Log Rate Limiter

// DEFINITION & MECHANICS:
// - Maintains a double-ended queue (deque) of exact timestamps for each request per user.
// - When a new request arrives at timestamp `now`:
//   1. Evict all timestamps older than `now - windowSizeMillis` from the front of the deque.
//   2. If deque size < maxRequests: append `now` to back and accept request.
//   3. Otherwise: reject request.
//
// ADVANTAGES:
// - 100% mathematically accurate rolling rate limit.
// - Zero boundary burst anomalies (unlike Fixed Window).
//
// DRAWBACKS:
// - High memory overhead: stores O(k) timestamps per user, where k = maxRequests.
// - Susceptible to memory pressure under heavy load or large quota limits.

class SlidingWindowLogRateLimiter : public RateLimiterStrategy{
    private:
        int maxRequests;
        long long windowSizeInMillis;
        unordered_map<string,deque<long long>> userRequestLogs;
        mutex mtx;

    public:

        SlidingWindowLogRateLimiter(int maxRequests, long long windowSizeInMillis):
        maxRequests(maxRequests),
        windowSizeInMillis(windowSizeInMillis){}

        string getAlgoName() override{
            return "Sliding Window Log";
        }


        bool allowRequest(const string& userId) override {
            lock_guard<mutex> lock(mtx);

            long long now = getCurrentTimeInMilliSec();

            auto it = userRequestLogs.find(userId);

            if(it == userRequestLogs.end()){
                userRequestLogs.emplace(userId,deque<long long>());
            }

            auto & currentUserRequestLogs = userRequestLogs[userId];

            while(!currentUserRequestLogs.empty() && (now - windowSizeInMillis) > currentUserRequestLogs.front()){
                currentUserRequestLogs.pop_front();
            }

            if(currentUserRequestLogs.size() < maxRequests){
                currentUserRequestLogs.push_back(now);
                return true;
            }

            return false;
        }

};


// Algo-3: SLIDING WINDOW COUNTER (APPROXIMATE)

// DEFINITION & MECHANICS:
// - Hybrid combining Fixed Window's low memory with Sliding Log's smooth boundary handling.
// - Tracks two counters per user:
//   1. `previousCount`: Requests in the preceding fixed window.
//   2. `currentCount`: Requests in the current fixed window.
// - Approximates rolling requests via a weighted linear interpolation formula:
//   elapsedTime = now - currentWindowStart
//   previousWindowWeight = (windowSizeMillis - elapsedTime) / windowSizeMillis
//   estimatedCount = currentCount + (previousCount * previousWindowWeight)
//
// WHY IT PREVENTS BURSTS:
// - If 10 requests were sent at the end of window 1, then at the beginning of window 2
//   (e.g., 20% in), previousWindowWeight is 0.8.
//   estimatedCount starts at 0 + 10 * 0.8 = 8.
//   Only 2 requests can be made, preventing boundary spikes.
//
// COMPLEXITY:
// - O(1) Time, O(1) Space per user (only 2 integers and 1 timestamp).

class SlidingWindowCounterRateLimiter : public RateLimiterStrategy {
    private:

        class SlidingWindow{
        public:
            long long currentWindowStart;
            int requestMadeInCurrentWindow;
            int requestMadeInPrevWindow;

            SlidingWindow(long long startTime=0): 
            currentWindowStart(startTime),
            requestMadeInCurrentWindow(0),
            requestMadeInPrevWindow(0){}

        };
    
        int maxRequests;
        long long windowSizeInMillis;
        unordered_map<string, SlidingWindow> userWindows;
        mutex mtx;

    public:
        SlidingWindowCounterRateLimiter(int maxRequests, long long windowSizeInMillis):
        maxRequests(maxRequests),
        windowSizeInMillis(windowSizeInMillis){}

    string getAlgoName() override{
        return "Sliding window counter";
    }
    
    bool allowRequest(const string & userId) override{
        lock_guard<mutex> lock(mtx);

        long long now = getCurrentTimeInMilliSec();

        long long currentWindowStart = now - (now % windowSizeInMillis);

        auto it = userWindows.find(userId);

        if(it==userWindows.end()){
            userWindows.emplace(userId, SlidingWindow(currentWindowStart));
        }

        SlidingWindow & window = userWindows[userId];

        if(window.currentWindowStart != currentWindowStart){

            long long windowPassed = (currentWindowStart - window.currentWindowStart)/ windowSizeInMillis;

            if(windowPassed == 1){
                window.requestMadeInPrevWindow = window.requestMadeInCurrentWindow;
            
            }
            else{
                window.requestMadeInPrevWindow = 0;
            }

            window.currentWindowStart = currentWindowStart;
            window.requestMadeInCurrentWindow = 0;
        }

        long long elapsedTime = now - currentWindowStart;
        
        double previousWindowWeight = (double) (windowSizeInMillis - elapsedTime) / windowSizeInMillis;

        double estimatedRequestCount = previousWindowWeight*window.requestMadeInPrevWindow + window.requestMadeInCurrentWindow;

        if(estimatedRequestCount < maxRequests){
            window.requestMadeInCurrentWindow++;
            return true;
        }

        return false;
    }


};
    
// Algo-4: Token Bucket Rate Limiter

// DEFINITION & MECHANICS:
// - A bucket has a maximum capacity `bucketCapacity` of tokens.
// - Tokens are continuously replenished at a constant rate `refillRatePerSecond`.
// - Tokens are capped at `bucketCapacity` (excess tokens spill over and are lost).
// - Each incoming request requires 1 token to be processed:
//   - If tokens >= 1: decrement by 1, allow request.
//   - If tokens < 1: reject request.
//
// LAZY REFILL OPTIMIZATION (NO BACKGROUND THREAD):
// - Instead of running a background timer to add tokens continuously, refill is
//   computed lazily on demand when a request arrives:
//   elapsedSeconds = (now - lastRefillTimestamp) / 1000.0
//   tokensToAdd = elapsedSeconds * refillRatePerSecond
//   tokens = min(bucketCapacity, tokens + tokensToAdd)
//
// USE CASES:
// - Ideal for systems that allow short, controlled bursts of traffic
//   (up to bucketCapacity) while enforcing a strict long-term average rate.

class TokenBucketRateLimiter: public RateLimiterStrategy{
    private:

    class Bucket{
        public:
            double tokens; // current token balance
            long long lastRefillTimestamp;

            Bucket(const double &tokens = 0.0, const long long & lastRefillTimestamp = 0):
            tokens(tokens),
            lastRefillTimestamp(lastRefillTimestamp){};
    };

    double maxCapacity;
    double refillRatePerSecond;
    unordered_map<string,Bucket> userBuckets;
    mutex mtx;

    public:
        TokenBucketRateLimiter(const int & maxCapacity, const double & refillRatePerSecond):
        maxCapacity(maxCapacity),
        refillRatePerSecond(refillRatePerSecond){};

        string getAlgoName() override{
            return "Token Bucket";
        }

        bool allowRequest(const string & userId)  override {
            lock_guard<mutex> lock(mtx);

            long long now = getCurrentTimeInMilliSec();

            auto it = userBuckets.find(userId);

            if(it==userBuckets.end()){
                userBuckets.emplace(userId,Bucket(maxCapacity, now));
            }

            Bucket &bucket = userBuckets[userId];

            double elapsedTime = (now - bucket.lastRefillTimestamp)/1000.0;

            double tokensToAdd = elapsedTime * refillRatePerSecond;

            bucket.tokens = min(maxCapacity, bucket.tokens + tokensToAdd );

            if(bucket.tokens >= 1.0){
                bucket.tokens-=1.0;
                return true;
            }

            return false;

        }
};

// Algo-5 - LEAKY BUCKET (WATER-LEVEL MODEL)

// DEFINITION & MECHANICS:
// - Models a bucket with a hole at the bottom that leaks water at a fixed rate.
// - Water level represents queued / backlogged requests waiting for processing.
// - In this LLD water-level variant:
//   - Bucket has capacity `bucketCapacity`.
//   - Water drains at `leakRatePerSecond`.
//   - An incoming request attempts to add 1 unit of water.
//   - If currentWaterLevel + 1.0 <= bucketCapacity: accept request.
//   - If bucket overflows: reject request (HTTP 429).
//
// DIFFERENCE VS TOKEN BUCKET:
// - Token Bucket smooths the long-term rate while allowing bursts up to capacity.
// - Leaky Bucket enforces a smooth, constant outflow rate and buffers incoming bursts
//   as backlog, preventing downstream server saturation.

class LeakyBucketRateLimiter : public RateLimiterStrategy {
    private:

        class LeakyBucket{
            public:
                double currentWaterLevel;
                long long lastLeakTimestamp;
                
                LeakyBucket(const double & currentWaterLevel = 0.0, const long & lastLeakTimestamp = 0):
                lastLeakTimestamp(lastLeakTimestamp),
                currentWaterLevel(currentWaterLevel){};
        };

    
        
        int maxCapacity;
        double leakRatePerSecond;
        unordered_map<string,LeakyBucket> userBuckets;
        mutex mtx;

    public:
        LeakyBucketRateLimiter(const int &maxCapacity, const double &leakRatePerSecond):
        maxCapacity(maxCapacity),
        leakRatePerSecond(leakRatePerSecond){};
        
        string getAlgoName() override{
            return "Leaky Bucket";
        }

        bool allowRequest(const string & userId) override{
            lock_guard<mutex> lock(mtx);

            long long now = getCurrentTimeInMilliSec();

            auto it = userBuckets.find(userId);

            if(it==userBuckets.end()){
                userBuckets.emplace(userId,LeakyBucket(0,now));
            }

            auto & bucket = userBuckets[userId];

            double elapsedTime = (now - bucket.lastLeakTimestamp ) *1000.0;

            double leakedAmount = elapsedTime * leakRatePerSecond;
            
            bucket.currentWaterLevel = max(0.0, bucket.currentWaterLevel - leakedAmount);
            bucket.lastLeakTimestamp = now;

            if(bucket.currentWaterLevel + 1.0 <= maxCapacity ){
                bucket.currentWaterLevel += 1.0;
                return true;
            }

            return false;
        }
};

class RateLimiterConfig{
    public:
    // for FWC, SWL, SWC
    int maxRequests = 5;
    long long windowSizeInMillis = 10000;

    // for Token Bucket
    int bucketCapacity = 5;
    double refillRatePerSecond = 1.0;

    // for Leaky Bucket
    // int bucketCapacity = 5;
    double leakRatePerSecond = 1.0;
};

// N4: What is static here?
// Normally, to call a member function, you need an object:
// RateLimiterFactory factory;
// factory.getRateLimiter(...);
// But with:
// static unique_ptr<RateLimiterStrategy> getRateLimiter(...)
// you can call it directly using the class:
// RateLimiterFactory::getRateLimiter(...);
// No factory object is needed.

// N5:  unique_ptr
// unique_ptr provides exclusive ownership and automatic memory management. 
// The factory returns unique_ptr<RateLimiterStrategy> so the dynamically created strategy is safely managed without manual delete.
// unique_ptr → only one owner
// unique_ptr<User> u1 = make_unique<User>();
// unique_ptr<User> u2 = u1; 

// shared_ptr → multiple owners
// shared_ptr<User> s1 = make_shared<User>();
// shared_ptr<User> s2 = s1;

class RateLimiterFactory{
    public:
    static unique_ptr<RateLimiterStrategy> getRateLimiter(RateLimiterType type, const RateLimiterConfig & config = RateLimiterConfig{}){
        switch (type) {
            case RateLimiterType::FIXED_WINDOW:
                return make_unique<FixedWindowRateLimiter>(config.maxRequests, config.windowSizeInMillis);
            case RateLimiterType::SLIDING_WINDOW_LOG:
                return make_unique<SlidingWindowLogRateLimiter>(config.maxRequests,config.windowSizeInMillis);
            case RateLimiterType::SLIDING_WINDOW_COUNTER:
                return make_unique<SlidingWindowCounterRateLimiter>(config.maxRequests,config.windowSizeInMillis);
            case RateLimiterType::TOKEN_BUCKET:
                return make_unique<TokenBucketRateLimiter>(config.bucketCapacity,config.refillRatePerSecond);
            case RateLimiterType::LEAKY_BUCKET:
                return make_unique<LeakyBucketRateLimiter>(config.bucketCapacity,config.leakRatePerSecond);
            default:
                throw invalid_argument("Unknown rate limiter type");
        }
    }
};

// API GATEWAY (FACADE PATTERN)

// DESIGN PATTERN: FACADE PATTERN
// - ApiGateway provides a unified entry point for routing and traffic policing.
// - It holds a `unique_ptr<RateLimiterStrategy>` and delegates the decision to it.
// - Decoupled from HTTP protocols and console I/O inside core evaluation logic.
// - Returns a boolean allowing upstream layers to formulate HTTP 200 vs HTTP 429.

class ApiGateway{
    private:
        unique_ptr<RateLimiterStrategy> rateLimiterStrategy;
    public:
        explicit ApiGateway(unique_ptr<RateLimiterStrategy> strategy):
        rateLimiterStrategy(std::move(strategy)){
            if (!rateLimiterStrategy) {
                throw invalid_argument("Rate limiter strategy cannot be null");
            }
        }

        bool handleRequest(const string & userId){
            return rateLimiterStrategy->allowRequest(userId);
        }

        string getRateLimiterStrategyName(){
            return rateLimiterStrategy->getAlgoName();
        }
};

void runDemo(){
    cout<<"Running Demo \n"<<endl;

    vector<pair<RateLimiterType,RateLimiterConfig>> tests;

    RateLimiterConfig fixedConfig;
    fixedConfig.maxRequests = 5;
    fixedConfig.windowSizeInMillis = 10000;
    tests.push_back({RateLimiterType::FIXED_WINDOW, fixedConfig});

    RateLimiterConfig slidingLogConfig;
    slidingLogConfig.maxRequests = 5;
    slidingLogConfig.windowSizeInMillis = 10000;
    tests.push_back({RateLimiterType::SLIDING_WINDOW_LOG, slidingLogConfig});

    RateLimiterConfig slidingCounterConfig;
    slidingCounterConfig.maxRequests = 5;
    slidingCounterConfig.windowSizeInMillis = 10000;
    tests.push_back({RateLimiterType::SLIDING_WINDOW_COUNTER, slidingCounterConfig});

    RateLimiterConfig tokenConfig;
    tokenConfig.bucketCapacity = 5;
    tokenConfig.refillRatePerSecond = 1.0;
    tests.push_back({RateLimiterType::TOKEN_BUCKET, tokenConfig});

    RateLimiterConfig leakyConfig;
    leakyConfig.bucketCapacity = 5;
    leakyConfig.leakRatePerSecond = 1.0;
    tests.push_back({RateLimiterType::LEAKY_BUCKET, leakyConfig});

    for(auto & test : tests){

        RateLimiterType algorithm = test.first;
        RateLimiterConfig config = test.second;

        

        auto strategy = RateLimiterFactory::getRateLimiter(algorithm, config);

        string strategyName = strategy->getAlgoName();

        cout<<"Running test for algorithm: "<<strategyName<<endl;

        ApiGateway apiGateway(std::move(strategy));

        string userId = "user-1234";

        cout << "Algorithm: " << strategyName << endl;
        cout<<endl;

        long long startTime = getCurrentTimeInMilliSec();

        for(int i=0;i<=10;i++){

            bool allowed  = apiGateway.handleRequest(userId);

            long long now = getCurrentTimeInMilliSec();

            cout<<"Request "<<i<<" | Time: "<<(now-startTime)<<"ms -> "<<(allowed ? "ALLOWED" : "REJECTED")<<endl;

            this_thread::sleep_for(chrono::milliseconds(300));
        }
        cout<<endl;
    }

    
}

int main(){
    runDemo();
    return 0;
}