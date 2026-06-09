#include "store.h"
#include <mutex>
#include <chrono>
#include <iostream>
using namespace std;

KeyValueStore :: KeyValueStore(size_t capacity) : max_capacity_(capacity){}

void KeyValueStore :: set(const string& key, const string &value, int ttl_seconds){
    lock_guard<mutex> lock(mutex_);

    auto now = chrono :: steady_clock :: now();
    bool has_expiry = (ttl_seconds > 0);
    auto expiry_time = has_expiry ? now + chrono :: seconds(ttl_seconds) : now;

    auto it = store_.find(key);
    if(it != store_.end()){
        it->second->value = value;
        it->second->has_expiry = has_expiry;
        it->second->expiry = expiry_time;

        lru_list.splice(lru_list.begin(), lru_list, it->second);
        return;
    }

    if(store_.size() >= max_capacity_){
        auto last = lru_list.back();
        store_.erase(last.key);
        lru_list.pop_back();
    }

    lru_list.push_front({key, value, expiry_time, has_expiry});

    store_[key] = lru_list.begin();
}

optional<string> KeyValueStore :: get(const string& key){
    lock_guard lock(mutex_);
    auto it = store_.find(key);

    if(it == store_.end()){
        return nullopt;
    }

    if(it->second->has_expiry){
        auto now = chrono :: steady_clock :: now();

        auto seconds_left = chrono::duration_cast<chrono :: seconds>(it->second->expiry - now).count();
        if(now > it->second->expiry){
            lru_list.erase(it->second);
            store_.erase(it);
            return nullopt;
        }
    }
    
    lru_list.splice(lru_list.begin(),lru_list, it->second);

    return it->second->value;
}

int KeyValueStore :: del(const string& key){
    lock_guard<mutex> lock(mutex_);

    auto it = store_.find(key);
    if(it == store_.end()){
        return 0;
    }

    lru_list.erase(it->second);
    store_.erase(it);
    return 1;
}