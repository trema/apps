Feature: control multiple openflow switchies using sliceable_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, 65535 slices, six servers, mac binding
    Given the slice table from script "../apps/sliceable_switch/test/65535_slices.pl"
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -a tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("sliceable_switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host65535") { mac "00:00:00:00:ff:ff" }
      vhost("host65536") { mac "00:00:00:01:00:00" }
      vhost("host131069") { mac "00:00:00:01:ff:fd" }
      vhost("host131070") { mac "00:00:00:01:ff:fe" }

      link "sliceable_switch1", "host1"
      link "sliceable_switch1", "host2"
      link "sliceable_switch1", "host65535"
      link "sliceable_switch1", "host65536"
      link "sliceable_switch1", "host131069"
      link "sliceable_switch1", "host131070"

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
      And I send packets from host65536 to host65535 (duration = 1)
      And I reset stats to host65536
      And I send packets from host65535 to host65536 (duration = 10)
      And I try to run "./trema show_stats host65535 --tx" (log = "tx.host65535.log")
      And I try to run "./trema show_stats host65536 --rx" (log = "rx.host65536.log")
    Then the content of "tx.host65535.log" and "rx.host65536.log" should be identical
      And I send packets from host65536 to host65535 (duration = 10)
      And I try to run "./trema show_stats host65536 --tx" (log = "tx.host65536.log")
      And I try to run "./trema show_stats host65535 --rx" (log = "rx.host65535.log")
    Then the content of "tx.host65536.log" and "rx.host65535.log" should be identical
      And I send packets from host131070 to host131069 (duration = 1)
      And I reset stats to host131070
      And I send packets from host131069 to host131070 (duration = 10)
      And I try to run "./trema show_stats host131069 --tx" (log = "tx.host131069.log")
      And I try to run "./trema show_stats host131070 --rx" (log = "rx.host131070.log")
    Then the content of "tx.host131069.log" and "rx.host131070.log" should be identical
      And I send packets from host131070 to host131069 (duration = 10)
      And I try to run "./trema show_stats host131070 --tx" (log = "tx.host131070.log")
      And I try to run "./trema show_stats host131069 --rx" (log = "rx.host131069.log")
    Then the content of "tx.host131070.log" and "rx.host131069.log" should be identical
