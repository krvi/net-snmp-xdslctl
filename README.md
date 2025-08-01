# net-snmp-xdslctl

## Problem
The Zyxel VMG4005-B50A xDSL modem does not include native SNMP support.
However, root access can be obtained, and the system has a few megabytes
of writable space suitable for a custom SNMP agent.
[net-snmp](https://github.com/net-snmp/net-snmp) can be cross-compiled
for the platform but it lacks built-in support for ADSL2-LINE-MIB and
VDSL2-LINE-MIB.

To my knowledge, the Zyxel-provided GPL source does not include any
code that accesses xDSL line statistics programmatically or directly
from the chipset.

## Solution
This module provides a partial, read-only implementation of
VDSL2-LINE-MIB by parsing the output of the firmware-included
`xdslctl` tool.
The code focuses on extracting key values useful for monitoring tools
like [LibreNMS](https://github.com/librenms/librenms).  
Much of the MIB boilerplate is generated using `mib2c`.

### Sample output
```
VDSL2-LINE-MIB::xdsl2LineStatusAttainableRateDs.13 = Gauge32: 97820000 bits/second
VDSL2-LINE-MIB::xdsl2LineStatusAttainableRateDs.14 = Gauge32: 86353000 bits/second
VDSL2-LINE-MIB::xdsl2LineStatusAttainableRateUs.13 = Gauge32: 37302000 bits/second
VDSL2-LINE-MIB::xdsl2LineStatusAttainableRateUs.14 = Gauge32: 27095000 bits/second
VDSL2-LINE-MIB::xdsl2LineStatusActAtpDs.13 = INTEGER: 144 0.1 dBm
VDSL2-LINE-MIB::xdsl2LineStatusActAtpDs.14 = INTEGER: 144 0.1 dBm
VDSL2-LINE-MIB::xdsl2LineStatusActAtpUs.13 = INTEGER: 79 0.1 dBm
VDSL2-LINE-MIB::xdsl2LineStatusActAtpUs.14 = INTEGER: 79 0.1 dBm
VDSL2-LINE-MIB::xdsl2LineStatusActProfile.13 = BITS: 02 00 profile17a(6)
VDSL2-LINE-MIB::xdsl2LineStatusActProfile.14 = BITS: 02 00 profile17a(6)
VDSL2-LINE-MIB::xdsl2ChStatusActDataRate.13.xtuc = Gauge32: 37302000 bits/second
VDSL2-LINE-MIB::xdsl2ChStatusActDataRate.13.xtur = Gauge32: 91054000 bits/second
VDSL2-LINE-MIB::xdsl2ChStatusActDataRate.14.xtuc = Gauge32: 26790000 bits/second
VDSL2-LINE-MIB::xdsl2ChStatusActDataRate.14.xtur = Gauge32: 78438000 bits/second
VDSL2-LINE-MIB::xdsl2LInvG994VendorId.13.xtur = STRING: "BDCM"
VDSL2-LINE-MIB::xdsl2LInvG994VendorId.14.xtur = STRING: "BDCM"
VDSL2-LINE-MIB::xdsl2LInvSystemVendorId.13.xtur = Hex-STRING: 21 B2
VDSL2-LINE-MIB::xdsl2LInvSystemVendorId.14.xtur = Hex-STRING: 21 B2
VDSL2-LINE-MIB::xdsl2LInvVersionNumber.13.xtur = Hex-STRING: 21 B2
VDSL2-LINE-MIB::xdsl2LInvVersionNumber.14.xtur = Hex-STRING: 21 B2
VDSL2-LINE-MIB::xdsl2LInvSerialNumber.13.xtur = STRING: "AAXXXXXXXXX-12"
VDSL2-LINE-MIB::xdsl2LInvSerialNumber.14.xtur = STRING: "AAXXXXXXXXX-12"
```

## Drawbacks / Deficiencies / TODOs
- [ ] **Avoiding hardcoded `ifIndex` values.**
The module currently assumes fixed `ifIndex` values for `ptm0` and
`ptm0.2`, based on the observed behavior of this modem.
Ideally, this should be determined dynamically by inspecting the
interface table and matching names — even though the most appropriate
one may not necessarily exist.
  * LibreNMS (and likely others) expects an interface with `ifType`
set to `vdsl(97)` or `adsl(94)`. This can be overridden in
`snmpd.conf`, for example:
    ```conf
    interface ptm0 97 300000000
    interface ptm0.2 97 300000000
    ```
- [ ] **Limited DSL type coverage.** 
  The parsing logic has only been tested with VDSL2. Other xDSL types may produce different output, and the module might not parse them correctly.
- [ ] The module, and parsing, has not been tested with a bad
xDSL connection or an xDSL connection in an errored state.
Output may be unexpected and should not be relied upon solely
for diagnostics.
- [ ] There may be duplicate code with regards to SNMP caching,
as I am not familiar with the caching logic in the templates made
by `mib2c`.