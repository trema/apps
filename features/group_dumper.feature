Feature: group_dumper app

  @wip
  Scenario: compile group_dumper
    When I cd to "group_dumper"
    Then I successfully run `make`
