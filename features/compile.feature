Feature: C apps compiles with the latest trema

  Scenario: compile topology
    When I cd to "topology"
    Then I successfully run `make`
