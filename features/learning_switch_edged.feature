Feature: learning_switch_edged app

  @wip
  Scenario: compile learning_switch_edged
    When I cd to "learning_switch_edged"
    Then I successfully run `make`
