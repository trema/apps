Feature: flow_manager app

  Scenario: compile flow_manager
    When I cd to "flow_manager"
    Then I successfully run `make`
