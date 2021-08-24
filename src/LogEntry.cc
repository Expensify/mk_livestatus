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

#include "LogEntry.h"
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include "MonitoringCore.h"

// 0123456789012345678901234567890
// [1234567890] FOO BAR: blah blah
static constexpr size_t timestamp_prefix_length = 13;

// TODO(sp) Fix classifyLogMessage() below to always set all fields and remove
// this set-me-to-zero-to-be-sure-block.
LogEntry::LogEntry(MonitoringCore *mc, size_t lineno, std::string line)
    : _lineno(static_cast<int32_t>(lineno))
    , _complete(std::move(line))
    , _state(0)
    , _attempt(0)
    , _host(nullptr)
    , _service(nullptr)
    , _contact(nullptr) {
    // pointer to options (everything after ':')
    size_t pos = _complete.find(':');
    if (pos != std::string::npos) {
        pos = _complete.find_first_not_of(' ', pos + 1);
    }
    if (pos == std::string::npos) {
        pos = _complete.size();
    }
    _options = &_complete[pos];

    try {
        if (_complete.size() < timestamp_prefix_length || _complete[0] != '[' ||
            _complete[11] != ']' || _complete[12] != ' ') {
            throw std::invalid_argument("timestamp delimiter");
        }
        _time = std::stoi(_complete.substr(1, 10));
    } catch (const std::logic_error &e) {
        _logclass = Class::invalid;
        _type = LogEntryType::none;
        return;  // ignore invalid lines silently
    }

    classifyLogMessage();
    applyWorkarounds();
    updateReferences(mc);
}

bool LogEntry::assign(Param par, const std::string &field) {
    switch (par) {
        case Param::HostName:
            this->_host_name = field;
            break;
        case Param::SvcDesc:
            this->_svc_desc = field;
            break;
        case Param::HostState:
            this->_state = static_cast<int>(parseHostState(field));
            break;
        case Param::ServiceState:
            this->_state = static_cast<int>(parseServiceState(field));
            break;
        case Param::State:
            this->_state = atoi(field.c_str());
            break;
        case Param::StateType:
            this->_state_type = field;
            break;
        case Param::Attempt:
            this->_attempt = atoi(field.c_str());
            break;
        case Param::Comment:
            this->_comment = field;
            break;
        case Param::CommandName:
            this->_command_name = field;
            break;
        case Param::ContactName:
            this->_contact_name = field;
            break;
        case Param::CheckOutput:
            this->_check_output = field;
            break;
    }

    return true;
};

