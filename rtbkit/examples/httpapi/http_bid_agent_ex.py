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
           "HTTPBaseBidAgentRequestHandler",
           "HTTPFixPriceBidAgentRequestHandler"]
           

# IMPORTS

# util libs
import shutil
from copy import deepcopy
# barebones HTTP server
import BaseHTTPServer
# tries to use the fast string IO lib, otherwise fall back to standard one
try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO
# json processing lib
import json

# tornado web
from tornado import process
from tornado import netutil
from tornado import httpserver
from tornado.web import RequestHandler, Application, url
from tornado.ioloop import IOLoop


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


# extends request handler class to deal with bids
# this class is a general bid Agent hadler.
# bid processing must be implemented in a derived class

class HTTPBaseBidAgentRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    """BidAgent HTTP request handler with GET and POST commands.
    This processes JSON bid request and sent JSON bid responses
    The GET and POST requests are identical
    There is no bid logic in this class. This is just a scafold class to
    build other bidders with specific logic"""

    server_version = "HTTPBidAgent/" + __version__

    def do_GET(self):
        """ Serve a GET request."""
        f = self.process_JSON_req()
        if f:
            shutil.copyfileobj(f, self.wfile)
            f.close()

    def do_POST(self):
        """ Serve a POST request."""
        f = self.process_JSON_req()
        if f:
            shutil.copyfileobj(f, self.wfile)
            f.close()

    def process_JSON_req(self):
        """ Process the JSON Req and return a JSON response """
        # create string files for the input and output
        fout = StringIO()

        # read content
        content_type = self.headers.get('Content-Type', "")
        content_len = int(self.headers.get('Content-Length', "0"))
        lineStr = self.rfile.read(content_len)
        
        # check if the content type is correct
        if (content_type == "application/json"):

            try:
                # process JSON if fails treat as exception
                req = self.decode_json(lineStr)

            except:
                # can't bid if request message is not properly formated
                print("can't bid if request message is not properly formated")
                self.http204_response()

            # req object will be processed by bidder class
            resp = self.process_bid(req)

            if (resp is not None):
                # prepare response
                resp_str = self.encode_json(resp)
                fout.write(resp_str)
                resp_len = fout.tell()
                fout.seek(0)

                # response headers
                self.http200_response(str(resp_len))

            else:
                # response is empty
                print("response is empty")
                self.http204_response()
                
        else:
            # can't bid if request message is not of the rigth content_type
            print("can't bid: request message is not of proper content_type")
            self.http204_response()
       
        return fout

    def decode_json(self, jsonStr):
        """ converts the HTML body String into object"""
        ss = StringIO(jsonStr)
        ret = json.load(ss)
        print("json decoded ------------------------------------")
        print(jsonStr)
        return ret

    def encode_json(self, jsonObj):
        """ converts an oject into a string to use as response"""
        ret = json.dumps(jsonObj)
        print("json encoded ------------------------------------")
        print(ret)
        return ret

    def process_bid(self, req):
        """---TBD in subclass---"""
        resp = None
        print("got response")
        return resp

    def http200_response(self, length):
        """ return valid headers"""
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Content-Length", length)
        self.send_header("x-openrtb-version", "2.1")
        self.end_headers()

    def http204_response(self):
        """ default answer in case there are some error
        invalid request answers as code 204
        """
        self.send_response(204)
        self.send_header("Content-Length", "0")
        self.end_headers()


class HTTPFixPriceBidAgentRequestHandler(HTTPBaseBidAgentRequestHandler,
                                         FixedPriceBidderMixIn):
    """This class extends HTTPBaseBidAgentRequestHandler
    The bidding logic is provided by a external object passed as
    parameter to the constructor"""

    def process_bid(self, req):
        """process bid request by calling bidder mixin do_bid() method"""
        if (self.bid_config is None):
            self.do_config()

        resp = self.do_bid(req)
        return resp


# test functions
def http_bidder_run():
    """start a http bid agent for test purposes"""
    server_address = ('', 7654)
    app = BaseHTTPServer(server_address, HTTPFixPriceBidAgentRequestHandler)
    return app


def tornado_bidder_run():
    app = Application([url(r"/", TornadoFixPriceBidAgentRequestHandler)])
    return app


# run test of this module
if __name__ == '__main__':
    # Tornado implementation
    app = tornado_bidder_run()

    # -- tornado advanced multi-process http server
    sockets = netutil.bind_sockets(7654)
    process.fork_processes(0)
    server = httpserver.HTTPServer(app)
    server.add_sockets(sockets)

    # -- tornado simple multi-process http server
    # server = httpserver.HTTPServer(app)
    # server.bind(7654)
    # server.start(0)  # Forks multiple sub-processes

    # -- tornado single process http server
    # server = httpserver.HTTPServer(app)
    # server.listen(7654)

    # -- tornado addp class only
    # app.listen(7654)

    # io loop
    IOLoop.instance().start()

    # -------------
    # default Python HTTP Server implementation
    # app = http_bidder_run()
    # app.serve_forever()



