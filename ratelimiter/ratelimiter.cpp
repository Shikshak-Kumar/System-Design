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

// =========================================================
// Utility
// =========================================================

using Clock = chrono::steady_clock;

long long currentTimeMillis() {
    return chrono::duration_cast<chrono::milliseconds>(
               Clock::now().time_since_epoch())
        .count();
}


// =========================================================
// Strategy Interface
// =========================================================

class RateLimiterStrategy {
public:
    virtual bool allowRequest(const string& userId) = 0;

    virtual string getName() const = 0;

    virtual ~RateLimiterStrategy() = default;
};


// =========================================================
// 1. Fixed Window Counter
// =========================================================

class FixedWindowRateLimiter : public RateLimiterStrategy {

private:

    struct Window {
        long long windowStartTime;
        int requestCount;

        Window(long long startTime = 0)
            : windowStartTime(startTime),
              requestCount(0) {}
    };

    int maxRequests;
    long long windowSizeMillis;

    unordered_map<string, Window> userWindows;

    mutex mtx;

public:

    FixedWindowRateLimiter(
        int maxRequests,
        long long windowSizeMillis
    )
        : maxRequests(maxRequests),
          windowSizeMillis(windowSizeMillis) {}

    bool allowRequest(const string& userId) override {

        lock_guard<mutex> lock(mtx);

        long long now = currentTimeMillis();

        // Create window if user doesn't exist
        if (userWindows.find(userId) == userWindows.end()) {
            userWindows.emplace(userId, Window(now));
        }

        Window& window = userWindows[userId];

        // Check whether current window expired
        if (now - window.windowStartTime >= windowSizeMillis) {

            window.windowStartTime = now;
            window.requestCount = 0;
        }

        // Allow request
        if (window.requestCount < maxRequests) {

            window.requestCount++;

            return true;
        }

        return false;
    }

    string getName() const override {
        return "Fixed Window Counter";
    }
};


// =========================================================
// 2. Sliding Window Log
// =========================================================

class SlidingWindowLogRateLimiter : public RateLimiterStrategy {

private:

    int maxRequests;
    long long windowSizeMillis;

    unordered_map<string, deque<long long>> userRequestLogs;

    mutex mtx;

public:

    SlidingWindowLogRateLimiter(
        int maxRequests,
        long long windowSizeMillis
    )
        : maxRequests(maxRequests),
          windowSizeMillis(windowSizeMillis) {}

    bool allowRequest(const string& userId) override {

        lock_guard<mutex> lock(mtx);

        long long now = currentTimeMillis();

        auto& requestLog = userRequestLogs[userId];

        // Remove timestamps outside the window
        while (!requestLog.empty() &&
               now - requestLog.front() >= windowSizeMillis) {

            requestLog.pop_front();
        }

        // Check limit
        if (requestLog.size() < static_cast<size_t>(maxRequests)) {

            requestLog.push_back(now);

            return true;
        }

        return false;
    }

    string getName() const override {
        return "Sliding Window Log";
    }
};


// =========================================================
// 3. Sliding Window Counter
// =========================================================

class SlidingWindowCounterRateLimiter : public RateLimiterStrategy {

private:

    struct SlidingWindow {

        long long currentWindowStart;

        int currentCount;

        int previousCount;

        SlidingWindow(long long startTime = 0)
            : currentWindowStart(startTime),
              currentCount(0),
              previousCount(0) {}
    };

    int maxRequests;

    long long windowSizeMillis;

    unordered_map<string, SlidingWindow> userWindows;

    mutex mtx;

public:

    SlidingWindowCounterRateLimiter(
        int maxRequests,
        long long windowSizeMillis
    )
        : maxRequests(maxRequests),
          windowSizeMillis(windowSizeMillis) {}

