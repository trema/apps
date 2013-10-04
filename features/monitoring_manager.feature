Feature: monitoring_manager app

  Scenario: compile monitoring_manager
    Given I cd to "topology"
    And I successfully run `make`
    And I cd to "../flow_manager"
    And I successfully run `make`
    When I cd to "../monitoring_manager"
    Then I successfully run `make`
