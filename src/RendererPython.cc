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

#include "RendererPython.h"
#include <ostream>
class Logger;

RendererPython::RendererPython(std::ostream &os, Logger *logger,
                               Encoding data_encoding)
    : Renderer(os, logger, data_encoding) {}

// --------------------------------------------------------------------------

void RendererPython::beginQuery() { _os << "["; }
void RendererPython::separateQueryElements() { _os << ",\n"; }
void RendererPython::endQuery() { _os << "]\n"; }

// --------------------------------------------------------------------------

void RendererPython::beginRow() { _os << "["; }
void RendererPython::beginRowElement() {}
void RendererPython::endRowElement() {}
void RendererPython::separateRowElements() { _os << ","; }
void RendererPython::endRow() { _os << "]"; }

// --------------------------------------------------------------------------

void RendererPython::beginList() { _os << "["; }
void RendererPython::separateListElements() { _os << ","; }
void RendererPython::endList() { _os << "]"; }

// --------------------------------------------------------------------------

void RendererPython::beginSublist() { beginList(); }
void RendererPython::separateSublistElements() { separateListElements(); }
void RendererPython::endSublist() { endList(); }

// --------------------------------------------------------------------------

void RendererPython::beginDict() { _os << "{"; }
void RendererPython::separateDictElements() { _os << ","; }
void RendererPython::separateDictKeyValue() { _os << ":"; }
void RendererPython::endDict() { _os << "}"; }

// --------------------------------------------------------------------------

void RendererPython::outputNull() { _os << "None"; }

void RendererPython::outputBlob(const std::vector<char> &value) {
    outputByteString("", value);
}

void RendererPython::outputString(const std::string &value) {
    // cppcheck is too dumb to see that we just take the address... :-/
    // cppcheck-suppress containerOutOfBoundsIndexExpression
    outputUnicodeString("u", &value[0], &value[value.size()], _data_encoding);
}
