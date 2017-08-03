/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: test_aes.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.1.13   1.0     Create
******************************************************************************/

#include "test_xml.h"
#include <string>
#include <iostream>
#include <stdio.h>

using namespace std;

test_xml::test_xml()
{

}

test_xml::~test_xml()
{

}

void test_xml::test()
{
    pugi::xml_document config;
    std::string src = "<items id=\"123456\">100</items>";
    config.load_string(src.c_str());
    pugi::xml_node root = config.child("items");
    cout << root.text().as_string() << endl;
    cout << root.attribute("id").as_string() << endl;
}
