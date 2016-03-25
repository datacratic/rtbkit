/* analytics.h
   Sirma Cagil Altay, 7 Mar 2016
   Copyright (c) 2015 Datacratic.  All rights reserved.

   This is the base class used to build a custom analytics plugin
*/

#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "soa/service/service_base.h"
#include "rtbkit/core/router/router.h"
#include "rtbkit/core/post_auction/events.h"
#include "soa/jsoncpp/json.h"
#include "rtbkit/common/bid_request.h"

namespace RTBKIT {

class Analytics : public Datacratic::ServiceBase {

public:
    
    typedef std::function < Analytics * (const std::string &, 
                                           std::shared_ptr<Datacratic::ServiceProxies>) > Factory;
    static std::string libNameSufix() { return "analytics"; }
    
    Analytics(const std::string & service_name, std::shared_ptr<Datacratic::ServiceProxies> proxies)
        : Datacratic::ServiceBase(service_name, proxies) {}
    virtual ~Analytics() {}

    virtual void init() {}
    virtual void bindTcp(const std::string & port_range = "logs") {}
    virtual void start() {}
    virtual void shutdown() {}
   
    // USED IN ROUTER 
    virtual void logMarkMessage(const Router & router, const double & last_check) {}
    virtual void logBidMessage(const BidMessage & message) {}
    virtual void logAuctionMessage(const std::shared_ptr<Auction> & auction) {}
    virtual void logConfigMessage(const std::string & agent, const std::string & config) {}
    virtual void logNoBudgetMessage(const BidMessage & message) {}
    virtual void logMessage(const std::string & msg, const BidMessage & message) {}
    virtual void logUsageMessage(Router & router, const double & period) {}
    virtual void logErrorMessage(const std::string & error,
                                 const std::vector<std::string> & message = std::vector<std::string>()) {}
    virtual void logRouterErrorMessage(const std::string & function,
                                       const std::string & exception,
                                       const std::vector<std::string> & message = std::vector<std::string>())
                                       {} 

    // USED IN PA
    virtual void logMatchedWinLoss(const MatchedWinLoss & matchedWinLoss) {}
    virtual void logMatchedCampaignEvent(const MatchedCampaignEvent & matchedCampaignEvent) {}
    virtual void logUnmatchedEvent(const UnmatchedEvent & unmatchedEvent) {}
    virtual void logPostAuctionErrorEvent(const PostAuctionErrorEvent & postAuctionErrorEvent) {} 
    virtual void logPAErrorMessage(const std::string & function,
                                   const std::string & exception,
                                   const std::vector<std::string> & message = std::vector<std::string>()) {} 

    // USED IN MOCK ADSERVER CONNECTOR
    virtual void logMockWinMessage(const PostAuctionEvent & event) {}

    // USED IN STANDARD ADSERVER CONNECTOR 
    virtual void logStandardWinMessage(const Json::Value & json) {}
    virtual void logStandardEventMessage(const Json::Value & json, const UserIds & userIds) {}

    // USED IN OTHER ADSERVER CONNECTORS
    virtual void logAdserverEvent(const Json::Value & json) {}
    virtual void logAdserverWin(const Json::Value & json, const std::string & accountKeyStr) {}
    virtual void logAuctionEventMessage(const Json::Value & json, const std::string & userIdStr) {}
    virtual void logEventJson(const std::string & event, const Json::Value & json) {}
    virtual void logDetailedWin(const Json::Value & json,
                                const std::string & userIdStr,
                                const std::string & campaign,
                                const std::string & strategy,
                                const std::string & bidTimeStamp) {}
}; // class Analytics

} // namespace RTBKIT

