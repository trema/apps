Feature: load_balance_switch app

  Scenario: compile load_balance_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../load_balance_switch"
    Then I successfully run `make`
