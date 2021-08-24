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

#include "DoubleColumn.h"
#include "Aggregator.h"
#include "DoubleAggregator.h"
#include "DoubleFilter.h"
#include "Filter.h"
#include "Renderer.h"
#include "Row.h"

void DoubleColumn::output(Row row, RowRenderer &r,
                          const contact * /*auth_user*/,
                          std::chrono::seconds /*timezone_offset*/) const {
    r.output(getValue(row));
}

std::unique_ptr<Filter> DoubleColumn::createFilter(
    Filter::Kind kind, RelationalOperator relOp,
    const std::string &value) const {
    return std::make_unique<DoubleFilter>(kind, *this, relOp, value);
}

std::unique_ptr<Aggregator> DoubleColumn::createAggregator(
    AggregationFactory factory) const {
    return std::make_unique<DoubleAggregator>(factory, this);
}
