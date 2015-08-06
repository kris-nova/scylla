/*
 * Copyright 2015 Cloudius Systems
 */

#include "api/api-doc/lsa.json.hh"
#include "api/lsa.hh"
#include "api/api.hh"

#include "http/exception.hh"
#include "utils/logalloc.hh"
#include "log.hh"

namespace api {

logging::logger logger("lsa-api");

void set_lsa(http_context& ctx, routes& r) {
    httpd::lsa_json::lsa_compact.set(r, [&ctx](std::unique_ptr<request> req) {
        logger.info("Triggering compaction");
        logalloc::shard_tracker().reclaim(std::numeric_limits<uint32_t>::max());
        return make_ready_future<json::json_return_type>(0);
    });
}

}