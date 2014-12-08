/* bidder_interface.cc
   Eric Robert, 2 April 2014
   Copyright (c) 2011 Datacratic.  All rights reserved.
*/

#include "jml/db/persistent.h"
#include "rtbkit/common/messages.h"
#include "bidder_interface.h"
#include "rtbkit/common/injection.h"

using namespace Datacratic;
using namespace RTBKIT;

BidderInterface::BidderInterface(ServiceBase & parent,
                                 std::string const & serviceName) :
    ServiceBase(serviceName, parent),
    router(nullptr),
    bridge(nullptr) {
}

BidderInterface::BidderInterface(std::shared_ptr<ServiceProxies> proxies,
                                 std::string const & serviceName) :
    ServiceBase(serviceName, proxies),
    router(nullptr),
    bridge(nullptr) {
}

void BidderInterface::setInterfaceName(const std::string &name) {
    this->name = name;
}

std::string BidderInterface::interfaceName() const {
    return name;
}

void BidderInterface::init(AgentBridge * value, Router * r) {
    router = r;
    bridge = value;
}

void BidderInterface::start()
{
}

void BidderInterface::shutdown()
{
}

namespace {
    typedef std::lock_guard<ML::Spinlock> Guard;
    static ML::Spinlock lock;
    static std::unordered_map<std::string, BidderInterface::Factory> factories;
}


BidderInterface::Factory getFactory(std::string const & name) {
  return getLibrary(name,
		    "bidder",
		    factories,
		    lock,
		    "bidder");
}

void BidderInterface::registerFactory(std::string const & name, Factory callback) {
    Guard guard(lock);
    if (!factories.insert(std::make_pair(name, callback)).second)
        throw ML::Exception("already had a bidder factory registered");
}


std::shared_ptr<BidderInterface> BidderInterface::create(
        std::string serviceName,
        std::shared_ptr<ServiceProxies> const & proxies,
        Json::Value const & json) {
    auto type = json.get("type", "unknown").asString();
    auto factory = getFactory(type);
    if(serviceName.empty()) {
        serviceName = json.get("serviceName", "bidder").asString();
    }

    return std::shared_ptr<BidderInterface>(factory(serviceName, proxies, json));
}

