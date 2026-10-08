#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <sys/_types/_u_int64_t.h>
#include <type_traits>
#include <utility>
#include <functional>
#include <utility>
#include <concepts>
#include <cstddef>
#include <vector>

static constexpr size_t SIZE_THRESH =  4;
static constexpr size_t BUCKET_THRESH = 3; 

// REDO WITH G++ 15 features
template <typename K>
concept Hashable = requires(K k) {
  { std::hash<K>{}(k) } -> std::convertible_to<size_t>; 
  { k == k }       -> std::convertible_to<bool>;
};

namespace hashmap {
  
  template <typename K, typename V>
  class HashMap {
      private: 
        std::vector<std::vector<std::pair<K,V>>> buckets;
        size_t size_;
        int64_t version;

        std::vector<std::pair<K,V>>& get_bucket(const K& key) {
            size_t index = hash_mixed(key) % buckets.size(); 
            std::vector<std::pair<K,V>>& bucket = buckets[index];
            return bucket;
      }

      uint64_t hash_mixed(const K& key) {  
          return splitmix64(std::hash<K>{}(key));
      }

      static uint64_t splitmix64(const size_t x) {
          uint64_t z = (x + 0x9e3779b97f4a7c15ULL);
          z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
          z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
          return z ^ (z >> 31);
      }
      
      // const overload
      const std::vector<std::pair<K,V>>& get_bucket(const K& key) const {
            size_t index = hash_mixed(key) % buckets.size();
            std::vector<std::pair<K,V>>& bucket = buckets[index];
            return bucket;
      }

      public:
        HashMap(size_t size=1) {
            buckets = std::vector<std::vector<std::pair<K,V>>>(size);
            size = 0;
        }

        void rehash(size_t new_size) {
            std::vector<std::vector<std::pair<K,V>>> new_buckets = std::vector<std::vector<std::pair<K,V>>>(new_size);

            for (auto& bucket : buckets ) {
                for (auto& kv : bucket) {
                    size_t index = hash_mixed(kv.first) % new_buckets.size();
                    new_buckets[index].emplace_back(std::move(kv));
                }
            }

            version++;
            swap(buckets, new_buckets);
            return;
        }

        V& operator[](const K& key) {
            for (auto& kv : get_bucket(key) ) {
                if (kv.first == key ) {
                    return kv.second;
                }
            }

            if ((size_+1) * SIZE_THRESH > buckets.size() * BUCKET_THRESH) {
                rehash(buckets.size() * 2);
            }

            std::vector<std::pair<K,V>>& bucket = get_bucket(key); 

            bucket.emplace_back(key, V{});
            size_++;
  
            return bucket.back().second;
        }

        void put(const K& key, V value) {
            std::vector<std::pair<K,V>>& bucket = get_bucket(key);
            
            for (auto& kv : bucket ) {
                if (kv.first == key) {
                    kv.second = std::move(value); 
                    return;
                }
            }
            bucket.emplace_back(key, std::move(value));
            size_++;

            if (size_ * SIZE_THRESH > buckets.size() * BUCKET_THRESH) {
                rehash(buckets.size() * 2);
            }

            return; 
        }

        bool putIfAbsent(const K& key, V value) {
            std::vector<std::pair<K,V>>& bucket = get_bucket(key);
            
            for (auto& kv : bucket ) {
                if (kv.first == key) {
                    return false;
                }
            }
            
            bucket.emplace_back(key, std::move(value));
            size_++;

            if (size_ * SIZE_THRESH > buckets.size() * BUCKET_THRESH) {
                rehash(buckets.size() * 2);
            }

            return true;
        }


        // Please note if you remove a key any value pointer you get can have changed after the remove.
        // can't be a const member due to pointer return
        V* get(K key) {
            for (auto& kv : get_bucket(key) ) {
                if (kv.first == key) {
                  return &kv.second;
                }
            }
            return nullptr;
        }

        // const pointer-to-const caller can only read
        const V* get(const K& key) const {
            for (auto& kv : get_bucket(key)) {
                if (kv.first == key) return &kv.second;
            }
            return nullptr;
        }

        void remove(const K& key) {
            std::vector<std::pair<K,V>>& bucket = get_bucket(key);
            int before = bucket.size();
            bucket.erase(remove_if(bucket.begin(), bucket.end(), [&key](const std::pair<K, V>& kv) { return kv.first == key; } ), bucket.end());
            if (bucket.size() != before ) 
                size_--;
        }

        bool contains(const K& key) const {
            const std::vector<std::pair<K,V>>& bucket = get_bucket(key);

            return any_of(bucket.begin(), bucket.end(), [&key](const std::pair<K,V>& kv) { return kv.first == key;}); 
        }

        size_t size() const {
            return size_;
        }

        std::vector<K> keys() const {
            std::vector<K> collected;
            collected.reserve(size_);
            
            for (auto& bucket : buckets ) {
                for (auto& kv : bucket) {
                    collected.emplace_back(kv.first);
                }
            }
            return collected;
        }

        std::vector<V> values() const {
            std::vector<V> collected;
            collected.reserve(size_);
            
            for (auto& bucket : buckets ) {
                for (auto& kv : bucket) {
                    collected.emplace_back(kv.second);
                }
            }
            return collected;
        }

          std::vector<std::pair<K,V>> pairs() const {
            
            std::vector<std::pair<K,V>> collected;
            collected.reserve(size_);
            
            for (auto& bucket : buckets ) {
                for (auto& kv : bucket) {
                    collected.emplace_back(kv);
                }
            }
            return collected;
        }

        V getOrDefault(const K& key, const V& fallback) {
            // can't be a const here due to get not being a const 
            V* value = get(key);
            if ( value != nullptr ) {
                return *value;
            }
            return fallback;
        }

        void clear() {
            buckets = std::vector<std::vector<std::pair<K,V>>>(buckets.size());
        }

        void merge(const HashMap& other, bool overwrite=true) {
            if (this == &other) 
                return;

            for ( auto& bucket : other.buckets ) {
                for ( auto& kv : bucket ) {
                    if ( overwrite ) {
                        put(kv.first, kv.second);
                    } else {
                        putIfAbsent(kv.first, kv.second);
                    }
                }
            }
        }

        HashMap<K,V>& operator+= (const HashMap& other) {
            merge(other);
            return *this;
        }
  };  
}
