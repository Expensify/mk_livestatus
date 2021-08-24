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

#ifndef LogCache_h
#define LogCache_h

#include "config.h"  // IWYU pragma: keep
#include <chrono>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
class Logfile;
class Logger;
class MonitoringCore;

using logfiles_t = std::map<time_t, std::unique_ptr<Logfile>>;

class LogCache {
public:
    std::mutex _lock;

    LogCache(MonitoringCore *mc, unsigned long max_cached_messages);
#ifdef CMC
    void setMaxCachedMessages(unsigned long m);
#endif
    void logLineHasBeenAdded(Logfile *logfile, unsigned logclasses);
    void update();
    auto begin() { return _logfiles.begin(); }
    auto end() { return _logfiles.end(); }

private:
    MonitoringCore *const _mc;
    unsigned long _max_cached_messages;
    unsigned long _num_at_last_check;
    logfiles_t _logfiles;
    std::chrono::system_clock::time_point _last_index_update;

    void addToIndex(std::unique_ptr<Logfile> logfile);
    [[nodiscard]] Logger *logger() const;
};

#endif  // LogCache_h
