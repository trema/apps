Feature: sliceable_routing_switch help

  As a Trema user
  I want to see the help message of sliceable_routing_switch command
  So that I can learn how to use sliceable_routing_switch


  Scenario: sliceable_routing_switch --help
    When I try to run "../apps/sliceable_routing_switch/sliceable_routing_switch --help"
    Then the output should be:
      """
      Sliceable routing switch.
      Usage: sliceable_routing_switch [OPTION]...
      
        -i, --idle_timeout=TIMEOUT  idle timeout value of flow entry
        -s, --slice_db=DB_FILE      slice database
        -f, --filter_db=DB_FILE     filter database
        -m, --loose                 enable loose mac-based slicing
        -r, --restrict_hosts        restrict hosts on switch port
        -n, --name=SERVICE_NAME     service name
        -t, --topology=SERVICE_NAME topology service name
        -d, --daemonize             run in the background
        -l, --logging_level=LEVEL   set logging level
        -h, --help                  display this help and exit
      """

  Scenario: sliceable_routing_switch -h
    When I try to run "../apps/sliceable_routing_switch/sliceable_routing_switch -h"
    Then the output should be:
      """
      Sliceable routing switch.
      Usage: sliceable_routing_switch [OPTION]...
      
        -i, --idle_timeout=TIMEOUT  idle timeout value of flow entry
        -s, --slice_db=DB_FILE      slice database
        -f, --filter_db=DB_FILE     filter database
        -m, --loose                 enable loose mac-based slicing
        -r, --restrict_hosts        restrict hosts on switch port
        -n, --name=SERVICE_NAME     service name
        -t, --topology=SERVICE_NAME topology service name
        -d, --daemonize             run in the background
        -l, --logging_level=LEVEL   set logging level
        -h, --help                  display this help and exit
      """
