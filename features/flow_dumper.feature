Feature: flow_dumper app

  Scenario: compile flow_dumper
    When I cd to "flow_dumper"
    Then I successfully run `make`
