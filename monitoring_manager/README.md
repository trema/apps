Monitoring Manager
==================

**!!!! NOTICE !!!!**
**THIS APPLICATION IS STILL UNDER DEVELOPMENT AND MAY NOT WORK AS EXPECTED.**

What's this?
------------

Monitoring manager acts as a proxy to provides high-level APIs for other 
Trema apps which are based on this library to monitor port/switch loading 
and flow statistics. It has a routine job to regularly query Flow Stats 
and Port Stats and summarize them in its data structue. When other Trema apps 
subscribe notifications, they will receive the messages when Port Loading 
or Flow bit rate reaches its setting of threshold or criteria . And also, 
other Trema apps can request or query Port Loading in any time.

Software architecture/hierarchy is as follows:

                +-------------------+  
                |        App        |  
                +-------------------+  
                |   libmonitoring   |  
                +-------------------+  
                          |  Monitoring Manager Interface
      +-----+   +------------------------------+
      | App |   |     Monitoring Manager       |
      +-----+   +------------------------------+
         |                     |  OpenFlow Application Interface
      +----------------------------------------+
      |                  Trema (core)          |
      +----------------------------------------+

The configuration for monitoring manager:
* port_percentage_condition
 * The threshold of port loading percentage
* port_setting_feature_rate
 * The speed rate of port
* flow_bit_rate_conditon
 * The threshold of flow loading
* flow_times_condition
 * How many seconds will flow become big flow when being over the threshold of flow loading

The criteria for monitoring manager to send notification
* Port Loading Notification
 * Check port loading per 4 seconds
 * Calculate the percentage of port loading
  * port_bit_rate = avg_rx_bytes * 8 
  * port_loading_percentage = port_bit_rate  * 100 / port_feature_rate
 * If the port loading percentage is higer than port_percentage_condition, then it sends port loading notification
* Flow Loading Notification
 * Check flow loading per 4 seconds
 * Calculate the flow bit rate
  * flow_bit_rate = bytes_count * 8 / duration seconds
 * If flow_bit_rate  is bigger than flow_bit_rate_condition, then flow_times addes1
 * If flow_times is higher than flow_times_condition (exp: 3) , then it sends flow loading notification


How to use
----------

Please refer to examples as references for understanding
how to use the APIs.

How to build
------------

  Build Trema and slicesable routing switch

        $ git clone git://github.com/trema/trema.git trema
        $ git clone git://github.com/teyenliu/apps.git apps
        $ cd trema
        $ ./build.rb
        $ cd ../apps/monitoring_manager

  Build monitoring manager and examples

        $ make

How to run examples
-------------------

    $ trema run -c ./examples/example1.conf


The result
----------
After running the example1 successfully, it will has the following info:

    set_service_name = libmonitoring.21983
    [send_monitoring_subscribe_request] ( tag = 0xd004, data = 0x1028280, length = 32 ).
    [send_my_port_loading_request] is triggered
    [handle_port_loading_reply] 0x000000000000e4, port = 1, curr = 1, avg_rx_bytes = 32767, avg_tx_bytes = 0, avg_rx_packets = 8098, avg_tx_packets = 42414, loading = 0
    
