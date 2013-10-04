Feature: C apps compiles with the latest trema

  Scenario: compile broadcast_helper
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../broadcast_helper"
    Then I successfully run `make`

  Scenario: compile flow_dumper
    When I cd to "flow_dumper"
    Then I successfully run `make`

  Scenario: compile flow_manager
    When I cd to "flow_manager"
    Then I successfully run `make`

  @wip
  Scenario: compile group_dumper
    When I cd to "group_dumper"
    Then I successfully run `make`

  @wip
  Scenario: compile learning_switch_edged
    When I cd to "learning_switch_edged"
    Then I successfully run `make`

  Scenario: compile load_balance_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../load_balance_switch"
    Then I successfully run `make`

  @wip
  Scenario: compile monitoring_manager
    When I cd to "monitoring_manager"
    Then I successfully run `make`

  Scenario: compile packetin_dispatcher
    When I cd to "packetin_dispatcher"
    Then I successfully run `make`

  Scenario: compile path_manager
    When I cd to "path_manager"
    Then I successfully run `make`

  Scenario: compile redirectable_routing_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../redirectable_routing_switch"
    Then I successfully run `make`

  Scenario: compile routing_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../routing_switch"
    Then I successfully run `make`

  Scenario: compile show_description
    When I cd to "show_description"
    Then I successfully run `make`

  Scenario: compile simple_load_balancer
    When I cd to "simple_load_balancer"
    Then I successfully run `make`

    Scenario: compile simple_restapi_manager
    When I cd to "simple_restapi_manager"
    Then I successfully run `make`

  Scenario: compile sliceable_switch
    Given I cd to "topology"
    And I successfully run `make`
    When I cd to "../sliceable_switch"
    Then I successfully run `make`

  Scenario: compile topology
    When I cd to "topology"
    Then I successfully run `make`

  Scenario: compile transaction_manager
    When I cd to "transaction_manager"
    Then I successfully run `make`
