#! /bin/sh -
#
# Copyright (C) 2011 NEC Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
PATH=/usr/bin:/bin

if [ "x$FILTER_DB_FILE" = "x" ]; then
    FILTER_DB_FILE=./filter.db
fi
if [ "x$SLICE_DB_FILE" = "x" ]; then
    SLICE_DB_FILE=./slice.db
fi

if [ ! -e "$SLICE_DB_FILE" ]; then
    sqlite3 "$SLICE_DB_FILE" < ./create_slice_table.sql
fi
if [ ! -e "$FILTER_DB_FILE" ]; then
    sqlite3 "$FILTER_DB_FILE" < ./create_filter_table.sql
    ./filter add default priority=0 action=ALLOW
fi
