Feature: control multiple openflow switchies using sliceable_routing_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_routing_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, one slices, two servers, one port binding, one port and mac binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 2    | 0xffff | host1      |
      | test1    | 0x1   | 1    | 0xffff | host2      |
    And the following port and mac binding records
      | slice_id | port_binding_id | address           | binding_id |
      | test1    | host2           | 00:00:00:00:00:02 | host2_mac  |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_routing_switch/sliceable_routing_switch -s tmp/slice.db -f tmp/filter.db --restrict_hosts" with following configuration (backgrounded):
      """
      vswitch("sliceable_routing_switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }

      link "sliceable_routing_switch1", "host1"
      link "sliceable_routing_switch1", "host2"

      app { path "../apps/topology/topology" }
      app { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_routing_switch"
      """
      And wait until "sliceable_routing_switch" is up
      And *** sleep 15 ***

    When I send 1 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     0 |

    When I send 1 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     1 |     0 |

    When I send 1 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     2 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     1 |     0 |
