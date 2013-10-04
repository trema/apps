Feature: simple_load_balancer app

  Scenario: compile simple_load_balancer
    When I cd to "simple_load_balancer"
    Then I successfully run `make`
