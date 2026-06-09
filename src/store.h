#ifndef STORE_H
#define STORE_H

#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <optional>
#include <chrono>
#include <list>

using namespace std;

struct CacheItem{
    string key;
    string value;
    chrono :: time_point<chrono :: steady_clock> expiry;
    bool has_expiry;
};

class KeyValueStore{
    private:
        size_t max_capacity_;
        
        list<CacheItem> lru_list;


        unordered_map<string, list<CacheItem> :: iterator> store_;
        mutex mutex_;

    public:
        KeyValueStore(size_t capacity = 1000);

        void set(const string& key, const string&value, int ttl_seconds = -1);
        optional<string> get(const string& key);
        int del(const string& key);
};

#endif