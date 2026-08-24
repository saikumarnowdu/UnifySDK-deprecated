# Z-Wave (ZPC)

The protocol controller that implements Z-Wave functionality is ZPC.

```{toctree}
---
maxdepth: 2
hidden:
---
../../../applications/zpc/readme_user.md
../../../applications/zpc/readme_certification.md
../../readme_debug.md
../../../applications/zpc/how_to_implement_zwave_command_classes.rst
../../../applications/zpc/how_to_write_uam_files_for_the_zpc.md
../../../applications/zpc/how_to_interact_with_clusters.rst
../../../applications/zpc/doc/supported_command_classes.md
```

The [ZPC User Guide](../../../applications/zpc/readme_user.md) explains how to use and configure ZPC.

The [ZPC Z-Wave Certification](../../../applications/zpc/readme_certification.md) page has information about obtaining Z-Wave certification with a Unify Z-Wave gateway.

The [ZPC Debugging Guide](../../readme_debug.md) has tips for debugging a Z-Wave network with Unify.

The guide [How to implement new Z-Wave command classes](../../../applications/zpc/how_to_implement_zwave_command_classes.rst) explains how to add support for additional Z-Wave command classes to support more Z-Wave devices.

The guide [How to write UAM files for ZPC](../../../applications/zpc/how_to_write_uam_files_for_the_zpc.md) goes into detail about how the attribute mapper can be used to map Z-Wave command classes to UCL MQTT messages.

The guide [How to interact with clusters](../../../applications/zpc/how_to_interact_with_clusters.rst) goes into detail about the implementation of Cluster in Unify.

The guide [Supported Command Classes](../../../applications/zpc/doc/supported_command_classes.md) goes into detail about how the command class are implemented. This documents gives specifics about the attributes store and MQTT topics that can interact with the class.

The guide [MQTT to Serial Code Flow](../../../applications/zpc/doc/mqtt_to_serial_flow.md) traces the outbound path from an MQTT command through attribute resolution to bytes on the NCP serial port, including state machine diagrams and a step-by-step trace for `OnOff/Commands/On`.

The guide [Serial to MQTT Code Flow](../../../applications/zpc/doc/serial_to_mqtt_flow.md) traces the inbound path from NCP serial RX through transport decapsulation and attribute mapping to an MQTT `OnOff/Attributes/OnOff/Reported` publish, including state machine diagrams and a step-by-step trace for `SWITCH_BINARY_REPORT`.

The doxygen generated <a href="../../../doxygen_zpc/index.html">ZPC API</a> 