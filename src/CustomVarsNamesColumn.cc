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

#include "CustomVarsNamesColumn.h"
#include "Row.h"

#ifdef CMC
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include "Object.h"
#include "cmc.h"
#else
#include "nagios.h"
#endif

std::vector<std::string> CustomVarsNamesColumn::getValue(
    Row row, const contact * /*auth_user*/,
    std::chrono::seconds /*timezone_offset*/) const {
    std::vector<std::string> names;
#ifdef CMC
    if (auto *object = columnData<Object>(row)) {
        auto attrs = object->customAttributes();
        std::transform(attrs.begin(), attrs.end(), std::back_inserter(names),
                       [](const auto &entry) { return entry.first; });
    }
#else
    if (auto p = columnData<customvariablesmember *>(row)) {
        for (auto cvm = *p; cvm != nullptr; cvm = cvm->next) {
            names.emplace_back(cvm->variable_name);
        }
    }
#endif
    return names;
}
