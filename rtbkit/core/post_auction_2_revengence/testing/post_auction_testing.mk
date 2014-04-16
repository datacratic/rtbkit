#------------------------------------------------------------------------------#
# post_auction_testing.mk
# Rémi Attab (remi.attab@gmail.com), 16 Apr 2014
# FreeBSD-style copyright and disclaimer apply
#
# Build file for the post auction service tests
#------------------------------------------------------------------------------#

$(eval $(call program,post_auction_redis_bench,arch utils types redis boost_program_options))

