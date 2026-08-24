# Command Classes {#zpc_command_classes}

ZPC implements Z-Wave Command Class handlers under
`applications/zpc/components/zwave_command_classes/`.

## Handler Generation

Many handlers are **auto-generated** from `assets/ZWave_custom_cmd_classes.xml` using
`scripts/generator.py`. Generated artifacts include:

- `zwave_<cc>_handlers.cpp` / `.h` — frame parsers and builders
- `zwave_<cc>_attribute_id.h` — attribute ID definitions
- `zwave_<cc>_attribute_id.uam` — attribute mapper rules

### Generator Usage

```bash
cd applications/zpc/components/zwave_command_classes/scripts
pip install -r requirements.txt
python3 generator.py -h
```

Example:

```bash
python3 generator.py \
  -x ../assets/ZWave_custom_cmd_classes.xml \
  -t ../templates/zwave_{{_name}}_handlers.cpp \
  -t ../templates/zwave_{{_name}}_handlers.h \
  -t ../templates/zwave_{{_name}}_attribute_id.h \
  -t ../templates/zwave_{{_name}}_attribute_id.uam \
  -m ../assets/modificators.json \
  -o generated/src
```

## Implemented Command Classes

The following command classes had source implementations in `ver_1.6.0`:

| Command Class | Source file prefix |
|---------------|-------------------|
| AGI | `zwave_command_class_agi` |
| Alarm Sensor | `zwave_command_class_alarm_sensor` |
| Association | `zwave_command_class_association` |
| Barrier Operator | `zwave_command_class_barrier_operator` |
| Basic | `zwave_command_class_basic` |
| Battery | `zwave_command_class_battery` |
| Binary Switch | `zwave_command_class_binary_switch` |
| Central Scene | `zwave_command_class_central_scene` |
| Configuration | `zwave_command_class_configuration_control` |
| Device Reset Locally | `zwave_command_class_device_reset_locally` |
| Door Lock | `zwave_command_class_door_lock_control` |
| Firmware Update | `zwave_command_class_firmware_update` |
| Humidity Control | `zwave_command_class_humidity_control_*` |
| Inclusion Controller | `zwave_command_class_inclusion_controller` |
| Indicator | `zwave_command_class_indicator` |
| Manufacturer Specific | `zwave_command_class_manufacturer_specific` |
| Meter | `zwave_command_class_meter_control` |
| Multi Channel | `zwave_command_class_multi_channel` |
| Multi Channel Association | `zwave_command_class_multi_channel_association` |
| Multi Command | `zwave_command_class_multi_command` |
| Multilevel Sensor | `zwave_command_class_multilevel_sensor` |
| Multilevel Switch | `zwave_command_class_switch_multilevel` |
| Notification | `zwave_command_class_notification` |
| Powerlevel | `zwave_command_class_powerlevel` |
| Scene Activation | `zwave_command_class_scene_activation_control` |
| Security 0 | `zwave_command_class_security_0` |
| Security 2 | `zwave_command_class_security_2` |
| Sound Switch | `zwave_command_class_sound_switch` |
| Supervision | `zwave_command_class_supervision` |
| Switch Color | `zwave_command_class_switch_color` |
| Thermostat (mode, fan, setpoint, operating state) | `zwave_command_class_thermostat_*` |
| Time | `zwave_command_class_time` |
| Transport Service | `zwave_command_class_transport_service` |
| User Code | `zwave_command_class_user_code` |
| Version | `zwave_command_class_version` |
| Wake Up | `zwave_command_class_wake_up` |
| Z-Wave Plus Info | `zwave_command_class_zwave_plus_info` |

## Rust Command Classes

Newer command classes are implemented in Rust under `zpc_rust/`:

- `zwave_command_class_firmware_update`
- `zwave_command_class_switch_color`
- Additional handlers via `zwave_rust_proc_macros`

Rust handlers are registered in `main.c` via:

- `zwave_command_class_init_rust_handlers()`
- `zwave_command_class_init_rust_handlers_legacy()`

## Command Handler Framework

`zwave_command_handler` dispatches incoming frames to the appropriate command class
handler. It is initialized **after** all command classes are registered.

## Excluded Command Classes

The generator `modificators.json` excludes certain command classes from auto-generation
(e.g. Security, Supervision, Network Management Inclusion, Firmware Update MD) because
they are handled by dedicated components.

## Related Pages

- [DotDot Mapping](@ref zpc_dotdot_mapping)
- [Architecture](@ref zpc_architecture)
- [Tools and Utilities](@ref zpc_tools)
