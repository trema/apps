Feature: topology help

  As a Trema user
  I want to control multiple openflow switches using topology application
  So that I can show topology


  Scenario: Four openflow switches, four servers
    When I try trema run "../apps/topology/topology_discovery" with following configuration (backgrounded):
      """
      vswitch("routing_switch1") { datapath_id "0x1" }
      vswitch("routing_switch2") { datapath_id "0x2" }
      vswitch("routing_switch3") { datapath_id "0x3" }
      vswitch("routing_switch4") { datapath_id "0x4" }

      link "routing_switch1", "routing_switch2"
      link "routing_switch1", "routing_switch3"
      link "routing_switch1", "routing_switch4"
      link "routing_switch2", "routing_switch3"
      link "routing_switch2", "routing_switch4"
      link "routing_switch3", "routing_switch4"

      app { path "../apps/topology/topology" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "topology_discovery" is up
      And *** sleep 15 ***
      And I try to run "TREMA_HOME=`pwd` ../apps/topology/show_topology -C"
    Then the output should be:
      """
      f-dpid,f-port,t-dpid,t-port,stat
      0x1,1,0x3,1,up
      0x1,2,0x4,1,up
      0x1,3,0x2,2,up
      0x1,65534,-,-,down
      0x2,1,0x4,3,up
      0x2,2,0x1,3,up
      0x2,3,0x3,3,up
      0x2,65534,-,-,down
      0x3,1,0x1,1,up
      0x3,2,0x4,2,up
      0x3,3,0x2,3,up
      0x3,65534,-,-,down
      0x4,1,0x1,2,up
      0x4,2,0x3,2,up
      0x4,3,0x2,1,up
      0x4,65534,-,-,down
      """


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
