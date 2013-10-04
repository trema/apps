Feature: routing_switch app

  Scenario: compile routing_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../routing_switch"
    Then I successfully run `make`
