Feature: packetin_dispatcher app

  Scenario: compile packetin_dispatcher
    When I cd to "packetin_dispatcher"
    Then I successfully run `make`
