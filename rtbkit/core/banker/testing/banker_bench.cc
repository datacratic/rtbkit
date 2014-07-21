/* banker_bench.cc
   Wolfgang Sourdeau, 21 July 2014
   Copyright (c) 2014 Datacratic.  All rights reserved.

   Benchmark "setBudget" by invoking using the HTTP services from the
   MasterBanker from different clients and different transports.
*/

#include <memory>

#include "jml/arch/timers.h"
#include "soa/service/message_loop.h"
#include "soa/service/rest_request_router.h"
#include "soa/service/http_client.h"
#include "soa/service/http_rest_proxy.h"
#include "soa/utils/benchmarks.h"

#include "rtbkit/core/banker/master_banker.h"
#include "rtbkit/core/banker/slave_banker.h"


using namespace std;
using namespace Datacratic;
using namespace RTBKIT;


/* add an account */
void addAccountSync(HttpClient & client, const AccountKey & account)
{
    int done(false), status(0);

    auto onResponse = [&] (const HttpRequest & rq,
                           HttpClientError error,
                           int newStatus,
                           string && headers,
                           string && body) {
        status = newStatus;
        done = true;
        ML::futex_wake(done);
    };
    auto cbs = make_shared<HttpClientSimpleCallbacks>(onResponse);

    string body("{"
                "   \"adjustmentLineItems\" : {},"
                "   \"adjustmentsIn\" : {},"
                "   \"adjustmentsOut\" : {},"
                "   \"allocatedIn\" : {},"
                "   \"allocatedOut\" : {},"
                "   \"budgetDecreases\" : {},"
                "   \"budgetIncreases\" : {},"
                "   \"commitmentsMade\" : {},"
                "   \"commitmentsRetired\" : {},"
                "   \"lineItems\" : {},"
                "   \"md\" : {"
                "      \"objectType\" : \"Account\","
                "      \"version\" : 1"
                "   },"
                "   \"recycledIn\" : {},"
                "   \"recycledOut\" : {},"
                "   \"spent\" : {},"
                "   \"type\" : \"budget\""
                "}");
    HttpRequest::Content content(body, "application/json");

    RestParams params{{"accountName", account.toString()},
                      {"accountType", "budget"}};
    client.post("/v1/accounts", cbs, content, params);
    while (!done) {
        int oldDone(done);
        ML::futex_wait(done, oldDone);
    }
    ExcAssert(status == 200);
}

/* sets a budget of 100c MicroUSD, using the HttpClient class */
void setBudgetSyncHC(HttpClient & client, const AccountKey & key, bool isPut)
{
    int done(false), status(0);

    auto onResponse = [&] (const HttpRequest & rq,
                           HttpClientError error,
                           int newStatus,
                           string && headers,
                           string && body) {
        status = newStatus;
        done = true;
        ML::futex_wake(done);
    };
    auto cbs = make_shared<HttpClientSimpleCallbacks>(onResponse);

    HttpRequest::Content content(string("{\"USD/1M\":1000000000}"),
                                 "application/json");
    if (isPut)
        client.put("/v1/accounts/" + key[0] + "/budget", cbs, content);
    else
        client.post("/v1/accounts/" + key[0] + "/budget", cbs, content);
    while (!done) {
        int oldDone(done);
        ML::futex_wait(done, oldDone);
    }
    ExcAssert(status == 200);
}

/* sets a budget of 100c MicroUSD, using the HttpClient class */
void setBudgetSyncRP(HttpRestProxy & client, const AccountKey & key,
                     bool isPut)
{
    HttpRestProxy::Response response;

    HttpRestProxy::Content content(string("{\"USD/1M\":1000000000}"),
                                   "application/json");
    if (isPut)
        response = client.put(string("/v1/accounts/") + key[0] + "/budget",
                              content);
    else
        response = client.post(string("/v1/accounts/") + key[0] + "/budget",
                               content);
    ExcAssert(response.code() == 200);
}

/* sets a budget of X MicroUSD, using the RestProxy class */
void setBudgetSyncZMQ(RestProxy & client, const AccountKey & key)
{
    int done(false), status(0);

    auto onDone = [&] (std::exception_ptr,
                       int newStatus, const std::string & body) {
        status = newStatus;
        done = true;
        ML::futex_wake(done);
    };

    client.push(onDone,
                "PUT", "/v1/accounts/" + key[0] + "/budget",
                {}, string("{\"USD/1M\":1000000000}"));
    while (!done) {
        int oldDone(done);
        ML::futex_wait(done, oldDone);
    }
    ExcAssert(status == 200);
}

