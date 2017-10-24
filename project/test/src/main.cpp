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
#include <time.h>

using namespace std;

struct position_info
{
    int UniqSequenceNo;
    char InstrumentID[31];
    char BrokerID[11];
    string InvestorID;
    int YdPosition;
    int Position;
    int CombPosition;
    position_info()
        :UniqSequenceNo(-1)
        , YdPosition(0)
        , Position(0)
        , CombPosition(10)
    {
        memset(InstrumentID, 0, sizeof(InstrumentID));
        memset(BrokerID, 0, sizeof(BrokerID));
    }
};

struct instrument_positions
{
    position_info buy_field;
    position_info sell_field;
    position_info buy_field_arbitrage;
    position_info sell_field_arbitrage;
    position_info buy_field_hedge;
    position_info sell_field_hedge;
    position_info buy_field_maker;
    position_info sell_field_maker;
};

int main(int argc, char ** argv)
{
    instrument_positions positions;
    for (int i = 0; i < 8; i++) {
        position_info* ptr = reinterpret_cast<position_info*>
            (reinterpret_cast<char*>(&positions) + sizeof(position_info)*i);
        //if (ptr->UniqSequenceNo == -1) continue;
        cout << ptr->InvestorID << " " << ptr->CombPosition << endl;
    }

    cout << sizeof(string) << endl;
    getchar();
    return 0;
}