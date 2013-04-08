Feature: REST interface for networks

  As a Trema user
  I want to handle network objects via REST API
  So that I can configure networks(slices) on sliceable_switch


  Scenario: Create network objects
    Given the REST interface for sliceable_switch
    When I try to send request POST /networks with
      """
      {"id":"network-1", "description":"Network #1"}
      """
    Then the response status shoud be 202
    And the slice records shoud be:
      | number | id        | description |
      | 1      | network-1 | Network #1  |

    When I try to send request POST /networks with
      """
      {"id":"network-2"}
      """
    Then the response status shoud be 202
    And the slice records shoud be:
      | number | id        | description |
      | 1      | network-1 | Network #1  |
      | 2      | network-2 |             |

    When I try to send request POST /networks with
      """
      {"description":"Network #3"}
      """
    Then the response status shoud be 202
    And the response body shoud be like:
      """
      \{"id":"[^"]+","description":"Network #3"\}
      """
    And the slice records shoud be like:
      | number | id        | description |
      | 1      | network-1 | Network #1  |
      | 2      | network-2 |             |
      | 3      | .+        | Network #3  |

    When I try to send request POST /networks with
      """
      {}
      """
    Then the response status shoud be 202
    And the response body shoud be like:
      """
      \{"id":"[^"]+","description":""\}
      """
    And the slice records shoud be like:
      | number | id        | description |
      | 1      | network-1 | Network #1  |
      | 2      | network-2 |             |
      | 3      | .+        | Network #3  |
      | 4      | .+        |             |

    When I try to send request POST /networks with
      """
      {"id": "network-1", "description":"Network #1-dup"}
      """
    Then the response status shoud be 422
    And the slice records shoud be like:
      | number | id        | description |
      | 1      | network-1 | Network #1  |
      | 2      | network-2 |             |
      | 3      | .+        | Network #3  |
      | 4      | .+        |             |


  Scenario: List and get network objects
    Given the REST interface for sliceable_switch
    When I try to send request GET /networks
    Then the response status shoud be 200
    And the response body shoud be:
      """
      []
      """

    Given the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    When I try to send request GET /networks
    Then the response status shoud be 200
    And the response body shoud be:
      """
      [{"id":"network-1","description":"Network #1"}]
      """

    When I try to send request GET /networks/network-1
    Then the response status shoud be 200
    And the response body shoud be:
      """
      {"bindings":[],"description":"Network #1"}
      """

    When I try to send request GET /networks/network-non
    Then the response status shoud be 404


  Scenario: Delete a network object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    When I try to send request DELETE /networks/network-1
    Then the response status shoud be 202
    And the slice records shoud be:
      | number | id        | description |

    When I try to send request DELETE /networks/network-non
    Then the response status shoud be 404


  Scenario: Modify a network object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |

    When I try to send request PUT /networks/network-1 with
      """
      {"description":"Network #1 modified"}
      """
    Then the response status shoud be 202
    And the slice records shoud be:
      | number | id        | description         |
      | 1      | network-1 | Network #1 modified |

    When I try to send request PUT /networks/network-non with
      """
      {"description":"Network None"}
      """
    Then the response status shoud be 404


  Scenario: Create attachment objects to a network object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    When I try to send request POST /networks/network-1/attachments with
      """
      {"id":"attachment-1", "mac":"00:00:00:00:01:00"}
      """
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 2    |             |      |      | 256 | attachment-1 | 1            |

    When I try to send request POST /networks/network-non/attachments with
      """
      {"id":"attachment-2", "mac":"00:00:00:00:02:00"}
      """
    Then the response status shoud be 404

    When I try to send request POST /networks/network-1/attachments with
      """
      {"id":"attachment-1", "mac":"00:00:00:00:01:00"}
      """
    Then the response status shoud be 422


  Scenario: List and get network attachment objects
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    When I try to send request GET /networks/network-1/attachments
    Then the response status shoud be 200
    And the response body shoud be:
      """
      []
      """

    Given the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 2    |             |      |      | 256 | attachment-1 | 1            |
    When I try to send request GET /networks/network-1/attachments
    Then the response status shoud be 200
    And the response body shoud be:
      """
      [{"id":"attachment-1","mac":"00:00:00:00:01:00"}]
      """

    When I try to send request GET /networks/network-1/attachments/attachment-1
    Then the response status shoud be 200
    And the response body shoud be:
      """
      {"id":"attachment-1","mac":"00:00:00:00:01:00"}
      """

    When I try to send request GET /networks/network-non/attachments
    Then the response status shoud be 404

    When I try to send request GET /networks/network-1/attachments/attachment-non
    Then the response status shoud be 404


  Scenario: Delete a network attachment object
    Given the REST interface for sliceable_switch
    And the following records were inserted into slices table
      | number | id        | description |
      | 1      | network-1 | Network #1  |
    And the following records were inserted into bindings table
      | type | datapath_id | port | vid  | mac | id           | slice_number |
      | 2    |             |      |      | 256 | attachment-1 | 1            |
    When I try to send request DELETE /networks/network-1/attachments/attachment-1
    Then the response status shoud be 202
    And the binding records shoud be:
      | type | datapath_id | port | vid  | mac | id           | slice_number |

    When I try to send request DELETE /networks/network-non/attachments/attachment-1
    Then the response status shoud be 404

    When I try to send request DELETE /networks/network-1/attachments/attachment-non
    Then the response status shoud be 404
