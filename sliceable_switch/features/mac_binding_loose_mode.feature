Feature: control multiple openflow switchies using sliceable_switch

  As a Trema user
  I want to control multiple openflow switches using sliceable_switch application
  So that I can send and receive packets


  Scenario: One openflow switch, two slices, four servers, mac binding ( loose mode )
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
      | test2    | slice2      |
    And the following mac binding records
      | slice_id | address           | binding_id |
      | test1    | 00:00:00:00:00:01 | host1      |
      | test1    | 00:00:00:00:00:02 | host2      |
      | test2    | 00:00:00:00:00:03 | host3      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -f tmp/filter.db --loose" with following configuration (backgrounded):
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

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I send 1 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     0 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     0 |     1 |     0 |     0 |

    When I send 1 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     1 |     0 |     0 |

    When I send 1 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     2 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     0 |     0 |

    When I send 1 packets from host1 to host3
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     3 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     0 |

    When I send 1 packets from host3 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     3 |     1 |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     0 |

    When I send 1 packets from host1 to host3
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     4 |     1 |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     0 |

    When I send 1 packets from host1 to host4
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     5 |     1 |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     1 |

    When I send 1 packets from host4 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     5 |     1 |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     1 |

    When I send 1 packets from host1 to host4
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     6 |     1 |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     1 |


  Scenario: Two openflow switch, two slices, four servers, mac binding ( loose mode )
    Given the following slice records
      | slice_id | description |
      | test1    | slice1      |
      | test2    | slice2      |
    And the following mac binding records
      | slice_id | address           | binding_id |
      | test1    | 00:00:00:00:00:01 | host1      |
      | test1    | 00:00:00:00:00:02 | host2      |
      | test2    | 00:00:00:00:00:03 | host3      |
    And the following filter records
      | filter_id | rule_specification      |
      | default   | priority=0 action=ALLOW |
    When I try trema run "../apps/sliceable_switch/sliceable_switch -s tmp/slice.db -f tmp/filter.db --loose" with following configuration (backgrounded):
      """
      vswitch("sliceable_switch1") { datapath_id "0x1" }
      vswitch("sliceable_switch2") { datapath_id "0x2" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }

      link "sliceable_switch1", "host1"
      link "sliceable_switch2", "host2"
      link "sliceable_switch2", "host3"
      link "sliceable_switch2", "host4"
      link "sliceable_switch1", "sliceable_switch2"

      run { path "../apps/topology/topology" }
      run { path "../apps/topology/topology_discovery" }

      event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
      filter :lldp => "topology_discovery", :packet_in => "sliceable_switch"
      """
      And wait until "sliceable_switch" is up
      And *** sleep 15 ***

    When I send 1 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     0 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     0 |     1 |     0 |     0 |

    When I send 1 packets from host2 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     1 |     0 |     0 |

    When I send 1 packets from host1 to host2
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     2 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     0 |     0 |

    When I send 1 packets from host1 to host3
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     3 |     1 |     0 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     0 |

    When I send 1 packets from host3 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     3 |     1 |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     0 |

    When I send 1 packets from host1 to host3
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     4 |     1 |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     0 |

    When I send 1 packets from host1 to host4
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     5 |     1 |     1 |     0 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     1 |

    When I send 1 packets from host4 to host1
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     5 |     1 |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     1 |

    When I send 1 packets from host1 to host4
    Then the total number of tx packets should be:
      | host1 | host2 | host3 | host4 |
      |     6 |     1 |     1 |     1 |
    And the total number of rx packets should be:
      | host1 | host2 | host3 | host4 |
      |     1 |     2 |     1 |     1 |
