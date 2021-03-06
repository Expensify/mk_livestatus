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

#include "CustomVarsDictColumn.h"
#include <stdexcept>
#include <utility>
#include "CustomVarsDictFilter.h"
#include "Filter.h"
#include "Renderer.h"
#include "Row.h"
class Aggregator;

#ifdef CMC
#include "Object.h"
#include "cmc.h"
#else
#include "nagios.h"
#endif

void CustomVarsDictColumn::output(
    Row row, RowRenderer &r, const contact * /*auth_user*/,
    std::chrono::seconds /*timezone_offset*/) const {
    DictRenderer d(r);
    for (const auto &it : getValue(row)) {
        d.output(it.first, it.second);
    }
}

std::unique_ptr<Filter> CustomVarsDictColumn::createFilter(
    Filter::Kind kind, RelationalOperator relOp,
    const std::string &value) const {
    return std::make_unique<CustomVarsDictFilter>(kind, *this, relOp, value);
}

std::unique_ptr<Aggregator> CustomVarsDictColumn::createAggregator(
    AggregationFactory /*factory*/) const {
    throw std::runtime_error("aggregating on dictionary column '" + name() +
                             "' not supported");
}

std::unordered_map<std::string, std::string> CustomVarsDictColumn::getValue(
    Row row) const {
    std::unordered_map<std::string, std::string> dict;
#ifdef CMC
    if (auto *object = columnData<Object>(row)) {
        return object->customAttributes();
    }
#else
    if (auto p = columnData<customvariablesmember *>(row)) {
        for (auto cvm = *p; cvm != nullptr; cvm = cvm->next) {
            dict.emplace(cvm->variable_name, cvm->variable_value);
        }
    }
#endif
    return dict;
}
