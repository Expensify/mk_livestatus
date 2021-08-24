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

#include "ServiceSpecialDoubleColumn.h"
#include "Row.h"

#ifdef CMC
#include "HostSpecialDoubleColumn.h"
class Object;
#else
#include <cstring>
#include <ctime>
#include "nagios.h"
#endif

double ServiceSpecialDoubleColumn::getValue(Row row) const {
#ifdef CMC
    if (auto object = columnData<Object>(row)) {
        switch (_type) {
            case Type::staleness:
                return HostSpecialDoubleColumn::staleness(object);
        }
    }
#else
    if (auto svc = columnData<service>(row)) {
        switch (_type) {
            case Type::staleness: {
                extern int interval_length;
                auto check_result_age =
                    static_cast<double>(time(nullptr) - svc->last_check);
                if (svc->check_interval != 0) {
                    return check_result_age /
                           (svc->check_interval * interval_length);
                }

                // check_mk PASSIVE CHECK without check interval uses the check
                // interval of its check-mk service
                bool is_cmk_passive =
                    strncmp(svc->check_command_ptr->name, "check_mk-", 9) == 0;
                if (is_cmk_passive) {
                    host *host = svc->host_ptr;
                    for (servicesmember *svc_member = host->services;
                         svc_member != nullptr; svc_member = svc_member->next) {
                        service *tmp_svc = svc_member->service_ptr;
                        if (strncmp(tmp_svc->check_command_ptr->name,
                                    "check-mk", 9) == 0) {
                            return check_result_age /
                                   ((tmp_svc->check_interval == 0
                                         ? 1
                                         : tmp_svc->check_interval) *
                                    interval_length);
                        }
                    }
                    // Shouldn't happen! We always expect a check-mk service
                    return 1;
                }
                // Other non-cmk passive and active checks without
                // check_interval
                return check_result_age / interval_length;
            }
        }
    }
#endif
    return 0;
}