    bool allowRequest(const string& userId) override {

        lock_guard<mutex> lock(mtx);

        long long now = currentTimeMillis();

        // Align current window to fixed boundaries
        long long currentWindowStart =
            now - (now % windowSizeMillis);

        if (userWindows.find(userId) == userWindows.end()) {

            userWindows.emplace(
                userId,
                SlidingWindow(currentWindowStart)
            );
        }

        SlidingWindow& window = userWindows[userId];

        // Window changed
        if (window.currentWindowStart != currentWindowStart) {

            long long windowsPassed =
                (currentWindowStart -
                 window.currentWindowStart)
                / windowSizeMillis;

            if (windowsPassed == 1) {

                // Previous current becomes previous window
                window.previousCount =
                    window.currentCount;

            } else {

                // More than one window passed
                window.previousCount = 0;
            }

            window.currentCount = 0;

            window.currentWindowStart =
                currentWindowStart;
        }

        // How much time has passed in current window
        long long elapsedTime =
            now - window.currentWindowStart;

        // Weight of previous window
        double previousWindowWeight =
            static_cast<double>(
                windowSizeMillis - elapsedTime
            ) / windowSizeMillis;

        // Estimated requests
        double estimatedRequestCount =
            window.currentCount +
            (window.previousCount *
             previousWindowWeight);

        // Allow if below limit
        if (estimatedRequestCount < maxRequests) {

            window.currentCount++;

            return true;
        }

        return false;
    }

    string getName() const override {
        return "Sliding Window Counter";
    }
};


// =========================================================
// 4. Token Bucket
// =========================================================

class TokenBucketRateLimiter : public RateLimiterStrategy {

private:

    struct Bucket {

        double tokens;

        long long lastRefillTimestamp;

        Bucket(
            double tokens = 0.0,
            long long timestamp = 0
        )
            : tokens(tokens),
              lastRefillTimestamp(timestamp) {}
    };

    int bucketCapacity;

    double refillRatePerSecond;

    unordered_map<string, Bucket> userBuckets;

    mutex mtx;

public:

    TokenBucketRateLimiter(
        int bucketCapacity,
        double refillRatePerSecond
    )
        : bucketCapacity(bucketCapacity),
          refillRatePerSecond(refillRatePerSecond) {}

    bool allowRequest(const string& userId) override {

        lock_guard<mutex> lock(mtx);

        long long now = currentTimeMillis();

        // Create bucket if user doesn't exist
        if (userBuckets.find(userId) == userBuckets.end()) {

            userBuckets.emplace(
                userId,
                Bucket(bucketCapacity, now)
            );
        }

        Bucket& bucket = userBuckets[userId];

        // Calculate elapsed time
        double elapsedSeconds =
            (now - bucket.lastRefillTimestamp)
            / 1000.0;

        // Calculate new tokens
        double tokensToAdd =
            elapsedSeconds * refillRatePerSecond;

        // Refill bucket
        bucket.tokens =
            min(
                static_cast<double>(bucketCapacity),
                bucket.tokens + tokensToAdd
            );

        bucket.lastRefillTimestamp = now;

        // Consume one token
        if (bucket.tokens >= 1.0) {

            bucket.tokens -= 1.0;

            return true;
        }

        return false;
    }

    string getName() const override {
        return "Token Bucket";
    }
};


// =========================================================
// 5. Leaky Bucket
// =========================================================

class LeakyBucketRateLimiter : public RateLimiterStrategy {

private:

    struct LeakyBucket {

        double currentWaterLevel;

        long long lastLeakTimestamp;

        LeakyBucket(long long timestamp = 0)
            : currentWaterLevel(0),
              lastLeakTimestamp(timestamp) {}
    };

    int bucketCapacity;

    double leakRatePerSecond;

    unordered_map<string, LeakyBucket> userBuckets;

    mutex mtx;

public:

    LeakyBucketRateLimiter(
        int bucketCapacity,
        double leakRatePerSecond
    )
        : bucketCapacity(bucketCapacity),
          leakRatePerSecond(leakRatePerSecond) {}

