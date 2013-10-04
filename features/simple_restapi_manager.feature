Feature: simple_restapi_manager app

  Scenario: compile simple_restapi_manager
    When I cd to "simple_restapi_manager"
    Then I successfully run `make`
