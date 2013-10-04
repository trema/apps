Feature: sliceable_switch app

  Scenario: compile sliceable_switch
    Given I cd to "topology"
    And I successfully run `make`
    And I cd to "../flow_manager"
    And I successfully run `make`
    When I cd to "../sliceable_switch"
    Then I successfully run `make`
