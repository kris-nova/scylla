/*
 * Copyright 2015 Cloudius Systems
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <chrono>
/**
 * A helper class to keep track of latencies
 */
namespace utils {

class latency_counter {
public:
    using time_point = std::chrono::system_clock::time_point;
    using duration = std::chrono::system_clock::duration;
private:
    time_point _start;
    time_point _stop;
public:
    void start() {
        _start = now();
    }

    bool is_start() const {
        // if start is not set it is still zero
        return _start.time_since_epoch().count();
    }
    latency_counter& stop() {
        _stop = now();
        return *this;
    }

    bool is_stopped() const {
        // if stop was not set, it is still zero
        return _stop.time_since_epoch().count();
    }

    duration latency() const {
        return _stop - _start;
    }

    latency_counter& check_and_stop() {
        if (!is_stopped()) {
            return stop();
        }
        return *this;
    }

    int64_t latency_in_nano() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(latency()).count();
    }

    static time_point now() {
        return std::chrono::system_clock::now();
    }
};

}
