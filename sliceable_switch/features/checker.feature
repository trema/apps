Feature: check port number

  As a Trema user
  I want to respond to packet_in events
  So that I can known the port number is used


  Scenario: One openflow switch, four servers
    When I try trema run "../apps/sliceable_switch/checker" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0x1" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }

      link "switch1", "host1"
      link "switch1", "host2"
      link "switch1", "host3"
      link "switch1", "host4"
      """
      And wait until "checker" is up
      And I try to run "./trema send_packets --source host1 --dest host2"
      And I try to run "./trema send_packets --source host2 --dest host3"
      And I try to run "./trema send_packets --source host3 --dest host4"
      And I try to run "./trema send_packets --source host4 --dest host1"
      And *** sleep 1 ***
      And I terminated all trema services
    Then the output should include:
      """
      192.168.0.1: datapath_id: 0x1, in_port: 3, vid: 0xffff, ether address: 00:00:00:00:00:01
      192.168.0.2: datapath_id: 0x1, in_port: 1, vid: 0xffff, ether address: 00:00:00:00:00:02
      192.168.0.3: datapath_id: 0x1, in_port: 2, vid: 0xffff, ether address: 00:00:00:00:00:03
      192.168.0.4: datapath_id: 0x1, in_port: 4, vid: 0xffff, ether address: 00:00:00:00:00:04

      """


  Scenario: Two openflow switches, four servers
    When I try trema run "../apps/sliceable_switch/checker" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0x1" }
      vswitch("switch2") { datapath_id "0x2" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }

      link "switch1", "host1"
      link "switch2", "host2"
      link "switch2", "host3"
      link "switch2", "host4"
      link "switch1", "switch2"
      """
      And wait until "checker" is up
      And I try to run "./trema send_packets --source host1 --dest host2"
      And I try to run "./trema send_packets --source host2 --dest host3"
      And I try to run "./trema send_packets --source host3 --dest host4"
      And I try to run "./trema send_packets --source host4 --dest host1"
      And *** sleep 1 ***
      And I terminated all trema services
    Then the output should include:
      """
      192.168.0.1: datapath_id: 0x1, in_port: 2, vid: 0xffff, ether address: 00:00:00:00:00:01
      192.168.0.2: datapath_id: 0x2, in_port: 1, vid: 0xffff, ether address: 00:00:00:00:00:02
      192.168.0.3: datapath_id: 0x2, in_port: 2, vid: 0xffff, ether address: 00:00:00:00:00:03
      192.168.0.4: datapath_id: 0x2, in_port: 4, vid: 0xffff, ether address: 00:00:00:00:00:04

      """


  Scenario: Six openflow switches, four servers
    When I try trema run "../apps/sliceable_switch/checker" with following configuration (backgrounded):
      """
      vswitch("switch1") { datapath_id "0x1" }
      vswitch("switch2") { datapath_id "0x2" }
      vswitch("switch3") { datapath_id "0x3" }
      vswitch("switch4") { datapath_id "0x4" }
      vswitch("switch5") { datapath_id "0x5" }
      vswitch("switch6") { datapath_id "0x6" }

      vhost("host1") { mac "00:00:00:00:00:01" }
      vhost("host2") { mac "00:00:00:00:00:02" }
      vhost("host3") { mac "00:00:00:00:00:03" }
      vhost("host4") { mac "00:00:00:00:00:04" }
      vhost("host5") { mac "00:00:00:00:00:05" }
      vhost("host6") { mac "00:00:00:00:00:06" }

      link "switch1", "host1"
      link "switch2", "host2"
      link "switch3", "host3"
      link "switch4", "host4"
      link "switch5", "host5"
      link "switch6", "host6"
      link "switch1", "switch2"
      link "switch1", "switch3"
      link "switch2", "switch4"
      link "switch3", "switch5"
      link "switch4", "switch6"

      """
      And wait until "checker" is up
      And I try to run "./trema send_packets --source host1 --dest host2"
      And I try to run "./trema send_packets --source host2 --dest host3"
      And I try to run "./trema send_packets --source host3 --dest host4"
      And I try to run "./trema send_packets --source host4 --dest host5"
      And I try to run "./trema send_packets --source host5 --dest host6"
      And I try to run "./trema send_packets --source host6 --dest host1"
      And *** sleep 1 ***
      And I terminated all trema services
    Then the output should include:
      """
      192.168.0.1: datapath_id: 0x1, in_port: 3, vid: 0xffff, ether address: 00:00:00:00:00:01
      192.168.0.2: datapath_id: 0x2, in_port: 1, vid: 0xffff, ether address: 00:00:00:00:00:02
      192.168.0.3: datapath_id: 0x3, in_port: 2, vid: 0xffff, ether address: 00:00:00:00:00:03
      192.168.0.4: datapath_id: 0x4, in_port: 3, vid: 0xffff, ether address: 00:00:00:00:00:04
      192.168.0.5: datapath_id: 0x5, in_port: 1, vid: 0xffff, ether address: 00:00:00:00:00:05
      192.168.0.6: datapath_id: 0x6, in_port: 1, vid: 0xffff, ether address: 00:00:00:00:00:06

      """
