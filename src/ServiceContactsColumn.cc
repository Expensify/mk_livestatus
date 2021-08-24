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

#include "ServiceContactsColumn.h"
#include "Row.h"

#ifdef CMC
#include "ContactList.h"
#include "Object.h"
#include "cmc.h"
#else
#include <unordered_set>
#include "nagios.h"
#endif

std::vector<std::string> ServiceContactsColumn::getValue(
    Row row, const contact* /*auth_user*/,
    std::chrono::seconds /*timezone_offset*/) const {
#ifdef CMC
    if (auto object = columnData<Object>(row)) {
        return object->_contact_list->contactNames();
    }
    return {};
#else
    std::unordered_set<std::string> names;
    if (auto svc = columnData<service>(row)) {
        for (auto cm = svc->contacts; cm != nullptr; cm = cm->next) {
            names.insert(cm->contact_ptr->name);
        }
        for (auto cgm = svc->contact_groups; cgm != nullptr; cgm = cgm->next) {
            for (auto cm = cgm->group_ptr->members; cm != nullptr;
                 cm = cm->next) {
                names.insert(cm->contact_ptr->name);
            }
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
#endif
}
