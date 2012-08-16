Feature: control multiple openflow switchies using sliceable_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, two slices, four servers, port binding
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 3    | 0xffff | host1      |
      | test1    | 0x1   | 1    | 0xffff | host2      |
      | test1    | 0x1   | 2    | 0xffff | host3      |
      | test1    | 0x1   | 4    | 0xffff | host4      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -a tmp/filter.db" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }

      link "switch1", "host1"
      link "switch1", "host2"
      link "switch1", "host3"
      link "switch1", "host4"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }
      run { path "../apps/flow_manager/flow_manager" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I send 1 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     0 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     0 |     0 |     0 |

    When I send 10 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |    10 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |    10 |     0 |     0 |

    When I send 10 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |    10 |    11 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |    11 |    10 |     0 |     0 |

    When I try trema kill "switch1"
      And *** sleep 2 ***
      And I try trema up "switch1"
      And *** sleep 15 ***
      And I reset stats to host1
      And I reset stats to host2
      And I send 1 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     0 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     0 |     0 |     0 |

    When I send 10 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |    10 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |    10 |     0 |     0 |

    When I send 10 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |    10 |    11 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |    11 |    10 |     0 |     0 |

