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

#include "OffsetStringMacroColumn.h"
#include <cstdlib>
#include <cstring>
#include "Column.h"
#include "RegExp.h"
#include "Row.h"

std::string OffsetStringMacroColumn::getValue(Row row) const {
    if (auto p = columnData<void>(row)) {
        auto s = offset_cast<const char *>(p, _string_offset);
        return *s == nullptr ? ""
                             : expandMacros(*s, getHost(row), getService(row));
    }
    return "";
}

// static
std::string OffsetStringMacroColumn::expandMacros(const std::string &raw,
                                                  const host *hst,
                                                  const service *svc) {
    // search for macro names, beginning with $
    std::string result;
    const char *scan = raw.c_str();
    while (*scan != 0) {
        const char *dollar = strchr(scan, '$');
        if (dollar == nullptr) {
            result += scan;
            break;
        }
        result += std::string(scan, dollar - scan);
        const char *otherdollar = strchr(dollar + 1, '$');
        if (otherdollar == nullptr) {  // unterminated macro, do not expand
            result += dollar;
            break;
        }
        std::string macroname =
            std::string(dollar + 1, otherdollar - dollar - 1);
        const char *replacement = expandMacro(macroname.c_str(), hst, svc);
        if (replacement != nullptr) {
            result += replacement;
        } else {
            result += std::string(
                dollar, otherdollar - dollar + 1);  // leave macro unexpanded
        }
        scan = otherdollar + 1;
    }
    return result;
}

// static
const char *OffsetStringMacroColumn::expandMacro(const char *macroname,
                                                 const host *hst,
                                                 const service *svc) {
    // host macros
    if (strcmp(macroname, "HOSTNAME") == 0) {
        return hst->name;
    }
    if (strcmp(macroname, "HOSTDISPLAYNAME") == 0) {
        return hst->display_name;
    }
    if (strcmp(macroname, "HOSTALIAS") == 0) {
        return hst->alias;
    }
    if (strcmp(macroname, "HOSTADDRESS") == 0) {
        return hst->address;
    }
    if (strcmp(macroname, "HOSTOUTPUT") == 0) {
        return hst->plugin_output;
    }
    if (strcmp(macroname, "LONGHOSTOUTPUT") == 0) {
        return hst->long_plugin_output;
    }
    if (strcmp(macroname, "HOSTPERFDATA") == 0) {
        return hst->perf_data;
    }
    if (strcmp(macroname, "HOSTCHECKCOMMAND") == 0) {
#ifndef NAGIOS4
        return hst->host_check_command;
#else
        return hst->check_command;
#endif  // NAGIOS4
    }
    if (strncmp(macroname, "_HOST", 5) == 0) {  // custom macro
        return expandCustomVariables(macroname + 5, hst->custom_variables);

        // service macros
    }
    if (svc != nullptr) {
        if (strcmp(macroname, "SERVICEDESC") == 0) {
            return svc->description;
        }
        if (strcmp(macroname, "SERVICEDISPLAYNAME") == 0) {
            return svc->display_name;
        }
        if (strcmp(macroname, "SERVICEOUTPUT") == 0) {
            return svc->plugin_output;
        }
        if (strcmp(macroname, "LONGSERVICEOUTPUT") == 0) {
            return svc->long_plugin_output;
        }
        if (strcmp(macroname, "SERVICEPERFDATA") == 0) {
            return svc->perf_data;
        }
        if (strcmp(macroname, "SERVICECHECKCOMMAND") == 0) {
#ifndef NAGIOS4
            return svc->service_check_command;
#else
            return svc->check_command;
#endif  // NAGIOS4
        }
        if (strncmp(macroname, "_SERVICE", 8) == 0) {  // custom macro
            return expandCustomVariables(macroname + 8, svc->custom_variables);
        }
    }

    // USER macros
    if (strncmp(macroname, "USER", 4) == 0) {
        int n = atoi(macroname + 4);
        if (n > 0 && n <= MAX_USER_MACROS) {
            extern char *macro_user[MAX_USER_MACROS];
            return macro_user[n - 1];
        }
    }

    return nullptr;
}

// static
const char *OffsetStringMacroColumn::expandCustomVariables(
    const char *varname, const customvariablesmember *custvars) {
    RegExp regExp(varname, RegExp::Case::ignore, RegExp::Syntax::literal);
    for (; custvars != nullptr; custvars = custvars->next) {
        if (regExp.match(custvars->variable_name)) {
            return custvars->variable_value;
        }
    }
    return nullptr;
}
