// +------------------------------------------------------------------+
// |             ____ _               _        __  __ _  __           |
// |            / ___| |__   ___  ___| | __   |  \/  | |/ /           |
// |           | |   | '_ \ / _ \/ __| |/ /   | |\/| | ' /            |
// |           | |___| | | |  __/ (__|   <    | |  | | . \            |
// |            \____|_| |_|\___|\___|_|\_\___|_|  |_|_|\_\           |
// |                                                                  |
// | Copyright Mathias Kettner 2014             mk@mathias-kettner.de |
// +------------------------------------------------------------------+
//
// This file is part of Check_MK.
// The official homepage is at http://mathias-kettner.de/check_mk.
//
// check_mk is free software;  you can redistribute it and/or modify it
// under the  terms of the  GNU General Public License  as published by
// the Free Software Foundation in version 2.  check_mk is  distributed
// in the hope that it will be useful, but WITHOUT ANY WARRANTY;  with-
// out even the implied warranty of  MERCHANTABILITY  or  FITNESS FOR A
// PARTICULAR PURPOSE. See the  GNU General Public License for more de-
// tails. You should have  received  a copy of the  GNU  General Public
// License along with GNU Make; see the file  COPYING.  If  not,  write
// to the Free Software Foundation, Inc., 51 Franklin St,  Fifth Floor,
// Boston, MA 02110-1301 USA.

#include "OringFilter.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <ostream>
#include <type_traits>
#include <vector>
#include "AndingFilter.h"
#include "Filter.h"
#include "Row.h"

// static
std::unique_ptr<Filter> OringFilter::make(Kind kind, Filters subfilters) {
    Filters filters;
    for (const auto &filter : subfilters) {
        if (filter->is_tautology()) {
            return AndingFilter::make(kind, Filters());
        }
        auto disjuncts = filter->disjuncts();
        filters.insert(filters.end(),
                       std::make_move_iterator(disjuncts.begin()),
                       std::make_move_iterator(disjuncts.end()));
    }
    return filters.size() == 1 ? std::move(filters[0])
                               : std::make_unique<OringFilter>(
                                     kind, std::move(filters), Secret());
}

bool OringFilter::accepts(Row row, const contact *auth_user,
                          std::chrono::seconds timezone_offset) const {
    for (const auto &filter : _subfilters) {
        if (filter->accepts(row, auth_user, timezone_offset)) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<Filter> OringFilter::partialFilter(
    std::function<bool(const Column &)> predicate) const {
    Filters filters;
    std::transform(
        _subfilters.begin(), _subfilters.end(), std::back_inserter(filters),
        [&](const auto &filter) { return filter->partialFilter(predicate); });
    return make(kind(), std::move(filters));
}

std::optional<std::string> OringFilter::stringValueRestrictionFor(
    const std::string &column_name) const {
    std::optional<std::string> restriction;
    for (const auto &filter : _subfilters) {
        if (auto current = filter->stringValueRestrictionFor(column_name)) {
            if (!restriction) {
                restriction = current;  // First restriction? Take it.
            } else if (restriction != current) {
                return {};  // Different restrictions? Give up.
            }
        } else {
            return {};  // No restriction for subfilter? Give up.
        }
    }
    return restriction;
}

std::optional<int32_t> OringFilter::greatestLowerBoundFor(
    const std::string &column_name,
    std::chrono::seconds timezone_offset) const {
    std::optional<int32_t> result;
    for (const auto &filter : _subfilters) {
        if (auto glb =
                filter->greatestLowerBoundFor(column_name, timezone_offset)) {
            result = result ? std::min(*result, *glb) : glb;
        }
    }
    return result;
}

std::optional<int32_t> OringFilter::leastUpperBoundFor(
    const std::string &column_name,
    std::chrono::seconds timezone_offset) const {
    std::optional<int32_t> result;
    for (const auto &filter : _subfilters) {
        if (auto lub =
                filter->leastUpperBoundFor(column_name, timezone_offset)) {
            result = result ? std::max(*result, *lub) : lub;
        }
    }
    return result;
}

std::optional<std::bitset<32>> OringFilter::valueSetLeastUpperBoundFor(
    const std::string &column_name,
    std::chrono::seconds timezone_offset) const {
    std::optional<std::bitset<32>> result;
    for (const auto &filter : _subfilters) {
        if (auto foo = filter->valueSetLeastUpperBoundFor(column_name,
                                                          timezone_offset)) {
            result = result ? (*result | *foo) : foo;
        }
    }
    return result;
}

std::unique_ptr<Filter> OringFilter::copy() const {
    return make(kind(), disjuncts());
}

std::unique_ptr<Filter> OringFilter::negate() const {
    Filters filters;
    std::transform(_subfilters.begin(), _subfilters.end(),
                   std::back_inserter(filters),
                   [](const auto &filter) { return filter->negate(); });
    return AndingFilter::make(kind(), std::move(filters));
}

bool OringFilter::is_tautology() const { return false; }

bool OringFilter::is_contradiction() const { return _subfilters.empty(); }

Filters OringFilter::disjuncts() const {
    Filters filters;
    std::transform(_subfilters.begin(), _subfilters.end(),
                   std::back_inserter(filters),
                   [](const auto &filter) { return filter->copy(); });
    return filters;
}

Filters OringFilter::conjuncts() const {
    Filters filters;
    filters.push_back(copy());
    return filters;
}

std::ostream &OringFilter::print(std::ostream &os) const {
    for (const auto &filter : _subfilters) {
        os << *filter << "\\n";
    }
    switch (kind()) {
        case Kind::row:
            os << "Or";
            break;
        case Kind::stats:
            os << "StatsOr";
            break;
        case Kind::wait_condition:
            os << "WaitConditionOr";
            break;
    }
    return os << ": " << _subfilters.size();
}
