/* casale_exchange_connector.cc
   Mathieu Stefani, 05 December 2014
   Copyright (c) 2014 Datacratic.  All rights reserved.
   
   Implementation of the Casale Exchange Connector
*/

#include "casale_exchange_connector.h"
#include "soa/utils/generic_utils.h"

using namespace Datacratic;

namespace RTBKIT {

/*****************************************************************************/
/* CASALE EXCHANGE CONNECTOR                                                */
/*****************************************************************************/

CasaleExchangeConnector::CasaleExchangeConnector(
        ServiceBase& owner, std::string name) 
    : OpenRTBExchangeConnector(owner, std::move(name))
    , creativeConfig("casale")
{
    this->auctionResource = "/bidder";
    this->auctionVerb = "POST";
    initCreativeConfiguration();
}

CasaleExchangeConnector::CasaleExchangeConnector(
        std::string name, std::shared_ptr<ServiceProxies> proxies)
    : OpenRTBExchangeConnector(std::move(name), std::move(proxies))
    , creativeConfig("casale")
{
    this->auctionResource = "/bidder";
    this->auctionVerb = "POST";
    initCreativeConfiguration();
}

void CasaleExchangeConnector::initCreativeConfiguration()
{
    creativeConfig.addField(
        "adm",
        [](const Json::Value& value, CreativeInfo& info) {
            Datacratic::jsonDecode(value, info.adm);
            if (info.adm.empty()) {
                throw std::invalid_argument("adm can not be empty");
            }

            return true;
    }).snippet();
}



ExchangeConnector::ExchangeCompatibility
CasaleExchangeConnector::getCampaignCompatibility(
        const AgentConfig& config,
        bool includeReasons) const
{
    ExchangeCompatibility result;

    const char* name = exchangeName().c_str();
    if (!config.providerConfig.isMember(name)) {
        result.setIncompatible(
                ML::format("providerConfig.%s is null", name), includeReasons);
        return result;
    }

    const auto& provConf = config.providerConfig[name];
    if (!provConf.isMember("seat")) {
        result.setIncompatible(
               ML::format("providerConfig.%s.seat is null"), includeReasons);
        return result;
    }

    const auto& seat = provConf["seat"];
    if (!seat.isUInt()) {
        result.setIncompatible(
                ML::format("providerConfig.%s.seat is not merdiumint or unsigned", name),
                includeReasons);
        return result;
    }

    uint64_t value = seat.asUInt();
    if (value > CampaignInfo::MaxSeatValue) {
        result.setIncompatible(
                ML::format("providerConfig.%s.seat > %lld", name, CampaignInfo::MaxSeatValue),
                includeReasons);
        return result;
    }

    auto info = std::make_shared<CampaignInfo>(); 
    info->seat = value;

    result.info = info;
    return result;
}

std::shared_ptr<BidRequest>
CasaleExchangeConnector::parseBidRequest(
        HttpAuctionHandler& handler,
        const HttpHeader& header,
        const std::string& payload)
{
    HttpHeader fixedHeaders(header);
    auto it = fixedHeaders.headers.find("x-openrtb-version");
    if (it != std::end(fixedHeaders.headers) && it->second == "2.0") {
        it->second = "2.1";
    }

    auto request = OpenRTBExchangeConnector::parseBidRequest(handler, header, payload);

    return request;

}

void
CasaleExchangeConnector::setSeatBid(
        const Auction& auction,
        int spotNum,
        OpenRTB::BidResponse& response) const {

    const Auction::Data *current = auction.getCurrentData();

    auto& resp = current->winningResponse(spotNum);

    const AgentConfig* config
        = std::static_pointer_cast<const AgentConfig>(resp.agentConfig).get();
    std::string name = exchangeName();

    auto campaignInfo = config->getProviderData<CampaignInfo>(name);
    int creativeIndex = resp.agentCreativeIndex;

    auto& creative = config->creatives[creativeIndex];
    auto creativeInfo = creative.getProviderData<CreativeInfo>(name);

    // Find the index in the seats array
    int seatIndex = indexOf(response.seatbid, &OpenRTB::SeatBid::seat, Id(campaignInfo->seat));

    OpenRTB::SeatBid* seatBid;

    // Creative the seat if it does not exist
    if (seatIndex == -1) {
        OpenRTB::SeatBid sbid;
        sbid.seat = Id(campaignInfo->seat);

        response.seatbid.push_back(std::move(sbid));

        seatBid = &response.seatbid.back();
    }
    else {
        seatBid = &response.seatbid[seatIndex];
    }

    ExcAssert(seatBid);
    seatBid->bid.emplace_back();
    auto& bid = seatBid->bid.back();

    CasaleCreativeConfiguration::Context context {
        creative,
        resp,
        *auction.request
    };

    bid.cid = Id(resp.agent);
    bid.crid = Id(resp.creativeId);
    bid.impid = auction.request->imp[spotNum].id;
    bid.id = Id(auction.id, auction.request->imp[0].id);
    bid.price.val = USD_CPM(resp.price.maxPrice);

    bid.adomain = creativeInfo->adomain;
    bid.adm = creativeConfig.expand(creativeInfo->adm, context);

}


} // namespace RTBKIT

namespace {

struct Init {
    Init() {
        RTBKIT::ExchangeConnector::registerFactory<CasaleExchangeConnector>();
    }
};

struct Init init;
}

