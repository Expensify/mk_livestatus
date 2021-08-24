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

#ifndef DowntimeOrComment_h
#define DowntimeOrComment_h

#include "config.h"  // IWYU pragma: keep
#include <ctime>
#include <string>
#include "nagios.h"

/* The structs for downtime and comment are so similar, that
   we handle them with the same logic */

/*
   typedef struct nebstruct_downtime_struct{
   int             type;
   int             flags;
   int             attr;
   struct timeval  timestamp;

   int             downtime_type;
   char            *host_name;
   char            *service_description;
   time_t          entry_time;
   char            *author_name;
   char            *comment_data;
   time_t          start_time;
   time_t          end_time;
   int             fixed;
   unsigned long   duration;
   unsigned long   triggered_by;
   unsigned long   downtime_id;

   void            *object_ptr; // not implemented yet
   }nebstruct_downtime_data;

   typedef struct nebstruct_comment_struct{
   int             type;
   int             flags;
   int             attr;
   struct timeval  timestamp;

   int             comment_type;
   char            *host_name;
   char            *service_description;
   time_t          entry_time;
   char            *author_name;
   char            *comment_data;
   int             persistent;
   int             source;
   int             entry_type;
   int             expires;
   time_t          expire_time;
   unsigned long   comment_id;

   void            *object_ptr; // not implemented yet
   }nebstruct_comment_data;
 */

class DowntimeOrComment {
public:
    int _type;
    bool _is_service;
    host *_host;
    service *_service;
    time_t _entry_time;
    std::string _author_name;
    std::string _comment;
    unsigned long _id;

    DowntimeOrComment(nebstruct_downtime_struct *dt, unsigned long id);
    virtual ~DowntimeOrComment();
};

class Downtime : public DowntimeOrComment {
public:
    time_t _start_time;
    time_t _end_time;
    int _fixed;
    // TODO(sp): Wrong types, caused by TableDowntimes accessing it via
    // OffsetIntColumn, should be unsigned long
    int _duration;
    int _triggered_by;
    explicit Downtime(nebstruct_downtime_struct *dt);
};

class Comment : public DowntimeOrComment {
public:
    time_t _expire_time;
    int _persistent;
    int _source;
    int _entry_type;
    int _expires;
    explicit Comment(nebstruct_comment_struct *co);
};

#endif  // DowntimeOrComment_h