int main(int argc, char ** argv)
{
    size_t nAccounts(1);
    {
        char * envAccs = ::getenv("NUM_ACCOUNTS");
        if (envAccs) {
            nAccounts = stoi(envAccs);
        }
    }

    cerr << ("Running benchmarks with num accounts = "
             + to_string(nAccounts) + "\n");

    Benchmarks bms;
    shared_ptr<Benchmark> bm;

    auto proxies = std::make_shared<ServiceProxies>();

    MessageLoop httpLoop;
    httpLoop.start();

    MasterBanker masterBanker(proxies, "masterBanker");

    // Setup a master banker used to keep the canonical budget of the
    // various bidding agent accounts. The data contained in this service is
    // periodically persisted to redis.
    masterBanker.init(std::make_shared<NoBankerPersistence>());
    masterBanker.bindTcp();
    masterBanker.start();

    ML::sleep(1);

    /* operations using async http client */
    {
        auto client = make_shared<HttpClient>("http://localhost:15000");
        httpLoop.addSource("httpClient", client);
        client->waitConnectionState(AsyncEventSource::CONNECTED);

        for (int i = 0; i < nAccounts; i++) {
            AccountKey key{"testCampaign" + to_string(i),
                           "testStrategy" + to_string(i)};
            addAccountSync(*client, key);
        }

        cerr << "hc set budget put using HttpClient\n";
        bm.reset(new Benchmark(bms, "HttpClient-set-budget-put"));
        for (int i = 0; i < nAccounts; i++) {
            AccountKey key{"testCampaign" + to_string(i),
                           "testStrategy" + to_string(i)};
            setBudgetSyncHC(*client, key, true);
        }
        bm.reset();

        cerr << "hc set budget post using HttpClient\n";
        bm.reset(new Benchmark(bms, "HttpClient-set-budget-post"));
        for (int i = 0; i < nAccounts; i++) {
            AccountKey key{"testCampaign" + to_string(i),
                           "testStrategy" + to_string(i)};
            setBudgetSyncHC(*client, key, false);
        }
        bm.reset();

        httpLoop.removeSource(client.get());
        client->waitConnectionState(AsyncEventSource::DISCONNECTED);
    }

    {
        /* operations using http rest proxy */
        cerr << "rp set budget put using HttpRestProxy\n";
        bm.reset(new Benchmark(bms, "HttpRestProxy-set-budget-put"));
        HttpRestProxy restProxy("http://localhost:15000");
        for (int i = 0; i < nAccounts; i++) {
            AccountKey key{"testCampaign" + to_string(i),
                           "testStrategy" + to_string(i)};
            setBudgetSyncRP(restProxy, key, true);
        }
        bm.reset();

        cerr << "hc set budget post using HttpRestProxy\n";
        bm.reset(new Benchmark(bms, "HttpRestProxy-set-budget-post"));
        for (int i = 0; i < nAccounts; i++) {
            AccountKey key{"testCampaign" + to_string(i),
                    "testStrategy" + to_string(i)};
            setBudgetSyncRP(restProxy, key, false);
        }
        bm.reset();
    }

    /* operations using zmq "rest" client */
    {
        auto client = make_shared<RestProxy>();
        httpLoop.addSource("zmqClient", client);
        client->waitConnectionState(AsyncEventSource::CONNECTED);
        client->init(proxies->config, "masterBanker");

        cerr << "zmq set budget put using RestProxy\n";
        bm.reset(new Benchmark(bms, "RestProxy-set-budget-put"));
        for (int i = 0; i < nAccounts; i++) {
            AccountKey key{"testCampaign" + to_string(i),
                           "testStrategy" + to_string(i)};
            setBudgetSyncZMQ(*client, key);
        }
        bm.reset();

        httpLoop.removeSource(client.get());
        client->waitConnectionState(AsyncEventSource::DISCONNECTED);
    }

    bms.dumpTotals();

    exit(0);

    // Test is done; clean up time.
    // budgetController.shutdown();
    masterBanker.shutdown();
}
