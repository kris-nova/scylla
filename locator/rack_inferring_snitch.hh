/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modified by Cloudius Systems.
 * Copyright 2015 Cloudius Systems.
 */

#pragma once

#include "core/sstring.hh"
#include "gms/inet_address.hh"
#include "locator/abstract_network_topology_snitch.hh"

namespace locator {

using inet_address = gms::inet_address;

/**
 * A simple endpoint snitch implementation that assumes datacenter and rack information is encoded
 * in the 2nd and 3rd octets of the ip address, respectively.
 */
class rack_inferring_snitch : public abstract_network_topology_snitch {
public:
    virtual sstring get_rack(inet_address endpoint) override {
        return std::to_string((endpoint.raw_addr() >> 8) & 0xFF);
    }

    virtual sstring get_datacenter(inet_address endpoint) override {
        return std::to_string((endpoint.raw_addr() >> 16) & 0xFF);
    }
};

} // namespace locator