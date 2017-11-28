#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <stack>
#include <deque>
#include <map>
#include <unordered_map>
#include <functional>
#include <numeric>
#include <thread>
#include <chrono>
#include <mutex>
#include <exception>
#include <iomanip>
#include <algorithm>
#include <utility>
#include <tuple>
#include <chrono>
#include <random>
#include <time.h>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <typeindex>
#include <exception>
using namespace std;

struct riskapi_position_field
{
    std::string instrument;
    int direction;
    int hedge_flag;
    int volume;
    riskapi_position_field()
        : instrument("")
        , direction(-1)
        , hedge_flag(-1) {}

    bool operator < (const riskapi_position_field& rhs) const
    {
        if (instrument < rhs.instrument) {
            return true;
        }
        else if (instrument == rhs.instrument && direction < rhs.direction) {
            return true;
        }
        else if (instrument == rhs.instrument && direction == rhs.direction
            && hedge_flag < rhs.hedge_flag) {
            return true;
        }
        else {
            return false;
        }

    }
};


int main(int argc, char *argv[])
{
    set<riskapi_position_field> inst;
    riskapi_position_field one;
    one.instrument = "m1804";
    one.direction = 1;
    one.hedge_flag = 2;

    inst.insert(one);

    one.direction = 1;
    auto it = inst.find(one);
    one.direction = 1;
    inst.insert(one);
    one.instrument = "m1712";
    inst.insert(one);
    one.hedge_flag = 3;
    inst.insert(one);
    one.volume = 100;
    inst.insert(one);
    one.direction = 2;
    cout << inst.size() << endl;
    getchar();
    return 0;
}