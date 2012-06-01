Feature: sliceable_switch help

  As a Trema user
  I want to see the help message of sliceable_switch command
  So that I can learn how to use sliceable_switch


  Scenario: sliceable_switch --help
    When I try to run "../apps/sliceable_switch/sliceable_switch --help"
    Then the output should be:
      """
      Sliceable switch.
      Usage: sliceable_switch [OPTION]...
      
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

  Scenario: sliceable_switch -h
    When I try to run "../apps/sliceable_switch/sliceable_switch -h"
    Then the output should be:
      """
      Sliceable switch.
      Usage: sliceable_switch [OPTION]...
      
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
