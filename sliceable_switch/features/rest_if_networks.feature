Feature: REST interface for filters

  As a Trema user
  I want to handle filters via REST API
  So that I can configure filter rules on sliceable_switch


  Scenario: create network objects via REST API
    Given the REST interface for sliceable_switch
    When I try to send request POST /networks with
      """
      {"id":"network-1", "description":"Network #1"}
      """
    Then the response status shoud be 202
    Then the response body shoud be:
      """
      {"id":"network-1","description":"Network #1"}
      """

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      [{"id":"network-1","description":"Network #1"}]
      """

    When I try to send request POST /networks with
      """
      {"id":"network-2"}
      """
    Then the response status shoud be 202
    Then the response body shoud be:
      """
      {"id":"network-2","description":""}
      """

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      [{"id":"network-1","description":"Network #1"},{"id":"network-2","description":""}]
      """

    When I try to send request POST /networks with
      """
      {"description":"Network #3"}
      """
    Then the response status shoud be 202
    Then the response body shoud be like:
      """
      \{"id":"[^"]+","description":"Network #3"\}
      """

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be like:
      """
      \[(\{[^\}]+\},)*\{"id":"[^"]+","description":"Network #3"\}(,\{[^\}]+\})*\]
      """

    When I try to send request POST /networks with
      """
      {}
      """
    Then the response status shoud be 202
    Then the response body shoud be like:
      """
      \{"id":"[^"]+","description":""\}
      """

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be like:
      """
      \[\{"id":[^\}]+\},\{"id":[^\}]+\},\{"id":[^\}]+\},\{"id":[^\}]+\}\]
      """

    When I try to send request POST /networks with
      """
      {"id": "network-1", "description":"Network #1-dup"}
      """
    Then the response status shoud be 422

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be like:
      """
      \[\{"id":[^\}]+\},\{"id":[^\}]+\},\{"id":[^\}]+\},\{"id":[^\}]+\}\]
      """


  Scenario: list and get network objects via REST API
    Given the REST interface for sliceable_switch
    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      []
      """

    When I try to send request POST /networks with
      """
      {"id":"network-1", "description":"Network #1"}
      """
    Then the response status shoud be 202

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      [{"id":"network-1","description":"Network #1"}]
      """

    When I try to send request GET /networks/network-1
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      {"bindings":[],"description":"Network #1"}
      """

    When I try to send request GET /networks/network-non
    Then the response status shoud be 404


  Scenario: delete a network object via REST API
    Given the REST interface for sliceable_switch
    When I try to send request POST /networks with
      """
      {"id":"network-1", "description":"Network #1"}
      """
    Then the response status shoud be 202

    When I try to send request DELETE /networks/network-1
    Then the response status shoud be 202

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      []
      """

    When I try to send request DELETE /networks/network-non
    Then the response status shoud be 404



  Scenario: modify a network object via REST API
    Given the REST interface for sliceable_switch
    When I try to send request POST /networks with
      """
      {"id":"network-1", "description":"Network #1"}
      """
    Then the response status shoud be 202

    When I try to send request PUT /networks/network-1 with
      """
      {"description":"Network #1 modified"}
      """
    Then the response status shoud be 202

    When I try to send request GET /networks
    Then the response status shoud be 200
    Then the response body shoud be:
      """
      [{"id":"network-1","description":"Network #1 modified"}]
      """

    When I try to send request PUT /networks/network-non with
      """
      {"description":"Network None"}
      """
    Then the response status shoud be 404
