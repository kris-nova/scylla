/*
 * Copyright 2015 ScyllaDB
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

#include <map>
#include <seastar/core/future.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/gate.hh>

namespace utils {

/*
 * Small utility to order func()->post() operation
 * so that the "post" step is guaranteed to only be run
 * when all func+post-ops for lower valued keys (T) are
 * completed.
 */
template<typename T, typename Comp = std::less<T>>
class flush_queue {
private:
    typedef std::map<T, promise<>, Comp> map_type;
    typedef typename map_type::reverse_iterator reverse_iterator;

    map_type _map;
    // embed all ops in a seastar::gate as well
    // so we can effectively block incoming ops
    seastar::gate _gate;

    template<typename Func, typename... Args>
    static auto call_helper(Func&& func, future<Args...> f) {
        using futurator = futurize<std::result_of_t<Func(Args&&...)>>;
        try {
            return futurator::apply(std::forward<Func>(func), f.get());
        } catch (...) {
            return futurator::make_exception_future(std::current_exception());
        }
    }
public:
    flush_queue() = default;
    // we are repeatedly using lambdas with "this" captured.
    // allowing moving would not be wise.
    flush_queue(flush_queue&&) = delete;
    flush_queue(const flush_queue&) = delete;

    /*
     * Runs func() followed by post(), but guaranteeing that
     * all operations with lower <T> keys have completed before
     * post() is executed.
     *
     * Post is invoked on the return value of Func
     * Returns a future containing the result of post()
     *
     * Any exception from Func is forwarded to end result, but
     * in case of exception post is _not_ run.
     */
    template<typename Func, typename Post>
    auto run_with_ordered_post_op(T rp, Func&& func, Post&& post) {
        assert(!_map.count(rp));
        assert(_map.empty() || _map.rbegin()->first < rp);

        _gate.enter();
        _map.emplace(std::make_pair(rp, promise<>()));

        using futurator = futurize<std::result_of_t<Func()>>;

        return futurator::apply(std::forward<Func>(func)).then_wrapped([this, rp, post = std::forward<Post>(post)](typename futurator::type f) mutable {
            auto i = _map.find(rp);
            assert(i != _map.end());

            auto run_post = [this, post = std::forward<Post>(post), f = std::move(f), i]() mutable {
                assert(i == _map.begin());
                return call_helper(std::forward<Post>(post), std::move(f)).finally([this, i]() {
                    auto pr = std::move(i->second);
                    assert(i == _map.begin());
                    _map.erase(i);
                    _gate.leave();
                    pr.set_value();
                });
            };

            if (i == _map.begin()) {
                return run_post();
            }

            --i;
            auto pr = std::exchange(i->second, promise<>());
            return i->second.get_future().then(std::move(run_post)).finally([pr = std::move(pr)]() mutable {
                pr.set_value();
            });
        });
    }
private:
    // waits for the entry at "i" to complete (and thus all before it)
    future<> wait_for_pending(reverse_iterator i) {
        if (i == _map.rend()) {
            return make_ready_future<>();
        }
        auto pr = std::exchange(i->second, promise<>());
        return i->second.get_future().then([pr = std::move(pr)]() mutable {
            pr.set_value();
        });
    }
public:
    // Waits for all operations currently active to finish
    future<> wait_for_pending() {
        return wait_for_pending(_map.rbegin());
    }
    // Waits for all operations whose key is less than or equal to "rp"
    // to complete
    future<> wait_for_pending(T rp) {
        auto i = _map.upper_bound(rp);
        return wait_for_pending(reverse_iterator(i));
    }
    // Closes this queue
    future<> close() {
        return _gate.close();
    }
    // Poll-check that queue is still open
    void check_open_gate() {
        _gate.enter();
        _gate.leave();
    }
};

}