std::vector<LogEntry::LogDef> LogEntry::log_definitions{
    LogDef{"INITIAL HOST STATE",
           Class::state,
           LogEntryType::state_host_initial,
           {Param::HostName, Param::HostState, Param::StateType, Param::Attempt,
            Param::CheckOutput}},
    ////////////////
    LogDef{"CURRENT HOST STATE",
           Class::state,
           LogEntryType::state_host,
           {Param::HostName, Param::HostState, Param::StateType, Param::Attempt,
            Param::CheckOutput}},
    ////////////////
    LogDef{"HOST ALERT",
           Class::alert,
           LogEntryType::alert_host,
           {Param::HostName, Param::HostState, Param::StateType, Param::Attempt,
            Param::CheckOutput}},
    ////////////////
    LogDef{"HOST DOWNTIME ALERT",
           Class::alert,
           LogEntryType::downtime_alert_host,
           {Param::HostName, Param::StateType, Param::Comment}},
    ////////////////
    LogDef{"HOST ACKNOWLEDGE ALERT",
           Class::alert,
           LogEntryType::acknowledge_alert_host,
           {Param::HostName, Param::StateType, Param::ContactName,
            Param::Comment}},
    ////////////////
    LogDef{"HOST FLAPPING ALERT",
           Class::alert,
           LogEntryType::flapping_host,
           {Param::HostName, Param::StateType, Param::Comment}},
    ////////////////
    LogDef{"INITIAL SERVICE STATE",
           Class::state,
           LogEntryType::state_service_initial,
           {Param::HostName, Param::SvcDesc, Param::ServiceState,
            Param::StateType, Param::Attempt, Param::CheckOutput}},
    ////////////////
    LogDef{"CURRENT SERVICE STATE",
           Class::state,
           LogEntryType::state_service,
           {Param::HostName, Param::SvcDesc, Param::ServiceState,
            Param::StateType, Param::Attempt, Param::CheckOutput}},
    ////////////////
    LogDef{"SERVICE ALERT",
           Class::alert,
           LogEntryType::alert_service,
           {Param::HostName, Param::SvcDesc, Param::ServiceState,
            Param::StateType, Param::Attempt, Param::CheckOutput}},
    ////////////////
    LogDef{"SERVICE DOWNTIME ALERT",
           Class::alert,
           LogEntryType::downtime_alert_service,
           {Param::HostName, Param::SvcDesc, Param::StateType, Param::Comment}},
    ////////////////
    LogDef{"SERVICE ACKNOWLEDGE ALERT",
           Class::alert,
           LogEntryType::acknowledge_alert_service,
           {Param::HostName, Param::SvcDesc, Param::StateType,
            Param::ContactName, Param::Comment}},
    ////////////////
    LogDef{"SERVICE FLAPPING ALERT",
           Class::alert,
           LogEntryType::flapping_service,
           {Param::HostName, Param::SvcDesc, Param::StateType, Param::Comment}},
    ////////////////
    LogDef{"TIMEPERIOD TRANSITION",
           Class::state,
           LogEntryType::timeperiod_transition,
           {}},
    ////////////////
    LogDef{"HOST NOTIFICATION",
           Class::hs_notification,
           LogEntryType::none,
           {Param::ContactName, Param::HostName, Param::StateType,
            Param::CommandName, Param::CheckOutput}},
    ////////////////
    LogDef{"SERVICE NOTIFICATION",
           Class::hs_notification,
           LogEntryType::none,
           {Param::ContactName, Param::HostName, Param::SvcDesc,
            Param::StateType, Param::CommandName, Param::CheckOutput}},
    ////////////////
    LogDef{"HOST NOTIFICATION RESULT",
           Class::hs_notification,
           LogEntryType::none,
           {Param::ContactName, Param::HostName, Param::StateType,
            Param::CommandName, Param::CheckOutput, Param::Comment}},
    ////////////////
    LogDef{
        "SERVICE NOTIFICATION RESULT",
        Class::hs_notification,
        LogEntryType::none,
        {Param::ContactName, Param::HostName, Param::SvcDesc, Param::StateType,
         Param::CommandName, Param::CheckOutput, Param::Comment}},
    ////////////////
    LogDef{"HOST NOTIFICATION PROGRESS",
           Class::hs_notification,
           LogEntryType::none,
           {Param::ContactName, Param::HostName, Param::StateType,
            Param::CommandName, Param::CheckOutput}},
    ////////////////
    LogDef{"SERVICE NOTIFICATION PROGRESS",
           Class::hs_notification,
           LogEntryType::none,
           {Param::ContactName, Param::HostName, Param::SvcDesc,
            Param::StateType, Param::CommandName, Param::CheckOutput}},
    ////////////////
    LogDef{"HOST ALERT HANDLER STARTED",
           Class::alert_handlers,
           LogEntryType::none,
           {Param::HostName, Param::CommandName}},
    ////////////////
    LogDef{"SERVICE ALERT HANDLER STARTED",
           Class::alert_handlers,
           LogEntryType::none,
           {Param::HostName, Param::SvcDesc, Param::CommandName}},
    ////////////////
    LogDef{"HOST ALERT HANDLER STOPPED",
           Class::alert_handlers,
           LogEntryType::none,
           {Param::HostName, Param::CommandName, Param::ServiceState,
            Param::CheckOutput}},
    ////////////////
    LogDef{"SERVICE ALERT HANDLER STOPPED",
           Class::alert_handlers,
           LogEntryType::none,
           {Param::HostName, Param::SvcDesc, Param::CommandName,
            Param::ServiceState, Param::CheckOutput}},
    ////////////////
    LogDef{"PASSIVE SERVICE CHECK",
           Class::passivecheck,
           LogEntryType::none,
           {Param::HostName, Param::SvcDesc, Param::State, Param::CheckOutput}},
    ////////////////
    LogDef{"PASSIVE HOST CHECK",
           Class::passivecheck,
           LogEntryType::none,
           {Param::HostName, Param::State, Param::CheckOutput}},
    ////////////////
    LogDef{"EXTERNAL COMMAND", Class::ext_command, LogEntryType::none, {}}};

// A bit verbose, but we avoid unnecessary string copies below.
void LogEntry::classifyLogMessage() {
    for (const auto &def : log_definitions) {
        if (textStartsWith(def.prefix) &&
            _complete.compare(timestamp_prefix_length + def.prefix.size(), 2,
                              ": ") == 0) {
            _text = &def.prefix[0];
            _logclass = def.log_class;
            _type = def.log_type;
            // TODO(sp) Use boost::tokenizer instead of this index fiddling
            size_t pos = timestamp_prefix_length + def.prefix.size() + 2;
            for (Param par : def.params) {
                size_t sep_pos = _complete.find(';', pos);
                size_t end_pos =
                    sep_pos == std::string::npos ? _complete.size() : sep_pos;
                assign(par, _complete.substr(pos, end_pos - pos));
                pos = sep_pos == std::string::npos ? _complete.size()
                                                   : (sep_pos + 1);
            }
            return;
        }
    }
    _text = &_complete[timestamp_prefix_length];
    if (textStartsWith("LOG VERSION: 2.0")) {
        _logclass = Class::program;
        _type = LogEntryType::log_version;
        return;
    }
    if (textStartsWith("logging initial states") ||
        textStartsWith("logging intitial states")) {
        _logclass = Class::program;
        _type = LogEntryType::log_initial_states;
        return;
    }
    if (textContains("starting...") || textContains("active mode...")) {
        _logclass = Class::program;
        _type = LogEntryType::core_starting;
        return;
    }
    if (textContains("shutting down...") || textContains("Bailing out") ||
        textContains("standby mode...")) {
        _logclass = Class::program;
        _type = LogEntryType::core_stopping;
        return;
    }
    if (textContains("restarting...")) {
        _logclass = Class::program;
        _type = LogEntryType::none;
        return;
    }
    _logclass = Class::info;
    _type = LogEntryType::none;
}

