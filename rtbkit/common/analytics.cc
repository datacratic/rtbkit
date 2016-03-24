/* analytics.cc
   Sirma Cagil Altay, 7 Mar 2016
   Copyright (c) 2015 Datacratic.  All rights reserved.

   This is the base class used to build a custom event logger plugin
*/

#include "analytics.h"
#include "rtbkit/core/router/router.h"
#include "rtbkit/core/post_auction/events.h"
#include "soa/jsoncpp/json.h"
#include "rtbkit/common/bid_request.h"

namespace RTBKIT {

std::string Analytics::libNameSufix() 
{ 
    return "analytics"; 
}

Analytics::
Analytics(const std::string & service_name, std::shared_ptr<Datacratic::ServiceProxies> proxies)
    : Datacratic::ServiceBase(service_name, proxies) {}

Analytics::~Analytics() {}

void Analytics::init() {}

void Analytics::bindTcp(const std::string & port_range) {}

void Analytics::start() {}

void Analytics::shutdown() {}


/**********************************************************************************************
* USED IN ROUTER
**********************************************************************************************/

void Analytics::logMarkMessage(const Router & router, const double & last_check) {}

void Analytics::logBidMessage(const BidMessage & message) {}

void Analytics::logAuctionMessage(const std::shared_ptr<Auction> & auction) {}

void Analytics::logConfigMessage(const std::string & agent, const std::string & config) {}

void Analytics::logNoBudgetMessage(const BidMessage & message) {}

void Analytics::logMessage(const std::string & msg, const BidMessage & message) {}

void Analytics::logUsageMessage(Router & router, const double & period) {}

void Analytics::logErrorMessage(const std::string & error, const std::vector<std::string> & message) {}

void Analytics::logRouterErrorMessage(const std::string & function,
                                      const std::string & exception,
                                      const std::vector<std::string> & message) {} 


/**********************************************************************************************
* USED IN PA
**********************************************************************************************/

void Analytics::logMatchedWinLoss(const MatchedWinLoss & matchedWinLoss) {}

void Analytics::logMatchedCampaignEvent(const MatchedCampaignEvent & matchedCampaignEvent) {}

void Analytics::logUnmatchedEvent(const UnmatchedEvent & unmatchedEvent) {}

void Analytics::logPostAuctionErrorEvent(const PostAuctionErrorEvent & postAuctionErrorEvent) {}

void Analytics::logPAErrorMessage(const std::string & function,
                                  const std::string & exception,
                                  const std::vector<std::string> & message) {} 

/**********************************************************************************************
* USED IN MOCK ADSERVER CONNECTOR
**********************************************************************************************/

void Analytics::logMockWinMessage(const PostAuctionEvent & event) {}

/**********************************************************************************************
* USED IN STANDARD ADSERVER CONNECTOR
**********************************************************************************************/

void Analytics::logStandardWinMessage(const Json::Value & json) {}

void Analytics::logStandardEventMessage(const Json::Value & json, const UserIds & userIds) {}

/**********************************************************************************************
* USED IN OTHER ADSERVER CONNECTORS
**********************************************************************************************/

void Analytics::logAdserverEvent(const Json::Value & json) {}

void Analytics::logAdserverWin(const Json::Value & json, const std::string & accountKeyStr) {}

void Analytics::logAuctionEventMessage(const Json::Value & json, const std::string & userIdStr) {}

void Analytics::logEventJson(const std::string & event, const Json::Value & json) {}

void Analytics::logDetailedWin(const Json::Value & json,
                                  const std::string & userIdStr,
                                  const std::string & campaign,
                                  const std::string & strategy,
                                  const std::string & bidTimeStamp) {}

} // namespace RTBKIT
