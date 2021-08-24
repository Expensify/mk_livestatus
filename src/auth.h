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

#ifndef auth_h
#define auth_h

#include "config.h"  // IWYU pragma: keep

#ifdef CMC
#include "contact_fwd.h"
#else
#include "nagios.h"
#endif

enum class AuthorizationKind { loose = 0, strict = 1 };

#ifdef CMC
inline contact *unknown_auth_user() {
    return reinterpret_cast<contact *>(0xdeadbeaf);
}
#else
contact *unknown_auth_user();
class MonitoringCore;
bool is_authorized_for(MonitoringCore *mc, const contact *ctc, const host *hst,
                       const service *svc);
bool is_authorized_for_host_group(MonitoringCore *mc, const hostgroup *hg,
                                  const contact *ctc);
bool is_authorized_for_service_group(MonitoringCore *mc, const servicegroup *sg,
                                     const contact *ctc);
#endif

#endif  // auth_h
