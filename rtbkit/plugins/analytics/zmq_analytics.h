/* zmq_analytics.h
   Sirma Cagil Altay, 7 Mar 2016
   Copyright (c) 2015 Datacratic.  All rights reserved.

   This is a logger class that publishes events through Zmq
*/

#pragma once

#include <memory>
#include <functional>
#include <string>

#include "rtbkit/common/analytics.h"
#include "soa/service/zmq_named_pub_sub.h"

namespace RTBKIT {
    class Router; 
    class BidMessage; 
    class Auction; 
    struct MatchedWinLoss; 
    struct MatchedCampaignEvent;
    struct UnmatchedEvent;
    struct PostAuctionErrorEvent;
    struct PostAuctionEvent;
    struct UserIds;
}
namespace Datacratic {
    class ServiceProxies;
}
namespace Json {
    class Value;
}

namespace RTBKIT {

class ZmqAnalytics : public Analytics {

public:
    ZmqAnalytics(const std::string & service_name, std::shared_ptr<Datacratic::ServiceProxies> proxies);
    virtual ~ZmqAnalytics(); 

    virtual void init();
    virtual void bindTcp(const std::string & port_range = "logs");
    virtual void start();
    virtual void shutdown();

    // USED IN ROUTER 
    virtual void logMarkMessage(const Router & router, const double & last_check);
    virtual void logBidMessage(const BidMessage & message);
    virtual void logAuctionMessage(const std::shared_ptr<Auction> & auction);
    virtual void logConfigMessage(const std::string & agent, const std::string & config);
    virtual void logNoBudgetMessage(const BidMessage & message);
    virtual void logMessage(const std::string & msg, const BidMessage & message);
    virtual void logUsageMessage(Router & router, const double & period);
    virtual void logErrorMessage(const std::string & error,
                                 const std::vector<std::string> & message = std::vector<std::string>());
    virtual void logRouterErrorMessage(const std::string & function,
                                       const std::string & exception, 
                                       const std::vector<std::string> & message = std::vector<std::string>());

    // USED IN PA
    virtual void logMatchedWinLoss(const MatchedWinLoss & matchedWinLoss);
    virtual void logMatchedCampaignEvent(const MatchedCampaignEvent & matchedCampaignEvent);
    virtual void logUnmatchedEvent(const UnmatchedEvent & unmatchedEvent);
    virtual void logPostAuctionErrorEvent(const PostAuctionErrorEvent & postAuctionErrorEvent); 
    virtual void logPAErrorMessage(const std::string & function,
                                   const std::string & exception, 
                                   const std::vector<std::string> & message = std::vector<std::string>());

    // USED IN MOCK ADSERVER CONNECTOR
    virtual void logMockWinMessage(const PostAuctionEvent & event);

    // USED IN STANDARD ADSERVER CONNECTOR 
    virtual void logStandardWinMessage(const Json::Value & json);
    virtual void logStandardEventMessage(const Json::Value & json, const UserIds & userIds);

    // USED IN OTHER ADSERVER CONNECTORS
    virtual void logAdserverEvent(const Json::Value & json);
    virtual void logAdserverWin(const Json::Value & json, const std::string & accountKeyStr);
    virtual void logAuctionEventMessage(const Json::Value & json, const std::string & userIdStr);
    virtual void logEventJson(const std::string & event, const Json::Value & json);
    virtual void logDetailedWin(const Json::Value & json,
                                const std::string & userIdStr,
                                const std::string & campaign,
                                const std::string & strategy,
                                const std::string & bidTimeStamp);

private:
    Datacratic::ZmqNamedPublisher zmq_publisher_;

}; // class ZmqEventLogger

} // namespace RTBKIT
