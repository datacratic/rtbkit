/** plugin_table_test.cc                                 -*- C++ -*-
    Sirma Cagil Altay, 22 Oct 2015
    Copyright (c) 2015 Datacratic.  All rights reserved.

    Tests for plugin table utilities

*/

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include "rtbkit/common/plugin_table.h"
#include "rtbkit/common/testing/custom_base_plugin.h"

#include <boost/test/unit_test.hpp>
#include <thread>

using namespace std;
using namespace RTBKIT;
using namespace TEST;

BOOST_AUTO_TEST_CASE(naming_convention_test_1)
{
    auto f = PluginTable<TestPlugin::Factory>::instance().getPlugin("custom_1","plugin");

    int num = f()->getNum();

    BOOST_REQUIRE_EQUAL(num,1);
}
