#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <sstream>

using namespace std;

struct TrafficData {
    int timestamp;
    int light_id;
    int cars_passed;
};

// Shared queue for producer-consumer pattern
queue<TrafficData> trafficQueue;
mutex queueMutex;
condition_variable cv;
bool finished = false;

// Producer thread function
void producer(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        int timestamp, light_id, cars_passed;
        if (ss >> timestamp >> light_id >> cars_passed) {
            unique_lock<mutex> lock(queueMutex);
            trafficQueue.push({timestamp, light_id, cars_passed});
            cv.notify_one();
        }
    }
    finished = true;
    cv.notify_all();
}

// Consumer thread function
void consumer(int topN) {
    map<int, int> totalTraffic;
    map<int, vector<pair<int, int>>> timestampTraffic;
    int maxCars = 0;
    
    while (true) {
        unique_lock<mutex> lock(queueMutex);
        cv.wait(lock, [] { return !trafficQueue.empty() || finished; });
        
        while (!trafficQueue.empty()) {
            TrafficData data = trafficQueue.front();
            trafficQueue.pop();
            if (data.light_id >= 0 && data.light_id <= 4) { // Ensure valid light IDs
                totalTraffic[data.light_id] += data.cars_passed;
                timestampTraffic[data.timestamp].push_back({data.light_id, data.cars_passed});
                maxCars = max(maxCars, totalTraffic[data.light_id]);
            }
        }
        
        if (finished && trafficQueue.empty()) break;
    }
    
    vector<pair<int, int>> sortedTraffic(totalTraffic.begin(), totalTraffic.end());
    sort(sortedTraffic.begin(), sortedTraffic.end(), [](auto &a, auto &b) {
        return a.second > b.second;
    });
    
    cout << "Top " << topN << " most congested traffic lights across all timestamps:\n";
    for (int i = 0; i < min(topN, (int)sortedTraffic.size()); ++i) {
        cout << "Light ID: " << sortedTraffic[i].first << ", Total Cars Passed: " << sortedTraffic[i].second << endl;
    }
    cout << "\nMaximum Cars Passed by a Single Traffic Light: " << maxCars << endl;
    
    cout << "\nDetailed Timestamp-wise Traffic Data:\n";
    for (const auto &entry : timestampTraffic) {
        cout << "Timestamp: " << entry.first << " min" << endl;
        for (const auto &light : entry.second) {
            cout << "  Light ID: " << light.first << " | Cars Passed: " << light.second << endl;
        }
    }
}

int main() {
    string filename = "traffic_data.txt";
    int topN = 5;
    
    thread producerThread(producer, filename);
    thread consumerThread(consumer, topN);
    
    producerThread.join();
    consumerThread.join();
    
    return 0;
}