    bool allowRequest(const string& userId) override {

        lock_guard<mutex> lock(mtx);

        long long now = currentTimeMillis();

        if (userBuckets.find(userId) == userBuckets.end()) {

            userBuckets.emplace(
                userId,
                LeakyBucket(now)
            );
        }

        LeakyBucket& bucket =
            userBuckets[userId];

        // Calculate elapsed time
        double elapsedSeconds =
            (now - bucket.lastLeakTimestamp)
            / 1000.0;

        // Calculate leaked amount
        double leakedAmount =
            elapsedSeconds * leakRatePerSecond;

        // Remove leaked requests
        bucket.currentWaterLevel =
            max(
                0.0,
                bucket.currentWaterLevel -
                leakedAmount
            );

        bucket.lastLeakTimestamp = now;

        // Add new request if bucket has capacity for 1 full request
        if (bucket.currentWaterLevel + 1.0 <=
            bucketCapacity) {

            bucket.currentWaterLevel += 1.0;

            return true;
        }

        return false;
    }

    string getName() const override {
        return "Leaky Bucket";
    }
};


// =========================================================
// Enum for Strategy Type
// =========================================================

enum class RateLimiterType {

    FIXED_WINDOW,

    SLIDING_WINDOW_LOG,

    SLIDING_WINDOW_COUNTER,

    TOKEN_BUCKET,

    LEAKY_BUCKET
};


// =========================================================
// Factory
// =========================================================

class RateLimiterFactory {

public:

    static unique_ptr<RateLimiterStrategy>
    getRateLimiter(RateLimiterType type) {

        switch (type) {

            case RateLimiterType::FIXED_WINDOW:

                return make_unique<
                    FixedWindowRateLimiter
                >(5, 10000);


            case RateLimiterType::SLIDING_WINDOW_LOG:

                return make_unique<
                    SlidingWindowLogRateLimiter
                >(5, 10000);


            case RateLimiterType::SLIDING_WINDOW_COUNTER:

                return make_unique<
                    SlidingWindowCounterRateLimiter
                >(5, 10000);


            case RateLimiterType::TOKEN_BUCKET:

                return make_unique<
                    TokenBucketRateLimiter
                >(5, 1);


            case RateLimiterType::LEAKY_BUCKET:

                return make_unique<
                    LeakyBucketRateLimiter
                >(5, 1);


            default:

                throw invalid_argument(
                    "Unsupported rate limiter type"
                );
        }
    }
};


// =========================================================
// API Gateway
// =========================================================

class ApiGateway {

private:

    unique_ptr<RateLimiterStrategy>
        rateLimiterStrategy;

public:

    explicit ApiGateway(
        unique_ptr<RateLimiterStrategy> strategy
    )
        : rateLimiterStrategy(std::move(strategy)) {
        if (!rateLimiterStrategy) {
            throw invalid_argument(
                "Rate limiter strategy cannot be null"
            );
        }
    }

    bool handleRequest(
        const string& userId,
        int requestNumber
    ) {

        bool allowed =
            rateLimiterStrategy->allowRequest(userId);

        if (allowed) {

            cout
                << "Request "
                << requestNumber
                << " allowed using "
                << rateLimiterStrategy->getName()
                << endl;

        } else {

            cout
                << "Request "
                << requestNumber
                << " rejected using "
                << rateLimiterStrategy->getName()
                << " | HTTP 429 Too Many Requests"
                << endl;
        }

        return allowed;
    }
};


// =========================================================
// UNIT TESTS & ASSERTION CHECKS
// =========================================================

// ---------------------------------------------------------
// 1. Fixed Window Tests
// ---------------------------------------------------------
void testFixedWindowRateLimiter() {
    // 3 requests allowed per 150ms window
    FixedWindowRateLimiter limiter(3, 150);
    assert(limiter.getName() == "Fixed Window Counter");

    // Burst 3 requests -> all allowed
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);

    // 4th request in the same window -> rejected
    assert(limiter.allowRequest("userA") == false);

    // Wait for the window to expire
    this_thread::sleep_for(chrono::milliseconds(160));

    // New window -> allowed again
    assert(limiter.allowRequest("userA") == true);

    cout << "testFixedWindowRateLimiter PASSED\n";
}

