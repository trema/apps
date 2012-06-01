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

create_mac_binding_tables() {
    ../../slice create mac_binding "Mac binding"
    ../../slice add-mac mac_binding 00:00:00:00:00:01 host1
    ../../slice add-mac mac_binding 00:00:00:00:00:02 host2
}

create_port_binding_tables() {
    ../../slice create port_binding "Port binding"
    ../../slice add-port port_binding 0xabc 3 0xffff host3
    ../../slice add-port port_binding 0xabc 4 0xffff host4
}

create_port_mac_binding_tables() {
    ../../slice create port_mac_binding "Port and mac binding"
    ../../slice add-mac port_mac_binding 00:00:00:00:00:05 host5
    ../../slice add-port port_mac_binding 0xabc 5 0xffff host6
}

show_slice() {
    slices=`../../slice list | awk 'NR > 1 { print $1 }' | sort`
    for slice in $slices
    do
      echo show $slice
      ../../slice show $slice
    done
}

tests() {
    create_tables
    echo slice help
    ../../slice

    show_slice

    echo create mac binding tables
    create_tables
    create_mac_binding_tables
    show_slice

    echo create port binding tables
    create_tables
    create_port_binding_tables
    show_slice

    echo create port and mac binding tables
    create_tables
    create_port_mac_binding_tables
    show_slice

    echo three slice
    create_tables
    create_mac_binding_tables
    create_port_binding_tables
    create_port_mac_binding_tables
    show_slice
}

tests
