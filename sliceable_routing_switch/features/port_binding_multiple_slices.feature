Feature: control multiple openflow switchies using sliceable_routing_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_routing_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, two slices, four servers, port binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
      | test2    | slice2      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 3    | 0xffff | host1      |
      | test1    | 0x1   | 1    | 0xffff | host2      |
      | test2    | 0x1   | 2    | 0xffff | host3      |
      | test2    | 0x1   | 4    | 0xffff | host4      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_routing_switch/sliceable_routing_switch -s tmp/slice.db -f tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("sliceable_routing_switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }

      link "sliceable_routing_switch1", "host1"
      link "sliceable_routing_switch1", "host2"
      link "sliceable_routing_switch1", "host3"
      link "sliceable_routing_switch1", "host4"

      app { path "../apps/topology/topology" }
      app { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_routing_switch"
      """
      And wait until "sliceable_routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical
      And I send packets from host3 to host4 (duration = 10)
      And I try to run "./trema show_stats host3 --tx" (log = "tx.host3.log")
      And I try to run "./trema show_stats host4 --rx" (log = "rx.host4.log")
    Then the content of "tx.host3.log" and "rx.host4.log" should be identical
      And I send packets from host4 to host3 (duration = 10)
      And I try to run "./trema show_stats host4 --tx" (log = "tx.host4.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host3.log")
    Then the content of "tx.host4.log" and "rx.host3.log" should be identical


  Scenario: Six openflow switches, three slices, six servers, port binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
      | test2    | slice2      |
      | test3    | slice3      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 3    | 0xffff | host1      |
      | test1    | 0x2   | 1    | 0xffff | host2      |
      | test2    | 0x3   | 2    | 0xffff | host3      |
      | test2    | 0x4   | 3    | 0xffff | host4      |
      | test3    | 0x5   | 1    | 0xffff | host5      |
      | test3    | 0x6   | 1    | 0xffff | host6      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_routing_switch/sliceable_routing_switch -s tmp/slice.db -f tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("sliceable_routing_switch1") { datapath_id "0x1" }
      vswitch("sliceable_routing_switch2") { datapath_id "0x2" }
      vswitch("sliceable_routing_switch3") { datapath_id "0x3" }
      vswitch("sliceable_routing_switch4") { datapath_id "0x4" }
      vswitch("sliceable_routing_switch5") { datapath_id "0x5" }
      vswitch("sliceable_routing_switch6") { datapath_id "0x6" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }
      vhost("host5") { mac "00:00:00:00:00:05" }
      vhost("host6") { mac "00:00:00:00:00:06" }

      link "sliceable_routing_switch1", "host1"
      link "sliceable_routing_switch2", "host2"
      link "sliceable_routing_switch3", "host3"
      link "sliceable_routing_switch4", "host4"
      link "sliceable_routing_switch5", "host5"
      link "sliceable_routing_switch6", "host6"
      link "sliceable_routing_switch1", "sliceable_routing_switch2"
      link "sliceable_routing_switch1", "sliceable_routing_switch3"
      link "sliceable_routing_switch2", "sliceable_routing_switch4"
      link "sliceable_routing_switch3", "sliceable_routing_switch5"
      link "sliceable_routing_switch4", "sliceable_routing_switch6"

      app { path "../apps/topology/topology" }
      app { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_routing_switch"
      """
      And wait until "sliceable_routing_switch" is up
      And *** sleep 15 ***
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical
      And I send packets from host3 to host4 (duration = 10)
      And I try to run "./trema show_stats host3 --tx" (log = "tx.host3.log")
      And I try to run "./trema show_stats host4 --rx" (log = "rx.host4.log")
    Then the content of "tx.host3.log" and "rx.host4.log" should be identical
      And I send packets from host4 to host3 (duration = 10)
      And I try to run "./trema show_stats host4 --tx" (log = "tx.host4.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host3.log")
    Then the content of "tx.host4.log" and "rx.host3.log" should be identical
      And I send packets from host5 to host6 (duration = 10)
      And I try to run "./trema show_stats host5 --tx" (log = "tx.host5.log")
      And I try to run "./trema show_stats host6 --rx" (log = "rx.host6.log")
    Then the content of "tx.host5.log" and "rx.host6.log" should be identical
      And I send packets from host6 to host5 (duration = 10)
      And I try to run "./trema show_stats host4 --tx" (log = "tx.host6.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host5.log")
    Then the content of "tx.host6.log" and "rx.host5.log" should be identical
