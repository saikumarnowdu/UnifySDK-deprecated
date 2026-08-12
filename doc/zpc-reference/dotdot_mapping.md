# DotDot Mapping {#zpc_dotdot_mapping}

ZPC translates between Z-Wave Command Classes and the DotDot/UCL attribute model
using **UAM (Unify Attribute Mapper)** rule files.

## Rule Location

```
applications/zpc/components/dotdot_mapper/rules/
```

Rules are installed with the `uic-zpc` Debian package to:

```
/usr/share/uic/rules/
```

## Rule Categories

### Cluster Mappings

Map DotDot clusters to Z-Wave Command Classes:

| Rule file | Mapping |
|-----------|---------|
| `OnOff_to_BinarySwitchCC.uam` | OnOff cluster → Binary Switch CC |
| `OnOff_to_MultilevelSwitchCC.uam` | OnOff cluster → Multilevel Switch CC |
| `OnOff_to_BasicCC.uam` | OnOff cluster → Basic CC |
| `Level_to_MultilevelSwitchCC.uam` | Level cluster → Multilevel Switch CC |
| `Level_to_SoundSwitchCC.uam` | Level cluster → Sound Switch CC |
| `DoorLock_to_DoorLockCC.uam` | DoorLock cluster → Door Lock CC |
| `ColorControl_to_ColorSwitchCC.uam` | ColorControl → Color Switch CC |
| `Thermostat.uam` | Thermostat cluster → Thermostat CCs |
| `Metering_to_Meter.uam` | Metering cluster → Meter CC |
| `PowerConfiguration_to_BatteryCC.uam` | PowerConfiguration → Battery CC |
| `Identify_to_IndicatorCC.uam` | Identify → Indicator CC |
| `BarrierControl_to_Barrier_Operator.uam` | BarrierControl → Barrier Operator CC |

### Sensor Mappings

Map measurement clusters to Multilevel Sensor CC:

- `TemperatureMeasurement_to_MultilevelSensorCC.uam`
- `RelativeHumidity_to_MultilevelSensorCC.uam`
- `IlluminanceMeasurement_to_MultilevelSensorCC.uam`
- `PressureMeasurement_to_MultilevelSensorCC.uam`
- `CarbonMonoxideMeasurement_to_MultilevelSensorCC.uam`
- `CarbonDioxideMeasurement_to_MultilevelSensorCC.uam`
- `SoilMoisture_to_MultilevelSensorCC.uam`
- `pHMeasurement_to_MultilevelSensorCC.uam`

### IAS Zone Mappings

Map IAS Zone clusters to notification and sensor command classes:

- `IasZone_MotionSensor_Binary_Sensor.uam`
- `IasZone_MotionSensor_Notification_HomeSecurity.uam`
- `IasZone_FireSensor_Notification_SmokeAlarm.uam`
- `IasZone_Water_Sensor_Notification_Water_Sensor.uam`
- `IasZone_CO_Sensor_Notification_CO_Alarm.uam`
- `IasZone_DoorWindow_Handle_V1_Alarm_Type.uam`

### Occupancy and Illuminance

- `OccupancySensing_to_BinarySensorCC.uam`
- `OccupancySensing_to_NotificationCC.uam`
- `IlluminanceLevelSensing_to_NotificationCC.uam`

### Simulation Rules

Simulate DotDot clusters that the ZPC serves locally:

- `OnOff_cluster_simulation.uam`
- `Level_cluster_simulation.uam`

### Device Quirks

Device-specific workarounds for known interoperability issues:

| Rule file | Device / Issue |
|-----------|----------------|
| `Quirks_aeotec_multisensor_7.uam` | Aeotec MultiSensor 7 |
| `Quirks_aeotec_switch_configuration_parameters.uam` | Aeotec switch config |
| `Quirks_hank_rgb_light_bulb_configuration_parameters.uam` | Hank RGB bulb |
| `Quirks_mh3900_thermostat.uam` | MH3900 thermostat |
| `Quirks_thermostat_setpoint_capabilities.uam` | Thermostat setpoint caps |
| `Quirks_indicator_capabilities.uam` | Indicator capabilities |
| `Quirks_set_wake_up_interval.uam` | Wake-up interval |
| `Quirks_force_estalish_lifeline.uam` | Force lifeline association |
| `Quirks_ZDB5100_logic_group_matrix.uam` | ZDB5100 logic groups |
| `Quirks_ZRB5120_logic_group_matrix.uam` | ZRB5120 logic groups |
| `Quirks_agi_data.uam` | AGI data handling |

### Utility Rules

- `ForceReadAttributes_links.uam` — force attribute reads
- `Basic.uam`, `Level.uam`, `State.uam`, `Humidity.uam`, `Scenes.uam`

## Mapper Components

| Library | Role |
|---------|------|
| `dotdot_mapper` | Core cluster mappers (OnOff, Basic, Binding) |
| `dotdot_mapper_binding_cluster_helper` | Binding cluster support |
| `zpc_attribute_mapper` | Initializes the UAM engine with ZPC rules |

## Data Flow

```
DotDot MQTT topic
       ↓
zpc_dotdot_mqtt / uic_mqtt_dotdot
       ↓
dotdot_mapper (UAM rules)
       ↓
attribute_store (desired/reported attributes)
       ↓
zpc_attribute_resolver
       ↓
zwave_command_classes → zwave_tx → NCP
```

## Related Pages

- [Core Components](@ref zpc_components)
- [Architecture](@ref zpc_architecture)
- Unify SDK: `doc/how_to_write_uam_files.rst`
