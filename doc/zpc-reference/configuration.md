# Configuration {#zpc_configuration}

ZPC configuration is managed by `zpc_config` (`applications/zpc/components/zpc_config/`).
Parameters come from command-line arguments and the Unify config file (`uic.cfg`).

## Configuration Structure

Defined in `zpc_config.h` as `zpc_config_t`:

### MQTT Settings

| Parameter | Type | Description |
|-----------|------|-------------|
| `mqtt_host` | `const char *` | MQTT broker hostname |
| `mqtt_port` | `int` | MQTT broker port |
| `mqtt_cafile` | `const char *` | PEM-encoded trusted CA certificates |
| `mqtt_certfile` | `const char *` | PEM-encoded client certificate |
| `mqtt_keyfile` | `const char *` | PEM-encoded private key |
| `mqtt_client_id` | `const char *` | MQTT client ID for TLS |
| `mqtt_client_psk` | `const char *` | Pre-shared key for TLS |

### Z-Wave Hardware

| Parameter | Type | Description |
|-----------|------|-------------|
| `serial_port` | `const char *` | Serial device for the Z-Wave NCP |
| `serial_log_file` | `const char *` | Optional serial log output file |
| `zwave_rf_region` | `const char *` | RF region: EU, US, ANZ, HK, IN, IL, RU, CN, JP, KR |
| `zwave_normal_tx_power_dbm` | `int` | Normal TX power (dBm) |
| `zwave_measured_0dbm_power` | `int` | Measured 0 dBm output power |
| `zwave_max_lr_tx_power_dbm` | `int` | Max Long Range TX power (dBm) |

### Persistence

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `datastore_file` | `const char *` | `/var/lib/uic/zpc.db` | SQLite database path |

Config key: `zpc.datastore_file` (macro `CONFIG_KEY_ZPC_DATASTORE_FILE`).

### Node Identity

| Parameter | Type | Description |
|-----------|------|-------------|
| `manufacturer_id` | `uint16_t` | Z-Wave manufacturer ID |
| `product_type` | `uint16_t` | Product type |
| `product_id` | `uint16_t` | Product ID |
| `device_id` | `const char *` | Device identifier string |
| `hardware_version` | `int` | Hardware version |
| `zpc_basic_device_type` | `uint8_t` | Basic device type for NIF |
| `zpc_generic_device_type` | `uint8_t` | Generic device type for NIF |
| `zpc_specific_device_type` | `uint8_t` | Specific device type for NIF |

### Network Behavior

| Parameter | Type | Description |
|-----------|------|-------------|
| `default_wake_up_interval` | `int` | Default wake-up interval for sleeping nodes |
| `missing_wake_up_notification` | `uint8_t` | Max missing wake-up periods before node is considered offline |
| `accepted_transmit_failure` | `uint8_t` | Max accepted frame transmission failures |
| `inclusion_protocol_preference` | `const char *` | SmartStart protocol priority list |

Protocol representation values:

- `"1"` — Z-Wave (`ZWAVE_CONFIG_REPRESENTATION`)
- `"2"` — Z-Wave Long Range (`ZWAVE_LONG_RANGE_CONFIG_REPRESENTATION`)

### OTA

| Parameter | Type | Description |
|-----------|------|-------------|
| `ota_cache_path` | `const char *` | Writable path for OTA image cache |
| `ota_cache_size` | `int` | OTA cache size in KB |

### NCP Update

| Parameter | Type | Description |
|-----------|------|-------------|
| `ncp_version` | `bool` | Print NCP version and exit |
| `ncp_update_filename` | `const char *` | Flash NCP firmware from file and exit |

## Config File Example

In `uic.cfg`:

```yaml
zpc:
  - serial: /dev/ttyUSB0
  - rf_region: EU
  - datastore_file: /var/lib/uic/zpc.db
```

## Debian Package Configuration

The `uic-zpc` package uses `debconf` templates to prompt for serial port, RF region,
and datastore file during installation. See `applications/zpc/debconf/`.

## API

```c
/**
 * @brief Get the current configuration. Must only be called after zpc_config_init.
 */
const zpc_config_t *zpc_config_get(void);
```

## Related Pages

- [Build and Deployment](@ref zpc_build_deploy)
- [Architecture](@ref zpc_architecture)