bool LogEntry::textStartsWith(const std::string &what) {
    return _complete.compare(timestamp_prefix_length, what.size(), what) == 0;
}

bool LogEntry::textContains(const std::string &what) {
    return _complete.find(what, timestamp_prefix_length) != std::string::npos;
}

// The NotifyHelper class has a long, tragic history: Through a long series of
// commits, it suffered from spelling mistakes like "HOST_NOTIFICATION" or "HOST
// NOTIFICATION" (without a colon), parameter lists not matching the
// corresponding format strings, and last but not least wrong ordering of
// fields. The net result of this tragedy is that due to legacy reasons, we have
// to support parsing an incorrect ordering of "state type" and "command name"
// fields. :-P
void LogEntry::applyWorkarounds() {
    if (_logclass != Class::hs_notification ||  // no need for any workaround
        _state_type.empty()) {                  // extremely broken line
        return;
    }

    if (_state_type == "check-mk-notify") {
        // Ooops, we encounter one of our own buggy lines...
        std::swap(_state_type, _command_name);
    }

    if (_state_type.empty()) {
        return;  // extremely broken line, even after a potential swap
    }

    _state = _svc_desc.empty()
                 ? static_cast<int>(parseHostState(_state_type))
                 : static_cast<int>(parseServiceState(_state_type));
}

namespace {
// Ugly: Depending on where we're called, the actual state type can be in
// parentheses at the end, e.g. "ALERTHANDLER (OK)".
std::string extractStateType(const std::string &str) {
    if (!str.empty() && str[str.size() - 1] == ')') {
        size_t lparen = str.rfind('(');
        if (lparen != std::string::npos) {
            return str.substr(lparen + 1, str.size() - lparen - 2);
        }
    }
    return str;
}

std::unordered_map<std::string, ServiceState> serviceStateTypes{
    // normal states
    {"OK", ServiceState::ok},
    {"WARNING", ServiceState::warning},
    {"CRITICAL", ServiceState::critical},
    {"UNKNOWN", ServiceState::unknown},
    // states from "... ALERT"/"... NOTIFICATION"
    {"RECOVERY", ServiceState::ok}};

std::unordered_map<std::string, HostState> hostStateTypes{
    // normal states
    {"UP", HostState::up},
    {"DOWN", HostState::down},
    {"UNREACHABLE", HostState::unreachable},
    // states from "... ALERT"/"... NOTIFICATION"
    {"RECOVERY", HostState::up},
    // states from "... ALERT HANDLER STOPPED" and "(HOST|SERVICE) NOTIFICATION
    // (RESULT|PROGRESS)"
    {"OK", HostState::up},
    {"WARNING", HostState::down},
    {"CRITICAL", HostState::unreachable},
    {"UNKNOWN", HostState::up}};
}  // namespace

ServiceState LogEntry::parseServiceState(const std::string &str) {
    auto it = serviceStateTypes.find(extractStateType(str));
    return it == serviceStateTypes.end() ? ServiceState::ok : it->second;
}

HostState LogEntry::parseHostState(const std::string &str) {
    auto it = hostStateTypes.find(extractStateType(str));
    return it == hostStateTypes.end() ? HostState::up : it->second;
}

unsigned LogEntry::updateReferences(MonitoringCore *mc) {
    unsigned updated = 0;
    if (!_host_name.empty()) {
        // Older Nagios headers are not const-correct... :-P
        _host = find_host(const_cast<char *>(_host_name.c_str()));
        updated++;
    }
    if (!_svc_desc.empty()) {
        // Older Nagios headers are not const-correct... :-P
        _service = find_service(const_cast<char *>(_host_name.c_str()),
                                const_cast<char *>(_svc_desc.c_str()));
        updated++;
    }
    if (!_contact_name.empty()) {
        // Older Nagios headers are not const-correct... :-P
        _contact = find_contact(const_cast<char *>(_contact_name.c_str()));
        updated++;
    }
    if (!_command_name.empty()) {
        _command = mc->find_command(_command_name);
        updated++;
    }
    return updated;
}
