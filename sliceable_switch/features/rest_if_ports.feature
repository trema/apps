Feature: REST interface for ports

  As a Trema user
  I want to handle port objects via REST API
  So that I can configure ports(port-bindings) on sliceable_switch


  Scenario: create port objects via REST API
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |

    When I try to send request POST /networks/network-1/ports with
      """
      {"id":"port-1","datapath_id":"0x0000000000001234","port":1,"vid":1024}
      """
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id     | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1 | 1            |

    When I try to send request POST /networks/network-1/ports with
      """
      {"datapath_id":"0x0000000000001234","port":2,"vid":1024}
      """
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id                     | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1                 | 1            |
      | 1    | 4660        | 2    | 1024 |     | 000000001234:0002:0400 | 1            |

    When I try to send request POST /networks/network-1/ports with
      """
      {"datapath_id":"0x0000000000001234","port":2,"vid":1024}
      """
    Then the response status shoud be 422
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id                     | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1                 | 1            |
      | 1    | 4660        | 2    | 1024 |     | 000000001234:0002:0400 | 1            |

    When I try to send request POST /networks/network-non/ports with
      """
      {"datapath_id":"0x0000000000001234","port":3,"vid":1024}
      """
    Then the response status shoud be 404
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id                     | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1                 | 1            |
      | 1    | 4660        | 2    | 1024 |     | 000000001234:0002:0400 | 1            |


  Scenario: list and get port objects via REST API
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |

    When I try to send request GET /networks/network-1/ports
    Then the response status shoud be 200
    And the response body shoud be:
      """
      []
      """

    Given the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |
    When I try to send request GET /networks/network-1/ports
    Then the response status shoud be 200
    And the response body shoud be:
      """
      [{"vid":1024,"datapath_id":"4660","id":"port-1","port":1}]
      """

    When I try to send request GET /networks/network-1/ports/port-1
    Then the response status shoud be 200
    And the response body shoud be:
      """
      {"config":{"vid":1024,"datapath_id":"4660","port":1}}
      """

    When I try to send request GET /networks/network-non/ports
    Then the response status shoud be 404

    When I try to send request GET /networks/network-1/ports/port-non
    Then the response status shoud be 404


  Scenario: delete a port object via REST API
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |
    When I try to send request DELETE /networks/network-1/ports/port-1
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id           | slice_number |

    When I try to send request DELETE /networks/network-non/ports/port-2
    Then the response status shoud be 404

    When I try to send request DELETE /networks/network-1/ports/port-non
    Then the response status shoud be 404


  Scenario: Create attachment objects to a port object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |
    When I try to send request POST /networks/network-1/ports/port-1/attachments with
      """
      {"id":"attachment-1", "mac":"00:00:00:00:01:00"}
      """
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |
      | 4    | 4660        | 1    | 1024 | 256 | attachment-1 | 1            |

    When I try to send request POST /networks/network-non/ports/port-1/attachments with
      """
      {"id":"attachment-2", "mac":"00:00:00:00:02:00"}
      """
    Then the response status shoud be 404

    When I try to send request POST /networks/network-1/ports/port-non/attachments with
      """
      {"id":"attachment-2", "mac":"00:00:00:00:02:00"}
      """
    Then the response status shoud be 422

    When I try to send request POST /networks/network-1/ports/port-1/attachments with
      """
      {"id":"attachment-1", "mac":"00:00:00:00:01:00"}
      """
    Then the response status shoud be 422


  Scenario: List and get prot attachment objects
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |
    When I try to send request GET /networks/network-1/attachments
    Then the response status shoud be 200
    And the response body shoud be:
      """
      []
      """

    Given the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 4    | 4660        | 1    | 1024 | 256 | attachment-1 | 1            |
    When I try to send request GET /networks/network-1/ports/port-1/attachments
    Then the response status shoud be 200
    And the response body shoud be:
      """
      [{"id":"attachment-1","mac":"00:00:00:00:01:00"}]
      """

    When I try to send request GET /networks/network-non/ports/port-1/attachments
    Then the response status shoud be 404

    When I try to send request GET /networks/network-1/ports/port-1/attachments/attachment-non
    Then the response status shoud be 404


  Scenario: Delete a port attachment object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |
      | 4    | 4660        | 1    | 1024 | 256 | attachment-1 | 1            |
    When I try to send request DELETE /networks/network-1/ports/port-1/attachments/attachment-1
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 1    | 4660        | 1    | 1024 |     | port-1       | 1            |

    When I try to send request DELETE /networks/network-1/ports/port-1/attachments/attachment-non
    Then the response status shoud be 404
