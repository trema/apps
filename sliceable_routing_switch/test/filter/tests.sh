#! /bin/sh
#
PATH=/usr/bin:/bin

CWD=`pwd`
export FILTER_DB_FILE=$CWD/filter.db
export SLICE_DB_FILE=$CWD/slice.db
export PERL5LIB=$CWD/../..

delete_tables() {
    if [ -f "$FILTER_DB_FILE" ]; then
      rm "$FILTER_DB_FILE"
    fi
    if [ -f "$SLICE_DB_FILE" ]; then
      rm "$SLICE_DB_FILE"
    fi
}

create_tables() {
    delete_tables
    ( cd ../.. && ./create_tables.sh )
}

show_filter() {
    filters=`../../filter list | awk 'NR > 1 { print $1 }' | sort`
    for filter in $filters
    do
      echo show $filter
      ../../filter show $filter
    done
}

set_default_deny() {
    ../../filter delete default
    ../../filter add default priority=0 action=DENY
}

add_dl_src_filter() {
    ../../filter add host1_mac priority=65535 dl_src=00:00:00:00:00:01 action=ALLOW
    ../../filter add host2_mac priority=65535 dl_src=00:00:00:00:00:02 action=ALLOW
    ../../filter add host3_mac priority=65535 dl_src=00:00:00:00:00:03 action=ALLOW
    ../../filter add host4_mac priority=65535 dl_src=00:00:00:00:00:04 action=ALLOW
    ../../filter add host5_mac priority=65535 dl_src=00:00:00:00:00:05 action=ALLOW
}

add_nw_src_filter() {
    ../../filter add host1_ip priority=0x8000 nw_src=192.168.0.1 action=ALLOW
    ../../filter add host2_ip priority=0x8000 nw_src=192.168.0.2 action=ALLOW
    ../../filter add host3_ip priority=0x8000 nw_src=192.168.0.3 action=ALLOW
    ../../filter add host4_ip priority=0x8000 nw_src=192.168.0.4 action=ALLOW
    ../../filter add host5_ip priority=0x8000 nw_src=192.168.0.5 action=ALLOW
    ../../filter add network192_168 priority=0x7000 nw_src=192.168.0.0/16 action=DENY
    ../../filter add network10      priority=0x9000 nw_src=10.0.0.0/8 action=ALLOW
    ../../filter add network172_16  priority=0x7000 nw_src=172.16.0.0/12 action=ALLOW
    ../../filter add network169_254 priority=0x7000 nw_src=169.254.0.0/16 action=ALLOW
    ../../filter add network127     priority=0xf000 nw_src=127.0.0.0/8 action=DENY
}

tests() {
    create_tables
    echo filter help
    ../../filter

    show_filter

    echo dl_src filter
    create_tables
    set_default_deny
    add_dl_src_filter
    show_filter

    echo nw_src filter
    create_tables
    set_default_deny
    add_nw_src_filter
    show_filter
}

tests
