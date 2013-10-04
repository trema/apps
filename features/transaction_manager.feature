Feature: transaction_manager app

  Scenario: compile transaction_manager
    When I cd to "transaction_manager"
    Then I successfully run `make`
