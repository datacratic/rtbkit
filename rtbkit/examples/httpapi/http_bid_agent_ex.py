#!/usr/bin/env python
"""HTTPBidAgent

This module builds on BaseHTTPServer by implementing GET and POST
requests to respond to BID requests
"""

__version__ = "0.1"
__all__ = ["OpenRtb_response",
           "FixedPriceBidderMixIn",
           "TornadoBaseBidAgentRequestHandler",
           "TornadoFixPriceBidAgentRequestHandler",
           "BudgetPacer"]
           

# IMPORTS

# util libs
import urllib
from copy import deepcopy
import json

# tornado web
from tornado import process
from tornado import netutil
from tornado import httpserver
from tornado.web import RequestHandler, Application, url
from tornado.ioloop import IOLoop
from tornado.httpclient import AsyncHTTPClient
from tornado.ioloop import PeriodicCallback


# IMPLEMENTATION


class OpenRtb_response():
    """this is a helper class to build basic OpenRTB json objects"""

    # field names - constants to avoid magic strings inside the function
    key_id = "id"
    key_bid = "bid"
    key_crid = "crid"
    key_ext = "ext"
    key_extid = "external-id"
    key_priority = "priority"
    key_impid = "impid"
    key_price = "price"
    key_seatbid = "seatbid"

    # template obejcts
    bid_object = {key_id: "1",
                  key_impid: "1",
                  key_price: 1.0,
                  key_crid: "",
                  key_ext: {key_priority: 1.0}}
    seat_bid_object = {key_bid: [deepcopy(bid_object)]}
    bid_response_object = {key_id: "1",
                           key_seatbid: [deepcopy(seat_bid_object)]}

    def get_empty_response(self):
        """returns an object with the scafold of an rtb response
        but containing only default values"""
        empty_resp = deepcopy(self.bid_response_object)

        return empty_resp

    def get_default_response(self, req):
        """returns an object with the scafold of an rtb response
        and fills some fields based on the request provided"""

        default_resp = None

        if (self.validate_req(req)):
            # since this is a valid request we can return a response
            default_resp = deepcopy(self.bid_response_object)

            # copy request id
            default_resp[self.key_id] = req[self.key_id]

            # empty the bid list (we assume only one seat bid for simplicity)
            default_resp[self.key_seatbid][0][self.key_bid] = []

            # default values for some of the fields of the bid response
            id_counter = 0
            new_bid = deepcopy(self.bid_object)

            # iterate over impressions array from request and
            # populate bid list
            for imp in req["imp"]:
                # -> imp is the field name @ the req

                # dumb bid id, just a counter
                id_counter = id_counter + 1
                new_bid[self.key_id] = str(id_counter)

                # copy impression id as imp for this bid
                new_bid[self.key_impid] = imp[self.key_id]

                externalId = 0
                # copy external id to the response
                try:
                    externalId = imp[self.key_ext]["external-ids"][0]
                    new_bid[self.key_ext][self.key_extid] = externalId
                except:
                    externalId = -1  # and do not add this fiel

                # will keep the defaul price as it'll be changed by bidder
                # and append this bid into the bid response
                ref2bidList = default_resp[self.key_seatbid][0][self.key_bid]
                ref2bidList.append(deepcopy(new_bid))

        return default_resp

    def validate_req(self, req):
        """ validates the fields in the request"""
        # not implemented yet. should check if the structure from
        # the request is accroding to the spec
        valid = True
        return valid


