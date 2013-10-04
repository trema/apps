Feature: topology app

  Scenario: compile topology
    When I cd to "topology"
    Then I successfully run `make`
