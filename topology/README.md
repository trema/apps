Topology
========

This directory includes sample implementations of topology management
components:

- Topology daemon is a service that provides topology information for
  trema apps.

- Topology discovery collects the topology information of OpenFlow
  switches. Topology discovery uses Link Layer Discovery Protocol
  (LLDP) to obtain topology information.

                     packet out
                 .---------------------------------------.
                 v                                       |
        +----------+           +-----------+           +-------+          +----------+
        |  switch  |  *    1   | packet in |  1    *   | trema |  *   1   | topology |
        |  daemon  | --------> |  filter   | --------> | apps  | <------> | daemon   |
        +----------+ packet in +-----------+ packet in +-------+ topology +----------+
          ^ 1    ^                       |                                  ^ 1
          |      |                       |                                  | topology
          |      `-------.               |                                  |
          v 1            |               |                                  v 1
        +----------+     |               |   packet in(LLDP)              +-----------+
        | openflow |     |               `------------------------------->| topology  |
        |  switch  |     `------------------------------------------------| discovery |
        +----------+                         packet out(LLDP)             +-----------+

- LLDP is transported over Ethernet by default but it also can be
  transported over IP.

- "show_topology" retrieves topology information from the topology
  daemon and prints it as graph-easy or trema network DSL style.

        $ TREMA_HOME=../../trema ./show_topology -G | graph-easy

* How to build

  Please see `../routing_switch/README`

* License & Terms

Copyright (C) 2008-2011 NEC Corporation

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

* Terms of Contributing to Trema program ("Program")

Please read the following terms before you submit to the Trema project
("Project") any original works of corrections, modifications,
additions, patches and so forth to the Program ("Contribution"). By
submitting the Contribution, you are agreeing to be bound by the
following terms.  If you do not or cannot agree to any of the terms,
please do not submit the Contribution:

1. You hereby grant to any person or entity receiving or distributing
   the Program through the Project a worldwide, perpetual,
   non-exclusive, royalty free license to use, reproduce, modify,
   prepare derivative works of, display, perform, sublicense, and
   distribute the Contribution and such derivative works.

2. You warrant that you have all rights necessary to submit the
   Contribution and, to the best of your knowledge, the Contribution
   does not infringe copyright, patent, trademark, trade secret, or
   other intellectual property rights of any third parties.

3. In the event that the Contribution is combined with third parties'
   programs, you notify to Project maintainers including NEC
   Corporation ("Maintainers") the author, the license condition, and
   the name of such third parties' programs.

4. In the event that the Maintainers incorporates the Contribution
   into the Program, your name will not be indicated in the copyright
   notice of the Program, but will be indicated in the contributor
   list on the Project website.
