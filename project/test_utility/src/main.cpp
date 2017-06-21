#include <stdio.h>
#include <iostream>

#include "test_nocopyable.h"
#include "test_singleton.h"

void test_nocopyable_func(int argc, char* argv[])
{
	test_nocopyable one;
	one.test();
	//test_nocopyable two = one;
	//two.test();
}

void test_singleton_func(int argc, char* argv[])
{
	test_singleton::create_instance();
	test_singleton::get_instance()->test();
	test_singleton::destory_instance();
}

int main(int argc, char* argv[])
{
    int ch = '1';
    do{
        printf("the list: \n");
        printf("0: exit \n");
        printf("1: test nocopyable \n");
        printf("2: test singleton \n");
        printf("please select your decide: ");
        ch = getchar();
        switch (ch) {
        case '0':
            printf("exit OK ~~\n");
            break;
        case '1':
			test_nocopyable_func(argc, argv);
            break;
        case '2':
			test_singleton_func(argc, argv);
            break;
        default:
            printf("please input right decide~~\n");
            break;
        }
    } while (0);
    //} while (ch != '0');

	getchar();
    getchar();
    return 0;
}
