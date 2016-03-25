# analytics makefile

$(eval $(call library,analytics_endpoint,analytics_endpoint.cc,services))
$(eval $(call program,analytics_runner,analytics_endpoint boost_program_options))

$(eval $(call library,zmq_analytics,zmq_analytics.cc,zmq services rtb_router))

$(eval $(call include_sub_make,analytics_testing,testing,analytics_testing.mk))

# Build of adserver_connector, rtb_router and post_auction depends on the build of zmq_analytics
# ie: make sure that zmq_analytics is built before building adserver_connector, rtb_router and post_auction
# so that when the libzmq_analytics will be called dynamically in adserver, router and post_auction 
# its library will sure be ready to load
$(LIB)/libadserver_connector.so $(LIB)/librtb_router.so $(LIB)/libpost_auction.so: $(LIB)/libzmq_analytics.so
