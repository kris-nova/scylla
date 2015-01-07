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
 */

/*
 * Copyright 2015 Cloudius Systems
 *
 * Modified by Cloudius Systems
 */

#ifndef CQL3_TERM_HH
#define CQL3_TERM_HH

#include <memory>

namespace cql3 {

class terminal;

class term;

/**
 * A CQL3 term, i.e. a column value with or without bind variables.
 *
 * A Term can be either terminal or non terminal. A term object is one that is typed and is obtained
 * from a raw term (Term.Raw) by poviding the actual receiver to which the term is supposed to be a
 * value of.
 */
class term {
public:
    /**
     * Collects the column specification for the bind variables in this Term.
     * This is obviously a no-op if the term is Terminal.
     *
     * @param boundNames the variables specification where to collect the
     * bind variables of this term in.
     */
    virtual void collect_marker_specification(::shared_ptr<variable_specifications> bound_names) = 0;

    /**
     * Bind the values in this term to the values contained in {@code values}.
     * This is obviously a no-op if the term is Terminal.
     *
     * @param options the values to bind markers to.
     * @return the result of binding all the variables of this NonTerminal (or
     * 'this' if the term is terminal).
     */
    virtual ::shared_ptr<terminal> bind(::shared_ptr<term> term, const query_options& options) = 0;

    /**
     * A shorter for bind(values).get().
     * We expose it mainly because for constants it can avoids allocating a temporary
     * object between the bind and the get (note that we still want to be able
     * to separate bind and get for collections).
     */
    virtual bytes bind_and_get(::shared_ptr<term> term, const query_options& options) = 0;

    /**
     * Whether or not that term contains at least one bind marker.
     *
     * Note that this is slightly different from being or not a NonTerminal,
     * because calls to non pure functions will be NonTerminal (see #5616)
     * even if they don't have bind markers.
     */
    virtual bool contains_bind_marker() = 0;

    virtual bool uses_function(sstring ks_name, sstring function_name) const = 0;

    /**
     * A parsed, non prepared (thus untyped) term.
     *
     * This can be one of:
     *   - a constant
     *   - a collection literal
     *   - a function call
     *   - a marker
     */
    class raw : public virtual assignment_testable {
    public:
        /**
         * This method validates this RawTerm is valid for provided column
         * specification and "prepare" this RawTerm, returning the resulting
         * prepared Term.
         *
         * @param receiver the "column" this RawTerm is supposed to be a value of. Note
         * that the ColumnSpecification may not correspond to a real column in the
         * case this RawTerm describe a list index or a map key, etc...
         * @return the prepared term.
         */
        virtual std::unique_ptr<term> prepare(sstring keyspace, const column_specification& receiver) = 0;
    };

    class multi_column_raw : public virtual raw {
    public:
        virtual std::unique_ptr<term> prepare(sstring keyspace, const std::vector<column_specification>& receiver) = 0;
    };
};

    /**
     * A terminal term, one that can be reduced to a byte buffer directly.
     *
     * This includes most terms that don't have a bind marker (an exception
     * being delayed call for non pure function that are NonTerminal even
     * if they don't have bind markers).
     *
     * This can be only one of:
     *   - a constant value
     *   - a collection value
     *
     * Note that a terminal term will always have been type checked, and thus
     * consumer can (and should) assume so.
     */
    class terminal : public term {
    public:
        virtual void collect_marker_specification(::shared_ptr<variable_specifications> bound_names) {
        }

        virtual ::shared_ptr<terminal> bind(::shared_ptr<term> term, const query_options& options) override {
            return ::static_pointer_cast<terminal>(term);
        }

        virtual bool uses_function(sstring ks_name, sstring function_name) const override {
            return false;
        }

        // While some NonTerminal may not have bind markers, no Term can be Terminal
        // with a bind marker
        virtual bool contains_bind_marker() const {
            return false;
        }

        /**
         * @return the serialized value of this terminal.
         */
        virtual bytes get(::shared_ptr<term> term, const query_options& options) = 0;

        virtual bytes bind_and_get(::shared_ptr<term> term, const query_options& options) override {
            return get(term, options);
        }
    };

    class multi_item_terminal : public terminal {
    public:
        virtual std::vector<bytes> get_elements() = 0;
    };

    class collection_terminal {
    public:
        /** Gets the value of the collection when serialized with the given protocol version format */
        virtual bytes get_with_protocol_version(int protocol_version) = 0;
    };

    /**
     * A non terminal term, i.e. a term that can only be reduce to a byte buffer
     * at execution time.
     *
     * We have the following type of NonTerminal:
     *   - marker for a constant value
     *   - marker for a collection value (list, set, map)
     *   - a function having bind marker
     *   - a non pure function (even if it doesn't have bind marker - see #5616)
     */
    class non_terminal : public virtual term {
    public:
        virtual bool uses_function(sstring ks_name, sstring function_name) const override {
            return false;
        }

        virtual bytes bind_and_get(::shared_ptr<term> term, const query_options& options) override {
            auto t = bind(term, options);
            return t == nullptr ? nullptr : t->get(term, options);
        }
    };
}

#endif