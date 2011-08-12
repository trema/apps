Feature: topology help

  As a Trema user
  I want to see the help message of topology command
  So that I can learn how to use topology


  Scenario: topology --help
    When I try to run "../apps/topology/topology --help"
    Then the output should be:
      """
      topology manager
      Usage: topology [OPTION]...

        -i, --lldp_over_ip          Send LLDP messages over IP
        -o, --lldp_ip_src=IP_ADDR   Source IP address for sending LLDP over IP
        -r, --lldp_ip_dst=IP_ADDR   Destination IP address for sending LLDP over IP
        -n, --name=SERVICE_NAME     service name
        -d, --daemonize             run in the background
        -l, --logging_level=LEVEL   set logging level
        -h, --help                  display this help and exit
      """


  Scenario: topology -h
    When I try to run "../apps/topology/topology -h"
    Then the output should be:
      """
      topology manager
      Usage: topology [OPTION]...

        -i, --lldp_over_ip          Send LLDP messages over IP
        -o, --lldp_ip_src=IP_ADDR   Source IP address for sending LLDP over IP
        -r, --lldp_ip_dst=IP_ADDR   Destination IP address for sending LLDP over IP
        -n, --name=SERVICE_NAME     service name
        -d, --daemonize             run in the background
        -l, --logging_level=LEVEL   set logging level
        -h, --help                  display this help and exit
      """
