Feature: REST interface for filters

  As a Trema user
  I want to handle filter objects via REST API
  So that I can configure filters on sliceable_switch


  Scenario: Create filter objects
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    When I try to send request POST /filters with
      """
      [{"id":"f1","priority":"1","ofp_wildcards":0,"in_port":"0x2","dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":"5","dl_vlan_pcp":"6","dl_type":"0x0800","nw_tos":"6","nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":"9","tp_dst":"10","wildcards":0,"in_datapath_id":"0xb","slice":"network-1","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |

    When I try to send request POST /filters with
      """
      [{"id":"f2","priority":2,"ofp_wildcards":0,"in_port":2,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":5,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"wildcards":0,"in_datapath_id":11,"slice":"network-1","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |
      | 2        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f2 |

    When I try to send request POST /filters with
      """
      [{"id":"f3","priority":3,"ofp_wildcards":0,"in_port":2,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":5,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"in_datapath_id":11,"slice":"network-1","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |
      | 2        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f2 |
      | 3        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f3 |

    When I try to send request POST /filters with
      """
      [{"id":"f4","priority":4,"ofp_wildcards":0,"in_port":2,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":5,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"slice":"network-1","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |
      | 2        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f2 |
      | 3        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f3 |
      | 4        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 1         | 0              | 1            | 0      | f4 |

    When I try to send request POST /filters with
      """
      [{"id":"f5","priority":5,"ofp_wildcards":0,"in_port":2,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":5,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |
      | 2        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f2 |
      | 3        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f3 |
      | 4        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 1         | 0              | 1            | 0      | f4 |
      | 5        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f5 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f6","priority":6,"in_port":2,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":5,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 6        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f6 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f7","priority":7,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan":5,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 7        | 0x1           | 0       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f7 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f8","priority":8,"dl_src":"00:00:00:00:00:03","dl_dst":"00:00:00:00:00:04","dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 8        | 0x3           | 0       | 3      | 4      | 0       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f8 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f9","priority":9,"dl_dst":"00:00:00:00:00:04","dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 9        | 0x7           | 0       | 0      | 4      | 0       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f9 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f10","priority":10,"dl_vlan_pcp":6,"dl_type":"0x0800","nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 10       | 0xf           | 0       | 0      | 0      | 0       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f10 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f11","priority":11,"dl_vlan_pcp":6,"nw_tos":6,"nw_proto":"0x04","nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 11       | 0x1f          | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 4        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f11 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f12","priority":12,"dl_vlan_pcp":6,"nw_tos":6,"nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_src":9,"tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 12       | 0x3f          | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 7      | 8      | 9      | 10     | 3         | 0              | 0            | 0      | f12 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f13","priority":13,"dl_vlan_pcp":6,"nw_tos":6,"nw_src":"0.0.0.7","nw_dst":"0.0.0.8","tp_dst":10,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 13       | 0x7f          | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 7      | 8      | 0      | 10     | 3         | 0              | 0            | 0      | f13 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f14","priority":14,"dl_vlan_pcp":6,"nw_tos":6,"nw_src":"0.0.0.7","nw_dst":"0.0.0.8","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 14       | 0xff          | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 7      | 8      | 0      | 0      | 3         | 0              | 0            | 0      | f14 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f15","priority":15,"dl_vlan_pcp":6,"nw_tos":6,"nw_src":"0.0.0.7/31","nw_dst":"0.0.0.8","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 15       | 0x1ff         | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 7      | 8      | 0      | 0      | 3         | 0              | 0            | 0      | f15 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f16","priority":16,"dl_vlan_pcp":6,"nw_tos":6,"nw_dst":"0.0.0.8","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 16       | 0x3fff        | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 0      | 8      | 0      | 0      | 3         | 0              | 0            | 0      | f16 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f17","priority":17,"dl_vlan_pcp":6,"nw_tos":6,"nw_dst":"0.0.0.8/31","action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 17       | 0x7fff        | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 0      | 8      | 0      | 0      | 3         | 0              | 0            | 0      | f17 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f18","priority":18,"dl_vlan_pcp":6,"nw_tos":6,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 18       | 0xfffff       | 0       | 0      | 0      | 0       | 6           | 0       | 6      | 0        | 0      | 0      | 0      | 0      | 3         | 0              | 0            | 0      | f18 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f19","priority":19,"nw_tos":6,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 19       | 0x1fffff      | 0       | 0      | 0      | 0       | 0           | 0       | 6      | 0        | 0      | 0      | 0      | 0      | 3         | 0              | 0            | 0      | f19 |

    Given the REST interface for sliceable_switch
    When I try to send request POST /filters with
      """
      [{"id":"f20","priority":20,"action":"ALLOW"}]
      """
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 20       | 0x3fffff      | 0       | 0      | 0      | 0       | 0           | 0       | 0      | 0        | 0      | 0      | 0      | 0      | 3         | 0              | 0            | 0      | f20 |

    When I try to send request POST /filters with
      """
      [{"id":"f20","priority":21,"action":"ALLOW"}]
      """
    Then the response status shoud be 422


  Scenario: List and get filter objects
    Given the REST interface for sliceable_switch
    When I try to send request GET /filters
    Then the response status shoud be 200
    And the response body shoud be:
      """
      []
      """

    Given the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into filter table
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |
    When I try to send request GET /filters
    Then the response status shoud be 200
    And the response body shoud be like:
      """
      \[\{(("priority":1|"ofp_wildcards":"NONE"|"in_port":2|"dl_src":"00:00:00:00:00:03"|"dl_dst":"00:00:00:00:00:04"|"dl_vlan":5|"dl_vlan_pcp":6|"dl_type":2048|"nw_tos":6|"nw_proto":4|"nw_src":"0.0.0.7"|"nw_dst":"0.0.0.8"|"tp_src":9|"tp_dst":10|"wildcards":"NONE"|"in_datapath_id":11|"slice":"network-1"|"action":"ALLOW"|"id":"f1"),?){19}\}\]
      """

    When I try to send request GET /filters/f1
    Then the response status shoud be 200
    And the response body shoud be like:
      """
      \{(("priority":1|"ofp_wildcards":"NONE"|"in_port":2|"dl_src":"00:00:00:00:00:03"|"dl_dst":"00:00:00:00:00:04"|"dl_vlan":5|"dl_vlan_pcp":6|"dl_type":2048|"nw_tos":6|"nw_proto":4|"nw_src":"0.0.0.7"|"nw_dst":"0.0.0.8"|"tp_src":9|"tp_dst":10|"wildcards":"NONE"|"in_datapath_id":11|"slice":"network-1"|"action":"ALLOW"|"id":"f1"),?){19}\}
      """

    Given the following records were inserted into filter table
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id  |
      | 20       | 0x3fffff      | 0       | 0      | 0      | 0       | 0           | 0       | 0      | 0        | 0      | 0      | 0      | 0      | 3         | 0              | 0            | 0      | f20 |
    When I try to send request GET /filters/f20
    Then the response status shoud be 200
    And the response body shoud be like:
      """
      \{(("priority":20|"ofp_wildcards":"((in_port|dl_src|dl_dst|dl_vlan|dl_vlan_pcp|dl_type|nw_tos|nw_proto|nw_src:32|nw_dst:32|tp_src|tp_dst),?){12}"|"in_port":0|"dl_src":"00:00:00:00:00:00"|"dl_dst":"00:00:00:00:00:00"|"dl_vlan":0|"dl_vlan_pcp":0|"dl_type":0|"nw_tos":0|"nw_proto":0|"nw_src":"0.0.0.0"|"nw_dst":"0.0.0.0"|"tp_src":0|"tp_dst":0|"wildcards":"((in_datapath_id|slice),?){2}"|"in_datapath_id":0|"slice":"UNDEFINED \(0\)"|"action":"ALLOW"|"id":"f20"),?){19}\}
      """

    When I try to send request GET /filters/f-non
    Then the response status shoud be 404


  Scenario: Delete a filter object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into filter table
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |
      | 1        | 0             | 2       | 3      | 4      | 5       | 6           | 2048    | 6      | 4        | 7      | 8      | 9      | 10     | 0         | 11             | 1            | 0      | f1 |
    When I try to send request DELETE /filters/f1
    Then the response status shoud be 202
    And the filter records shoud be:
      | priority | ofp_wildcards | in_port | dl_src | dl_dst | dl_vlan | dl_vlan_pcp | dl_type | nw_tos | nw_proto | nw_src | nw_dst | tp_src | tp_dst | wildcards | in_datapath_id | slice_number | action | id |

    When I try to send request DELETE /filters/f-non
    Then the response status shoud be 404