// ---------------------------------------------------------
// 2. Sliding Window Log Tests
// ---------------------------------------------------------
void testSlidingWindowLogRateLimiter() {
    // 2 requests allowed per 150ms sliding window
    SlidingWindowLogRateLimiter limiter(2, 150);
    assert(limiter.getName() == "Sliding Window Log");

    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == false);

    // Wait for log window to slide past initial requests
    this_thread::sleep_for(chrono::milliseconds(160));

    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == false);

    cout << "testSlidingWindowLogRateLimiter PASSED\n";
}

// ---------------------------------------------------------
// 3. Sliding Window Counter Tests
// ---------------------------------------------------------
void testSlidingWindowCounterRateLimiter() {
    // 3 requests per 500ms
    SlidingWindowCounterRateLimiter limiter(3, 500);
    assert(limiter.getName() == "Sliding Window Counter");

    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);

    // Exceeded limit within current window
    assert(limiter.allowRequest("userA") == false);

    cout << "testSlidingWindowCounterRateLimiter PASSED\n";
}

// ---------------------------------------------------------
// 4. Token Bucket Tests
// ---------------------------------------------------------
void testTokenBucketRateLimiter() {
    // Capacity 3 tokens, refill 10 tokens/sec (1 token per 100ms)
    TokenBucketRateLimiter limiter(3, 10.0);
    assert(limiter.getName() == "Token Bucket");

    // Consume all 3 initial tokens
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);

    // Empty bucket -> rejected
    assert(limiter.allowRequest("userA") == false);

    // Wait 120ms to refill at least 1 token
    this_thread::sleep_for(chrono::milliseconds(120));
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == false);

    // Cap check: wait 500ms (would produce 5 tokens, but max capacity is capped at 3)
    this_thread::sleep_for(chrono::milliseconds(500));
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == false);

    cout << "testTokenBucketRateLimiter PASSED\n";
}

// ---------------------------------------------------------
// 5. Leaky Bucket Tests
// ---------------------------------------------------------
void testLeakyBucketRateLimiter() {
    // Capacity 2, leaks 5 req/sec (1 request leaks every 200ms)
    LeakyBucketRateLimiter limiter(2, 5.0);
    assert(limiter.getName() == "Leaky Bucket");

    // Add 2 requests to fill bucket
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == true);

    // Overflow -> rejected
    assert(limiter.allowRequest("userA") == false);

    // Wait 220ms so 1 request leaks out
    this_thread::sleep_for(chrono::milliseconds(220));
    assert(limiter.allowRequest("userA") == true);
    assert(limiter.allowRequest("userA") == false);

    cout << "testLeakyBucketRateLimiter PASSED\n";
}

// ---------------------------------------------------------
// 6. Factory & Strategy Resolution Tests
// ---------------------------------------------------------
void testRateLimiterFactory() {
    auto fw = RateLimiterFactory::getRateLimiter(RateLimiterType::FIXED_WINDOW);
    assert(fw != nullptr && fw->getName() == "Fixed Window Counter");

    auto swl = RateLimiterFactory::getRateLimiter(RateLimiterType::SLIDING_WINDOW_LOG);
    assert(swl != nullptr && swl->getName() == "Sliding Window Log");

    auto swc = RateLimiterFactory::getRateLimiter(RateLimiterType::SLIDING_WINDOW_COUNTER);
    assert(swc != nullptr && swc->getName() == "Sliding Window Counter");

    auto tb = RateLimiterFactory::getRateLimiter(RateLimiterType::TOKEN_BUCKET);
    assert(tb != nullptr && tb->getName() == "Token Bucket");

    auto lb = RateLimiterFactory::getRateLimiter(RateLimiterType::LEAKY_BUCKET);
    assert(lb != nullptr && lb->getName() == "Leaky Bucket");

    // Invalid type exception
    bool caughtException = false;
    try {
        RateLimiterFactory::getRateLimiter(static_cast<RateLimiterType>(999));
    } catch (const invalid_argument&) {
        caughtException = true;
    }
    assert(caughtException);

    cout << "testRateLimiterFactory PASSED\n";
}

