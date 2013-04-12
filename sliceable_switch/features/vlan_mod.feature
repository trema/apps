Feature: control multiple openflow switchies using sliceable_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_switch application
  So that I can modify flow setup properly


  Scenario: Two openflow switch, one slice, two servers, port and mac binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 1    | 0xffff | host1      |
      | test1    | 0x2   | 1    | 0x0007 | host2      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -a tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0x1" }
      vswitch("switch2") { datapath_id "0x2" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }

      link "switch1", "host1"
      link "switch2", "host2"
      link "switch1", "switch2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I send 1 packets from host2 to host1
    And I send 1 packets from host1 to host2
    Then switch1 should have a flow entry like in_port=1,actions=output:2
    And switch2 should have a flow entry like in_port=2,actions=mod_vlan_vid:7,output:1


  Scenario: Two openflow switch, one slice, two servers, port and mac binding, set_reverse_flow
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 1    | 0xffff | host1      |
      | test1    | 0x2   | 1    | 0x0007 | host2      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -a tmp/filter.db --setup_reverse_flow" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0x1" }
      vswitch("switch2") { datapath_id "0x2" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }

      link "switch1", "host1"
      link "switch2", "host2"
      link "switch1", "switch2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I send 1 packets from host2 to host1
    And I send 1 packets from host1 to host2
    Then switch1 should have a flow entry like in_port=1,actions=output:2
    And switch1 should have a flow entry like in_port=2,actions=strip_vlan,output:1
    And switch2 should have a flow entry like in_port=2,actions=mod_vlan_vid:7,output:1
    And switch2 should have a flow entry like in_port=1,actions=output:2
