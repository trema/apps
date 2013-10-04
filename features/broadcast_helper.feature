Feature: broadcast_helper app

  Scenario: compile broadcast_helper
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../broadcast_helper"
    Then I successfully run `make`
