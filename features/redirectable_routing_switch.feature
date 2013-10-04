Feature: redirectable_routing_switch app

  Scenario: compile redirectable_routing_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../redirectable_routing_switch"
    Then I successfully run `make`
