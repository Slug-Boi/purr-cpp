#include "../hashmap.hpp"
#include <iostream>
#include <string>
using namespace hashmap;
using namespace std;

int main() {
    HashMap<string, int> map = HashMap<string, int>(10);

    map.put("test", 10);

    cout << *map.get("test"); 
}