class FixedPriceBidderMixIn():
    """Dumb bid agent Mixin that bid 100% at $1"""

    # mixins do not have their __init__ (constructor) called
    # so this class do not have it and the load of configuration
    # have to be dealt with by the class that incorporates it!!!

    bid_config = None
    openRtb = OpenRtb_response()

    def do_config(self):
        cfg = open("http_config.json")
        data = json.load(cfg)
        self.bid_config = {}
        self.bid_config["probability"] = data["bidProbability"]
        self.bid_config["price"] = 1.0
        self.bid_config["creatives"] = data["creatives"]

    def do_bid(self, req):
        # -------------------
        # bid logic
        # since this is a fix price bidder,
        # just mapping the request to the response
        # and using the default price ($1) will do the work.
        # -------------------

        # get defaul response
        resp = self.openRtb.get_default_response(req)

        # update bid with price and creatives
        crndx = 0
        ref2seatbid0 = resp[OpenRtb_response.key_seatbid][0]
        for bid in ref2seatbid0[OpenRtb_response.key_bid]:
            bid[OpenRtb_response.key_price] = self.bid_config["price"]
            creativeId = str(self.bid_config["creatives"][crndx]["id"])
            bid[OpenRtb_response.key_crid] = creativeId
            crndx = (crndx + 1) % len(self.bid_config["creatives"])

        return resp


# tornado request handler class extend
# this class is a general bid Agent hadler.
# bid processing must be implemented in a derived class

class TornadoBaseBidAgentRequestHandler(RequestHandler):
    """ extends tornado handler to answer openRtb requests"""
    def post(self):
        result_body = self.process_req()
        self.write(result_body)

    def get(self):
        result_body = self.process_req()
        self.write(result_body)

    def process_req(self):
        """processes post requests"""

        ret_val = ""

        if self.request.headers["Content-Type"].startswith("application/json"):
            req = json.loads(self.request.body)
        else:
            req = None

        if (req is not None):
            resp = self.process_bid(req)

            if (resp is not None):
                self.set_status(200)
                self.set_header("Content-type", "application/json")
                self.set_header("x-openrtb-version", "2.1")
                ret_val = json.dumps(resp)
            else:
                self.set_status(204)
                ret_val = "Error\n"
        else:
            self.set_status(204)
            ret_val = "Error\n"

        return ret_val

    def process_bid(self, req):
        """---TBD in subclass---"""
        resp = None
        print("got response")
        return resp


class TornadoFixPriceBidAgentRequestHandler(TornadoBaseBidAgentRequestHandler,
                                            FixedPriceBidderMixIn):
    """ This class extends TornadoBaseBidAgentRequestHandler
    The bidding logic is provided by a external object passed as
    parameter to the constructor"""

    def __init__(self, application, request, **kwargs):
        """constructor just call parent INIT and run MixIn's do_config"""
        super(TornadoBaseBidAgentRequestHandler, self).__init__(application, request, **kwargs)
        if (self.bid_config is None):
            self.do_config()

    def process_bid(self, req):
        """process bid request by calling bidder mixin do_bid() method"""
        resp = self.do_bid(req)
        return resp


class BudgetPacer(object):
    """send rest requests to the bancker to pace the budget)"""
    
    def config(self, banker_address, amount):
        """config pacer"""
        post_data = {"USD/1M": amount}
        self.body = urllib.urlencode(post_data)
        self.headers = {"Accept": "application/json"}
        self.url = "http://" + banker_address[0]
        self.url = self.url + ":" + banker_address[1]
        self.url = self.url + "/v1/accounts/hello:world/balance"
        self.http_client = AsyncHTTPClient()

    def http_request(self):
        """called periodically to updated the budget"""
        print("pacing budget!")
        # self.http_client.fetch(self.url, callback=None, method='POST', headers=self.headers, body=self.body)


# test functions
def tornado_bidder_run():
    app = Application([url(r"/", TornadoFixPriceBidAgentRequestHandler)])
    return app


# run test of this module
if __name__ == '__main__':
    # -- tornado advanced multi-process http server
    sockets = netutil.bind_sockets(7654)

    # fork working processes
    process.fork_processes(0)

    # Tornado app implementation
    app = tornado_bidder_run()

    # start http servers
    server = httpserver.HTTPServer(app)
    server.add_sockets(sockets)

    process_counter = process.task_id()
    # perform this action only in the parent process
    if (process_counter == 0):
        # --instantiate pacer
        pacer = BudgetPacer()
        pacer.config(("129.168.168.229", "9876"), 10000)

        # add periodic event
        PeriodicCallback(pacer.http_request, 60000).start()  # 60 sec

    # main io loop
    IOLoop.instance().start()


