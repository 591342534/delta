#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <map>
#include <list>
#include "test_strand.h"
#include "test_deadline_timer.h"
#include "test_log.h"
#include "test_base.h"
#include "alarm.h"
#include "timer_manager.h"
void test_strand1(int argc, char* argv[])
{
    test_strand tt;

    //tt.test_strands();
    tt.test_service();
    getchar(); //»»ÐÐ·û
    getchar(); 
}

void test_deadline_timer1(int argc, char* argv[])
{
    test_deadline_timer tt;

    tt.test_timer_syn();
    //tt.test_timer_asyn();
    //tt.test_timer_asyn_loop();
    getchar(); //»»ÐÐ·û
    getchar();
}

void test_log1(int argc, char* argv[])
{
    test_log tt;
    tt.test();
    getchar(); //»»ÐÐ·û
    getchar();
}

void TestBase1(int argc, char* argv[])
{
    TestBase tt;
    tt.Test();
    getchar(); //»»ÐÐ·û
    getchar();
}

void TestTimer(int argc, char* argv[])
{
    alarm inst(9, 14, 0);
    TimerManager::get_timer_manager().RegisterTimer(500,
        std::bind(&alarm::on_timeout, &inst));
    TimerManager::get_timer_manager().start();
    getchar();
    getchar();
}

int main(int argc, char* argv[])
{
    int ch = '1';
    do{
        printf("the list: \n");
        printf("0: exit \n");
        printf("1: test strand \n");
        printf("2: test deadline_timer \n");
        printf("3: test log \n");
        printf("4: test base \n");
        printf("5: test timer \n");
        printf("please select your decide: ");
        ch = getchar();
        switch (ch) {
        case '0':
            printf("exit OK ~~\n");
            break;
        case '1':
            test_strand1(argc, argv);
            break;
        case '2':
            test_deadline_timer1(argc, argv);
            break;
        case '3':
            test_log1(argc, argv);
            break;
        case '4':
            TestBase1(argc, argv);
            break;
        case '5':
            TestTimer(argc, argv);
            break;
        default:
            printf("please input right decide~~\n");
            break;
        }
    } while (ch != '0');

    getchar();
    getchar();
    return 0;
}