// ---------------------------------------------------------
// 7. Multi-User Isolation Tests
// ---------------------------------------------------------
void testMultiUserIsolation() {
    FixedWindowRateLimiter limiter(2, 5000);

    // user1 exhausts quota
    assert(limiter.allowRequest("user1") == true);
    assert(limiter.allowRequest("user1") == true);
    assert(limiter.allowRequest("user1") == false);

    // user2 is isolated and has full quota
    assert(limiter.allowRequest("user2") == true);
    assert(limiter.allowRequest("user2") == true);
    assert(limiter.allowRequest("user2") == false);

    // user3 also isolated
    assert(limiter.allowRequest("user3") == true);

    cout << "testMultiUserIsolation PASSED\n";
}

// ---------------------------------------------------------
// 8. API Gateway Integration & Validation Tests
// ---------------------------------------------------------
void testApiGateway() {
    // Null strategy validation
    bool caughtNullStrategy = false;
    try {
        ApiGateway gateway(nullptr);
    } catch (const invalid_argument&) {
        caughtNullStrategy = true;
    }
    assert(caughtNullStrategy);

    // Normal operation with Token Bucket
    auto strategy = RateLimiterFactory::getRateLimiter(RateLimiterType::TOKEN_BUCKET);
    ApiGateway gateway(std::move(strategy));

    // First 5 allowed (capacity is 5 in factory), 6th rejected
    for (int i = 1; i <= 5; i++) {
        assert(gateway.handleRequest("client-abc", i) == true);
    }
    assert(gateway.handleRequest("client-abc", 6) == false);

    cout << "testApiGateway PASSED\n";
}

// ---------------------------------------------------------
// 9. Thread Safety & Concurrency Tests
// ---------------------------------------------------------
void testConcurrency() {
    // 10 initial tokens, 0 refill rate during test
    auto limiter = make_shared<TokenBucketRateLimiter>(10, 0.0);
    atomic<int> allowedCount{0};
    atomic<int> rejectedCount{0};

    vector<thread> threads;
    int totalRequests = 30;

    for (int i = 0; i < totalRequests; i++) {
        threads.emplace_back([limiter, &allowedCount, &rejectedCount]() {
            if (limiter->allowRequest("concurrent-user")) {
                allowedCount++;
            } else {
                rejectedCount++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Exactly 10 requests allowed, remaining 20 rejected
    assert(allowedCount == 10);
    assert(rejectedCount == 20);
    assert(allowedCount + rejectedCount == totalRequests);

    cout << "testConcurrency PASSED\n";
}


// =========================================================
// RUN ALL TESTS
// =========================================================

void runAllTests() {

    cout << "\n========== RUNNING TESTS ==========\n\n";

    testFixedWindowRateLimiter();
    testSlidingWindowLogRateLimiter();
    testSlidingWindowCounterRateLimiter();
    testTokenBucketRateLimiter();
    testLeakyBucketRateLimiter();
    testRateLimiterFactory();
    testMultiUserIsolation();
    testApiGateway();
    testConcurrency();

    cout << "\n========== ALL TESTS PASSED ==========\n";
}


// =========================================================
// DEMO
// =========================================================

void runDemo() {

    cout << "\n========== RATE LIMITER LLD DEMO ==========\n\n";

    // Imagine this comes from configuration
    RateLimiterType selectedAlgorithm =
        RateLimiterType::TOKEN_BUCKET;

    // Factory creates required strategy
    auto strategy =
        RateLimiterFactory::getRateLimiter(
            selectedAlgorithm
        );

    // API Gateway uses the strategy
    ApiGateway apiGateway(
        std::move(strategy)
    );

    string userId = "user-123";

    cout
        << "Selected Algorithm: "
        << RateLimiterFactory::getRateLimiter(
               selectedAlgorithm
           )->getName()
        << endl;

    cout
        << "----------------------------------"
        << endl;

    // Send 10 requests
    for (int i = 1; i <= 10; i++) {

        apiGateway.handleRequest(
            userId,
            i
        );

        // 300 ms between requests
        this_thread::sleep_for(
            chrono::milliseconds(300)
        );
    }

    cout << "\n========== DEMO COMPLETE ==========\n";
}


// =========================================================
// MAIN
// =========================================================

int main() {

    runAllTests();

    runDemo();

    return 0;
}