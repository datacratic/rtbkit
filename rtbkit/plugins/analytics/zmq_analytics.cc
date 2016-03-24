/* zmq_analytics.cc
   Sirma Cagil Altay, 7 Mar 2016
   Copyright (c) 2015 Datacratic.  All rights reserved.

   This is the base class used to build a custom event logger plugin
*/

#include "zmq_analytics.h"
#include "soa/types/date.h"
#include "rtbkit/core/router/router.h"
#include "jml/arch/format.h"
#include "soa/jsoncpp/json.h"
#include "rtbkit/common/auction.h"
#include "rtbkit/core/post_auction/events.h"
#include "rtbkit/common/auction_events.h"
#include "rtbkit/common/currency.h"
#include "rtbkit/common/bid_request.h"
#include "boost/algorithm/string/trim.hpp"

using namespace Datacratic;

namespace RTBKIT {

ZmqAnalytics::ZmqAnalytics(const std::string & service_name, std::shared_ptr<ServiceProxies> proxies)
    : Analytics(service_name+"/logger", proxies),
      zmq_publisher_(getZmqContext())
{}

ZmqAnalytics::~ZmqAnalytics() {}

void ZmqAnalytics::init()
{
    zmq_publisher_.init(getServices()->config, serviceName());
}

void ZmqAnalytics::bindTcp(const std::string & port_range)
{
    zmq_publisher_.bindTcp(getServices()->ports->getRange(port_range));
}

void ZmqAnalytics::start()
{
    zmq_publisher_.start();
}

void ZmqAnalytics::shutdown()
{
    zmq_publisher_.shutdown();
}


/**********************************************************************************************
* USED IN ROUTER
**********************************************************************************************/

void ZmqAnalytics::logMarkMessage(const Router & router, const double & last_check) 
{
    zmq_publisher_.publish("MARK",
                           Date::now().print(5),
                           Date::fromSecondsSinceEpoch(last_check).print(),
                           ML::format("active: %zd augmenting, %zd inFlight, "
                                      "%zd agents",
                                      router.augmentationLoop.numAugmenting(),
                                      router.inFlight.size(),                                             
                                      router.agents.size())
                           );
}

void ZmqAnalytics::logBidMessage(const BidMessage & message) 
{
    zmq_publisher_.publish("BID",
                           Date::now().print(5),
                           message.agents[0],
                           message.auctionId.toString(),
                           message.bids.toJson().toStringNoNewLine(),
                           message.meta
                           );
}

void ZmqAnalytics::logAuctionMessage(const std::shared_ptr<Auction> & auction)
{
    zmq_publisher_.publish("AUCTION", 
                           Date::now().print(5),
                           auction->id.toString(), 
                           auction->requestStr
                           );
}

void ZmqAnalytics::logConfigMessage(const std::string & agent, const std::string & config)
{
    zmq_publisher_.publish("CONFIG",
                           Date::now().print(5),
                           agent,
                           config
                          );
}

void ZmqAnalytics::logNoBudgetMessage(const BidMessage & message) 
{
    zmq_publisher_.publish("NOBUDGET",
                           Date::now().print(5),
                           message.agents[0],
                           message.auctionId.toString(),
                           message.bids.toJson().toStringNoNewLine(),
                           message.meta
                          );
}

void ZmqAnalytics::logMessage(const std::string & msg, const BidMessage & message)
{
    zmq_publisher_.publish(msg,
                         Date::now().print(5),
                         message.agents[0],
                         message.auctionId.toString(),
                         message.bids.toJson().toStringNoNewLine(),
                         message.meta
                        );

}

void ZmqAnalytics::logUsageMessage(Router & router, const double & period)
{
    std::string p = std::to_string(period);

    for (auto it = router.lastAgentUsageMetrics.begin();
         it != router.lastAgentUsageMetrics.end();) {
        if (router.agents.count(it->first) == 0) {
            it = router.lastAgentUsageMetrics.erase(it);
        }
        else {
            it++;
        }
    }

    for (const auto & item : router.agents) {
        auto & info = item.second;
        auto & last = router.lastAgentUsageMetrics[item.first];

        Router::AgentUsageMetrics newMetrics(info.stats->intoFilters,
                                     info.stats->passedStaticFilters,
                                     info.stats->passedDynamicFilters,
                                     info.stats->auctions,
                                     info.stats->bids);
        Router::AgentUsageMetrics delta = newMetrics - last;

        zmq_publisher_.publish("USAGE",
                               Date::now().print(5),
                               "AGENT", 
                               p, 
                               item.first,
                               info.config->account.toString(),
                               delta.intoFilters,
                               delta.passedStaticFilters,
                               delta.passedDynamicFilters,
                               delta.auctions,
                               delta.bids,
                               info.config->bidProbability);
        last = move(newMetrics);
    }

    {
        Router::RouterUsageMetrics newMetrics;
        int numExchanges = 0;
        float acceptAuctionProbability(0.0);

        router.forAllExchanges([&](std::shared_ptr<ExchangeConnector> const & item) {
            ++numExchanges;
            newMetrics.numRequests += item->numRequests;
            newMetrics.numAuctions += item->numAuctions;
            acceptAuctionProbability += item->acceptAuctionProbability;
        });
        newMetrics.numBids = router.numBids;
        newMetrics.numNoPotentialBidders = router.numNoPotentialBidders;
        newMetrics.numAuctionsWithBid = router.numAuctionsWithBid;

        Router:: RouterUsageMetrics delta = newMetrics - router.lastRouterUsageMetrics;


        zmq_publisher_.publish("USAGE",
                               Date::now().print(5),
                               "ROUTER", 
                               p, 
                               delta.numRequests,
                               delta.numAuctions,
                               delta.numNoPotentialBidders,
                               delta.numBids,
                               delta.numAuctionsWithBid,
                               acceptAuctionProbability / numExchanges);

        router.lastRouterUsageMetrics = move(newMetrics);
    }
}

void ZmqAnalytics::logErrorMessage(const std::string & error, const std::vector<std::string> & message)
{
    zmq_publisher_.publish("ERROR",
                           Date::now().print(5),
                           error,
                           message
                          );
}

void ZmqAnalytics::logRouterErrorMessage(const std::string & function,
                                         const std::string & exception, 
                                         const std::vector<std::string> & message)
{
    zmq_publisher_.publish("ROUTERERROR",
                           Date::now().print(5),
                           function,
                           exception,
                           message
                          );
}


/**********************************************************************************************
* USED IN PA
**********************************************************************************************/

void ZmqAnalytics::logMatchedWinLoss(const MatchedWinLoss & matchedWinLoss) 
{
    zmq_publisher_.publish(
            "MATCHED" + matchedWinLoss.typeString(),                // 0
            Date::now().print(5),                                   // 1

            matchedWinLoss.auctionId.toString(),                    // 2
            std::to_string(matchedWinLoss.impIndex),                // 3
            matchedWinLoss.response.agent,                          // 4
            matchedWinLoss.response.account.at(1, ""),              // 5

            matchedWinLoss.winPrice.toString(),                     // 6
            matchedWinLoss.response.price.maxPrice.toString(),      // 7
            std::to_string(matchedWinLoss.response.price.priority), // 8

            matchedWinLoss.requestStr,                              // 9
            matchedWinLoss.response.bidData.toJsonStr(),            // 10
            matchedWinLoss.response.meta,                           // 11

            // This is where things start to get weird.

            std::to_string(matchedWinLoss.response.creativeId),     // 12
            matchedWinLoss.response.creativeName,                   // 13
            matchedWinLoss.response.account.at(0, ""),              // 14

            matchedWinLoss.uids.toJsonStr(),                        // 15
            matchedWinLoss.meta,                                    // 16

            // And this is where we lose all pretenses of sanity.

            matchedWinLoss.response.account.at(0, ""),              // 17
            matchedWinLoss.impId.toString(),                        // 18
            matchedWinLoss.response.account.toString(),             // 19

            // Ok back to sanity now.

            matchedWinLoss.requestStrFormat,                        // 20
            matchedWinLoss.rawWinPrice.toString(),                  // 21
            matchedWinLoss.augmentations.toString()                 // 22
        );
}

void ZmqAnalytics::logMatchedCampaignEvent(const MatchedCampaignEvent & matchedCampaignEvent)
{
    zmq_publisher_.publish(
            "MATCHED" + matchedCampaignEvent.label,    // 0
            Date::now().print(5),                      // 1

            matchedCampaignEvent.auctionId.toString(), // 2
            matchedCampaignEvent.impId.toString(),     // 3
            matchedCampaignEvent.requestStr,           // 4

            matchedCampaignEvent.bid,                  // 5
            matchedCampaignEvent.win,                  // 6
            matchedCampaignEvent.campaignEvents,       // 7
            matchedCampaignEvent.visits,               // 8

            matchedCampaignEvent.account.at(0, ""),    // 9
            matchedCampaignEvent.account.at(1, ""),    // 10
            matchedCampaignEvent.account.toString(),   // 11

            matchedCampaignEvent.requestStrFormat      // 12
    );
}

void ZmqAnalytics::logUnmatchedEvent(const UnmatchedEvent & unmatchedEvent)
{
    zmq_publisher_.publish(
            // Use event type not label since label is only defined for campaign events.
            "UNMATCHED" + string(print(unmatchedEvent.event.type)),             // 0
            Date::now().print(5),                                               // 1

            unmatchedEvent.reason,                                              // 2
            unmatchedEvent.event.auctionId.toString(),                          // 3
            unmatchedEvent.event.adSpotId.toString(),                           // 4

            std::to_string(unmatchedEvent.event.timestamp.secondsSinceEpoch()), // 5
            unmatchedEvent.event.metadata.toJson()                              // 6
        );
}

void ZmqAnalytics::logPostAuctionErrorEvent(const PostAuctionErrorEvent & postAuctionErrorEvent)
{
    zmq_publisher_.publish("PAERROR",
                           Date::now().print(5),
                           postAuctionErrorEvent.key,
                           postAuctionErrorEvent.message);
}

void ZmqAnalytics::logPAErrorMessage(const std::string & function,
                                     const std::string & exception, 
                                     const std::vector<std::string> & message)
{
    zmq_publisher_.publish("PAERROR",
                           Date::now().print(5),
                           function,
                           exception,
                           message
                          );
}


/**********************************************************************************************
* USED IN MOCK ADSERVER CONNECTOR
**********************************************************************************************/

void ZmqAnalytics::logMockWinMessage(const PostAuctionEvent & event)
{
    zmq_publisher_.publish("WIN",
                           Date::now().print(3),
                           event.auctionId.toString(),
                           event.winPrice.toString(),
                           "0");
}

/**********************************************************************************************
* USED IN STANDARD ADSERVER CONNECTOR
**********************************************************************************************/

void ZmqAnalytics::logStandardWinMessage(const Json::Value & json) 
{
    zmq_publisher_.publish("WIN",
                           Date::now().print(3),
                           json["bidRequestId"].asString(),
                           json["impid"].asString(),
                           USD_CPM(json["price"].asDouble()).toString());
}

void ZmqAnalytics::logStandardEventMessage(const Json::Value & json, const UserIds & userIds)
{
    zmq_publisher_.publish(json["type"].asString(),
                           Date::now().print(3),
                           json["bidRequestId"].asString(),
                           json["impid"].asString(),
                           userIds.toString());
}

/**********************************************************************************************
* USED IN OTHER ADSERVER CONNECTORS
**********************************************************************************************/

void ZmqAnalytics::logAdserverEvent(const Json::Value & json)
{
    zmq_publisher_.publish(json["type"].asString(),
                           Date::now().print(3),
                           json["id"].asString(),
                           json["impid"].asString());
}

void ZmqAnalytics::logAdserverWin(const Json::Value & json, const std::string & accountKeyStr)
{
    zmq_publisher_.publish("WIN",
                           Date::now().print(3),
                           json["auctionId"].asString(),
                           json["adSpotId"].asString(),
                           accountKeyStr,
                           json["winPrice"].asString(),
                           json["dataCost"].asString());
}

void ZmqAnalytics::logAuctionEventMessage(const Json::Value & json, const std::string & userIdStr)
{
    std::string event = json["event"].asString();
    if (event == "click")
       event = "CLICK";
    else if (event == "conversion")
       event = "CONVERSION";

    zmq_publisher_.publish(event,
                           Date::fromSecondsSinceEpoch(json["timestamp"].asDouble()),
                           json["auctionId"].asString(),
                           json["adSpotId"].asString(),
                           userIdStr);
}

void ZmqAnalytics::logEventJson(const std::string & event, const Json::Value & json)
{
    std::string jsonStr = json.toString();
    boost::trim(jsonStr);
    zmq_publisher_.publish(event,
                           Date::fromSecondsSinceEpoch(json["timestamp"].asDouble()),
                           jsonStr);
}

void ZmqAnalytics::logDetailedWin(const Json::Value & json,
                                  const std::string & userIdStr,
                                  const std::string & campaign,
                                  const std::string & strategy,
                                  const std::string & bidTimeStamp)
{
    std::string jsonStr = json.toString();
    boost::trim(jsonStr);
    zmq_publisher_.publish("WIN",
                           Date::now().print(3),
                           jsonStr,
                           json["bid_request_id"].asString(),
                           json["spot_id"].asString(),
                           json["buyer_price"].asString(),
                           userIdStr,
                           campaign,
                           strategy,
                           bidTimeStamp);
}

} // namespace RTBKIT

namespace {

struct AtInit {
    AtInit()
    {
        PluginInterface<Analytics>::registerPlugin(
            "zmq",
            [](const std::string & service_name, std::shared_ptr<ServiceProxies> proxies)
            {
                return new RTBKIT::ZmqAnalytics(service_name, std::move(proxies));
            });
    }
} atInit;
    
} // anonymous namespace
