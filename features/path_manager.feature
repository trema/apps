Feature: path_manager app

  Scenario: compile path_manager
    When I cd to "path_manager"
    Then I successfully run `make`
