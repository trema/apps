Feature: control multiple openflow switchies using routing_switch

  As a Trema user
  I want to control multiple openflow switches using routing_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, two servers
    When I try trema run "../apps/routing_switch/routing_switch" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0xabc" }

      vhost("host1")
      vhost("host2")

      link "switch1", "host1"
      link "switch1", "host2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical


  Scenario: restart openflow switch
    When I try trema run "../apps/routing_switch/routing_switch" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0xabc" }

      vhost("host1")
      vhost("host2")

      link "switch1", "host1"
      link "switch1", "host2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical

    When I try trema kill "switch1"
      And *** sleep 2 ***
      And I try trema up "switch1"
      And *** sleep 15 ***
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical



  Scenario: Four openflow switches, four servers
    When I try trema run "../apps/routing_switch/routing_switch" with following configuration (backgrounded):
      """
      vswitch("routing_switch1") { datapath_id "0x1" }
      vswitch("routing_switch2") { datapath_id "0x2" }
      vswitch("routing_switch3") { datapath_id "0x3" }
      vswitch("routing_switch4") { datapath_id "0x4" }

      vhost("host1")
      vhost("host2")
      vhost("host3")
      vhost("host4")

      link "routing_switch1", "host1"
      link "routing_switch2", "host2"
      link "routing_switch3", "host3"
      link "routing_switch4", "host4"
      link "routing_switch1", "routing_switch2"
      link "routing_switch1", "routing_switch3"
      link "routing_switch1", "routing_switch4"
      link "routing_switch2", "routing_switch3"
      link "routing_switch2", "routing_switch4"
      link "routing_switch3", "routing_switch4"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host4 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host4 --rx" (log = "rx.host4.log")
    Then the content of "tx.host1.log" and "rx.host4.log" should be identical


  Scenario: Five openflow switches, two servers
    When I try trema run "../apps/routing_switch/routing_switch" with following configuration (backgrounded):
      """
      vswitch("routing_switch1") { datapath_id "0x1" }
      vswitch("routing_switch2") { datapath_id "0x2" }
      vswitch("routing_switch3") { datapath_id "0x3" }
      vswitch("routing_switch4") { datapath_id "0x4" }
      vswitch("routing_switch5") { datapath_id "0x5" }

      vhost("host1")
      vhost("host2")

      link "routing_switch2", "host1"
      link "routing_switch4", "host2"
      link "routing_switch1", "routing_switch2"
      link "routing_switch2", "routing_switch3"
      link "routing_switch3", "routing_switch4"
      link "routing_switch3", "routing_switch5"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical


  Scenario: Three openflow switches, three servers, use the lldp-mac-dst option
    When I try trema run "../apps/routing_switch/routing_switch" with following configuration (backgrounded):
      """
      vswitch("routing_switch1") { datapath_id "0x1" }
      vswitch("routing_switch2") { datapath_id "0x2" }
      vswitch("routing_switch3") { datapath_id "0x3" }

      vhost("host1")
      vhost("host2")
      vhost("host3")

      link "routing_switch1", "host1"
      link "routing_switch2", "host2"
      link "routing_switch3", "host3"
      link "routing_switch1", "routing_switch2"
      link "routing_switch2", "routing_switch3"

      run { path "../apps/topology/topology" }
      run {
        path "../apps/topology/topology_discovery"
        options "--lldp_mac_dst=01:00:00:00:00:ff"
      }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host3 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host3.log")
    Then the content of "tx.host1.log" and "rx.host3.log" should be identical


  Scenario: Three openflow switches, three servers, use the lldp-over-ip option
    When I try trema run "../apps/routing_switch/routing_switch" with following configuration (backgrounded):
      """
      vswitch("routing_switch1") { datapath_id "0x1" }
      vswitch("routing_switch2") { datapath_id "0x2" }
      vswitch("routing_switch3") { datapath_id "0x3" }

      vhost("host1")
      vhost("host2")
      vhost("host3")

      link "routing_switch1", "host1"
      link "routing_switch2", "host2"
      link "routing_switch3", "host3"
      link "routing_switch1", "routing_switch2"
      link "routing_switch2", "routing_switch3"

      run {
        path "../apps/topology/topology"
        options "--lldp_over_ip", "--lldp_ip_src=10.0.0.1", "--lldp_ip_dst=10.0.0.2"
      }
      run {
        path "../apps/topology/topology_discovery"
        options "--lldp_over_ip", "--lldp_ip_src=10.0.0.1", "--lldp_ip_dst=10.0.0.2"
      }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "routing_switch"
      """
      And wait until "routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host3 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host3.log")
    Then the content of "tx.host1.log" and "rx.host3.log" should be identical


  Scenario: routing_switch --help
    When I try to run "../apps/routing_switch/routing_switch --help"
    Then the output should be:
      """
      Switching HUB.
      Usage: routing_switch [OPTION]...

        -i, --idle_timeout=TIMEOUT       Idle timeout value of flow entry
        -A, --handle_arp_with_packetout  Handle ARP with packetout
        -n, --name=SERVICE_NAME     service name
        -t, --topology=SERVICE_NAME topology service name
        -d, --daemonize             run in the background
        -l, --logging_level=LEVEL   set logging level
        -h, --help                  display this help and exit
      """


  Scenario: routing_switch -h
    When I try to run "../apps/routing_switch/routing_switch -h"
    Then the output should be:
      """
      Switching HUB.
      Usage: routing_switch [OPTION]...

        -i, --idle_timeout=TIMEOUT       Idle timeout value of flow entry
        -A, --handle_arp_with_packetout  Handle ARP with packetout
        -n, --name=SERVICE_NAME     service name
        -t, --topology=SERVICE_NAME topology service name
        -d, --daemonize             run in the background
        -l, --logging_level=LEVEL   set logging level
        -h, --help                  display this help and exit
      """
