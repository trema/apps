Feature: control multiple openflow switchies using sliceable_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, one slices, two servers, port binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 2    | 0xffff | host1      |
      | test1    | 0x1   | 1    | 0xffff | host2      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -a tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("sliceable_switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }

      link "sliceable_switch1", "host1"
      link "sliceable_switch1", "host2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I try to run "./trema send_packets --source host1 --dest host2"
    Then the number of flow entries on sliceable_switch1 should be 0

    When I try to run "./trema send_packets --source host2 --dest host1"
    Then the number of flow entries on sliceable_switch1 should be 1
    Then sliceable_switch1 should have a flow entry like in_port=1,actions=output:2
    Then sliceable_switch1 should not have a flow entry like in_port=2,actions=output:1

    When I try to run "./trema send_packets --source host1 --dest host2"
    Then the number of flow entries on sliceable_switch1 should be 2
    Then sliceable_switch1 should have a flow entry like in_port=2,actions=output:1


  Scenario: One openflow switch, one slices, two servers, port binding, setup_reverse_flow
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 2    | 0xffff | host1      |
      | test1    | 0x1   | 1    | 0xffff | host2      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -a tmp/filter.db --setup_reverse_flow" with following configuration (backgrounded):
      """
      vswitch("sliceable_switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }

      link "sliceable_switch1", "host1"
      link "sliceable_switch1", "host2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I try to run "./trema send_packets --source host1 --dest host2"
    Then the number of flow entries on sliceable_switch1 should be 0

    When I try to run "./trema send_packets --source host2 --dest host1"
    Then the number of flow entries on sliceable_switch1 should be 2
    Then sliceable_switch1 should have a flow entry like in_port=1,actions=output:2
    Then sliceable_switch1 should have a flow entry like in_port=2,actions=output:1
