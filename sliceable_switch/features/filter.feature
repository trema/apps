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
      | filter_id | rule_specification                 |
      | allow_ssh | priority=10 tp_dst=22 action=ALLOW |
      | default   | priority=0 action=DENY             |
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

    When I try to run "./trema send_packets --source host1 --dest host1 --tp_src 123 --tp_dst 20"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     0 |

    When I try to run "./trema send_packets --source host2 --dest host2 --tp_src 22 --tp_dst 20"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     0 |

    When I try to run "./trema send_packets --source host1 --dest host2 --tp_src 123 --tp_dst 22"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     2 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     1 |

    When I try to run "./trema send_packets --source host2 --dest host1 --tp_src 22 --tp_dst 123"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     2 |     2 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     1 |


  Scenario: One openflow switch, one slices, two servers, port binding, setup_reverse_flow
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
    And the following port binding records
      | slice_id | dpid  | port | vid    | binding_id |
      | test1    | 0x1   | 2    | 0xffff | host1      |
      | test1    | 0x1   | 1    | 0xffff | host2      |
    And the following filter records
      | filter_id | rule_specification                 |
      | allow_ssh | priority=10 tp_dst=22 action=ALLOW |
      | default   | priority=0 action=DENY             |
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

    When I try to run "./trema send_packets --source host1 --dest host1 --tp_src 123 --tp_dst 20"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     0 |

    When I try to run "./trema send_packets --source host2 --dest host2 --tp_src 22 --tp_dst 20"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     0 |

    When I try to run "./trema send_packets --source host1 --dest host2 --tp_src 123 --tp_dst 22"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     2 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     0 |     1 |

    When I try to run "./trema send_packets --source host2 --dest host1 --tp_src 22 --tp_dst 123"
    Then the total number of tx packets should be:
      | host1 | host2 |
      |     2 |     2 |
    And the total number of rx packets should be:
      | host1 | host2 |
      |     1 |     1 |

