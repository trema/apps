Feature: control multiple openflow switchies using sliceable_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, two slices, four servers, mac binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
      | test2    | slice2      |
    And the following mac binding records
      | slice_id | address           | binding_id |
      | test1    | 00:00:00:00:00:01 | host1      |
      | test1    | 00:00:00:00:00:02 | host2      |
      | test2    | 00:00:00:00:00:03 | host3      |
      | test2    | 00:00:00:00:00:04 | host4      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -f tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("sliceable_switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }

      link "sliceable_switch1", "host1"
      link "sliceable_switch1", "host2"
      link "sliceable_switch1", "host3"
      link "sliceable_switch1", "host4"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***
      And I send packets from host2 to host1 (duration = 1)
      And I reset stats to host2
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical
      And I send packets from host4 to host3 (duration = 1)
      And I reset stats to host4
      And I send packets from host3 to host4 (duration = 10)
      And I try to run "./trema show_stats host3 --tx" (log = "tx.host3.log")
      And I try to run "./trema show_stats host4 --rx" (log = "rx.host4.log")
    Then the content of "tx.host3.log" and "rx.host4.log" should be identical
      And I send packets from host4 to host3 (duration = 10)
      And I try to run "./trema show_stats host4 --tx" (log = "tx.host4.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host3.log")
    Then the content of "tx.host4.log" and "rx.host3.log" should be identical


  Scenario: Six openflow switches, three slices, six servers, mac binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
      | test2    | slice2      |
      | test3    | slice3      |
    And the following mac binding records
      | slice_id | address           | binding_id |
      | test1    | 00:00:00:00:00:01 | host1      |
      | test1    | 00:00:00:00:00:02 | host2      |
      | test2    | 00:00:00:00:00:03 | host3      |
      | test2    | 00:00:00:00:00:04 | host4      |
      | test3    | 00:00:00:00:00:05 | host5      |
      | test3    | 00:00:00:00:00:06 | host6      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -f tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("sliceable_switch1") { datapath_id "0x1" }
      vswitch("sliceable_switch2") { datapath_id "0x2" }
      vswitch("sliceable_switch3") { datapath_id "0x3" }
      vswitch("sliceable_switch4") { datapath_id "0x4" }
      vswitch("sliceable_switch5") { datapath_id "0x5" }
      vswitch("sliceable_switch6") { datapath_id "0x6" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }
      vhost("host5") { mac "00:00:00:00:00:05" }
      vhost("host6") { mac "00:00:00:00:00:06" }

      link "sliceable_switch1", "host1"
      link "sliceable_switch2", "host2"
      link "sliceable_switch3", "host3"
      link "sliceable_switch4", "host4"
      link "sliceable_switch5", "host5"
      link "sliceable_switch6", "host6"
      link "sliceable_switch1", "sliceable_switch2"
      link "sliceable_switch1", "sliceable_switch3"
      link "sliceable_switch2", "sliceable_switch4"
      link "sliceable_switch3", "sliceable_switch5"
      link "sliceable_switch4", "sliceable_switch6"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***
      And I send packets from host2 to host1 (duration = 1)
      And I reset stats to host2
      And I send packets from host1 to host2 (duration = 10)
      And I try to run "./trema show_stats host1 --tx" (log = "tx.host1.log")
      And I try to run "./trema show_stats host2 --rx" (log = "rx.host2.log")
    Then the content of "tx.host1.log" and "rx.host2.log" should be identical
      And I send packets from host2 to host1 (duration = 10)
      And I try to run "./trema show_stats host2 --tx" (log = "tx.host2.log")
      And I try to run "./trema show_stats host1 --rx" (log = "rx.host1.log")
    Then the content of "tx.host2.log" and "rx.host1.log" should be identical
      And I send packets from host4 to host3 (duration = 1)
      And I reset stats to host4
      And I send packets from host3 to host4 (duration = 10)
      And I try to run "./trema show_stats host3 --tx" (log = "tx.host3.log")
      And I try to run "./trema show_stats host4 --rx" (log = "rx.host4.log")
    Then the content of "tx.host3.log" and "rx.host4.log" should be identical
      And I send packets from host4 to host3 (duration = 10)
      And I try to run "./trema show_stats host4 --tx" (log = "tx.host4.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host3.log")
    Then the content of "tx.host4.log" and "rx.host3.log" should be identical
      And I send packets from host6 to host5 (duration = 1)
      And I reset stats to host6
      And I send packets from host5 to host6 (duration = 10)
      And I try to run "./trema show_stats host5 --tx" (log = "tx.host5.log")
      And I try to run "./trema show_stats host6 --rx" (log = "rx.host6.log")
    Then the content of "tx.host5.log" and "rx.host6.log" should be identical
      And I send packets from host6 to host5 (duration = 10)
      And I try to run "./trema show_stats host4 --tx" (log = "tx.host6.log")
      And I try to run "./trema show_stats host3 --rx" (log = "rx.host5.log")
    Then the content of "tx.host6.log" and "rx.host5.log" should be identical
