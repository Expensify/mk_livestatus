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

#include "LogCache.h"
#include <sstream>
#include <string>
#include <utility>
#include "FileSystem.h"
#include "LogEntry.h"  // IWYU pragma: keep
#include "Logfile.h"
#include "Logger.h"
#include "MonitoringCore.h"

namespace {
// Check memory every N'th new message
constexpr unsigned long check_mem_cycle = 1000;
}  // namespace

int num_cached_log_messages = 0;

LogCache::LogCache(MonitoringCore *mc, unsigned long max_cached_messages)
    : _mc(mc)
    , _max_cached_messages(max_cached_messages)
    , _num_at_last_check(0) {
    update();
}

#ifdef CMC
void LogCache::setMaxCachedMessages(unsigned long m) {
    if (m != _max_cached_messages) {
        Notice(logger())
            << "changing maximum number of messages for log file cache to "
            << m;
        _max_cached_messages = m;
    }
}
#endif

void LogCache::update() {
    if (!_logfiles.empty() &&
        _mc->last_logfile_rotation() <= _last_index_update) {
        return;
    }

    Informational(logger()) << "updating log file index";

    _logfiles.clear();
    num_cached_log_messages = 0;

    _last_index_update = std::chrono::system_clock::now();
    // We need to find all relevant logfiles. This includes directory, the
    // current nagios.log and all files in the archive.
    addToIndex(
        std::make_unique<Logfile>(_mc, this, _mc->historyFilePath(), true));

    fs::path dirpath = _mc->logArchivePath();
    try {
        for (const auto &entry : fs::directory_iterator(dirpath)) {
            addToIndex(
                std::make_unique<Logfile>(_mc, this, entry.path(), false));
        }
    } catch (const fs::filesystem_error &e) {
        Warning(logger()) << "updating log file index: " << e.what();
    }

    if (_logfiles.empty()) {
        Notice(logger()) << "no log file found, not even "
                         << _mc->historyFilePath();
    }
}

void LogCache::addToIndex(std::unique_ptr<Logfile> logfile) {
    time_t since = logfile->since();
    if (since == 0) {
        return;
    }
    // make sure that no entry with that 'since' is existing yet.  Under normal
    // circumstances this never happens, but the user might have copied files
    // around.
    if (_logfiles.find(since) != _logfiles.end()) {
        Warning(logger()) << "ignoring duplicate log file " << logfile->path();
        return;
    }

    _logfiles.emplace(since, std::move(logfile));
}

/* This method is called each time a log message is loaded
   into memory. If the number of messages loaded in memory
   is to large, memory will be freed by flushing logfiles
   and message not needed by the current query.

   The parameters to this method reflect the current query,
   not the messages that just has been loaded.
 */
void LogCache::logLineHasBeenAdded(Logfile *logfile, unsigned logclasses) {
    if (static_cast<unsigned long>(++num_cached_log_messages) <=
        _max_cached_messages) {
        return;  // current message count still allowed, everything ok
    }

    /* Memory checking an freeing consumes CPU ressources. We save
       ressources, by avoiding to make the memory check each time
       a new message is loaded when being in a sitation where no
       memory can be freed. We do this by suppressing the check when
       the number of messages loaded into memory has not grown
       by at least check_mem_cycle messages */
    if (static_cast<unsigned long>(num_cached_log_messages) <
        _num_at_last_check + check_mem_cycle) {
        return;  // Do not check this time
    }

    // [1] Begin by deleting old logfiles
    // Begin deleting with the oldest logfile available
    logfiles_t::iterator it;
    for (it = _logfiles.begin(); it != _logfiles.end(); ++it) {
        if (it->second.get() == logfile) {
            // Do not touch the logfile the Query is currently accessing
            break;
        }
        if (it->second->size() > 0) {
            num_cached_log_messages -= it->second->size();
            it->second->flush();  // drop all messages of that file
            if (static_cast<unsigned long>(num_cached_log_messages) <=
                _max_cached_messages) {
                // remember the number of log messages in cache when
                // the last memory-release was done. No further
                // release-check shall be done until that number changes.
                _num_at_last_check = num_cached_log_messages;
                return;
            }
        }
    }
    // The end of this loop must be reached by 'break'. At least one logfile
    // must be the current logfile. So now 'it' points to the current logfile.
    // We save that pointer for later.
    auto queryit = it;

    // [2] Delete message classes irrelevent to current query
    // Starting from the current logfile (wo broke out of the
    // previous loop just when 'it' pointed to that)
    for (; it != _logfiles.end(); ++it) {
        if (it->second->size() > 0 &&
            (it->second->classesRead() & ~logclasses) != 0) {
            Debug(logger()) << "freeing classes " << ~logclasses << " of file "
                            << it->second->path();
            // flush only messages not needed for current query
            long freed = it->second->freeMessages(~logclasses);
            num_cached_log_messages -= freed;
            if (static_cast<unsigned long>(num_cached_log_messages) <=
                _max_cached_messages) {
                _num_at_last_check = num_cached_log_messages;
                return;
            }
        }
    }

    // [3] Flush newest logfiles
    // If there are still too many messages loaded, continue
    // flushing logfiles from the oldest to the newest starting
    // at the file just after (i.e. newer than) the current logfile
    for (it = ++queryit; it != _logfiles.end(); ++it) {
        if (it->second->size() > 0) {
            Debug(logger()) << "flush newer log, " << it->second->size()
                            << " number of entries";
            num_cached_log_messages -= it->second->size();
            it->second->flush();
            if (static_cast<unsigned long>(num_cached_log_messages) <=
                _max_cached_messages) {
                _num_at_last_check = num_cached_log_messages;
                return;
            }
        }
    }
    _num_at_last_check = num_cached_log_messages;
    // If we reach this point, no more logfiles can be unloaded,
    // despite the fact that there are still too many messages
    // loaded.

    Debug(logger()) << "cannot unload more messages, still "
                    << num_cached_log_messages << " loaded (max is "
                    << _max_cached_messages << ")";
}

Logger *LogCache::logger() const { return _mc->loggerLivestatus(); }
