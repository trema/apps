Feature: show_description app

  Scenario: compile show_description
    When I cd to "show_description"
    Then I successfully run `make`
