"""Python wrapper around the C harness for real-hardware Z-Wave tests."""

from __future__ import annotations

import time
from ctypes import (
    c_bool,
    c_char_p,
    c_int,
    c_uint8,
    create_string_buffer,
    POINTER,
    byref,
)
from dataclasses import dataclass
from enum import IntFlag
from typing import Optional

from zwave_hw_test._native import lib


class TransmitOptions(IntFlag):
    ACK = 0x01
    LOW_POWER = 0x02
    AUTO_ROUTE = 0x04
    NO_ROUTE = 0x10
    EXPLORE = 0x20
    MULTICAST = 0x40


@dataclass
class AppFrame:
    data: bytes
    rx_status: int
    source_node: int


class ZwaveHarness:
    """Direct zwapi access for automation without UCL."""

    def __init__(self, port: str, poll_timeout_s: float = 30.0, log_level: int = 2):
        self._poll_timeout_s = poll_timeout_s
        self._handle = None
        self._open(port, log_level)

    def _open(self, port: str, log_level: int) -> None:
        lib().zwave_harness_set_log_level(c_int(log_level))
        handle = lib().zwave_harness_open(c_char_p(port.encode("utf-8")))
        if not handle:
            raise RuntimeError(
                f"failed to open {port}: check NCP connection and permissions"
            )
        self._handle = handle

    def close(self) -> None:
        if self._handle is not None:
            lib().zwave_harness_close(self._handle)
            self._handle = None

    def __enter__(self) -> ZwaveHarness:
        return self

    def __exit__(self, *_) -> None:
        self.close()

    def _last_error(self) -> str:
        if self._handle is None:
            return "harness closed"
        return lib().zwave_harness_last_error(self._handle).decode("utf-8", errors="replace")

    def poll(self) -> bool:
        return bool(lib().zwave_harness_poll(self._handle))

    def poll_until_tx_complete(self, timeout_s: Optional[float] = None) -> int:
        deadline = time.monotonic() + (timeout_s or self._poll_timeout_s)
        while time.monotonic() < deadline:
            self.poll()
            try:
                return self.get_last_tx_status()
            except RuntimeError:
                time.sleep(0.01)
        raise TimeoutError(f"transmit did not complete within {timeout_s or self._poll_timeout_s}s")

    def poll_until_app_frame(self, timeout_s: Optional[float] = None) -> AppFrame:
        deadline = time.monotonic() + (timeout_s or self._poll_timeout_s)
        while time.monotonic() < deadline:
            self.poll()
            try:
                return self.get_last_app_frame()
            except RuntimeError:
                time.sleep(0.01)
        raise TimeoutError(f"no application frame within {timeout_s or self._poll_timeout_s}s")

    def get_version(self) -> tuple[int, int]:
        major = c_uint8()
        minor = c_uint8()
        rc = lib().zwave_harness_get_version(self._handle, byref(major), byref(minor))
        if rc != 0:
            raise RuntimeError(self._last_error())
        return major.value, minor.value

    def get_protocol_version_string(self) -> str:
        buf = create_string_buffer(128)
        rc = lib().zwave_harness_get_protocol_version_string(
            self._handle, buf, c_int(len(buf))
        )
        if rc != 0:
            raise RuntimeError(self._last_error())
        return buf.value.decode("utf-8")

    def get_library_type(self) -> int:
        out = c_uint8()
        rc = lib().zwave_harness_get_library_type(self._handle, byref(out))
        if rc != 0:
            raise RuntimeError(self._last_error())
        return out.value

    def send_frame(self, func_id: int, payload: bytes = b"") -> bytes:
        out = (c_uint8 * 255)()
        out_len = c_uint8(255)
        payload_buf = (c_uint8 * len(payload))(*payload) if payload else None
        rc = lib().zwave_harness_send_frame(
            self._handle,
            c_uint8(func_id),
            payload_buf,
            c_uint8(len(payload)),
            out,
            byref(out_len),
        )
        if rc != 0:
            raise RuntimeError(self._last_error())
        return bytes(out[:out_len.value])

    def send_nop(self, node_id: int, tx_options: TransmitOptions = TransmitOptions.ACK) -> None:
        rc = lib().zwave_harness_send_nop(
            self._handle, c_uint8(node_id), c_uint8(int(tx_options))
        )
        if rc != 0:
            raise RuntimeError(self._last_error())

    def send_data(
        self,
        node_id: int,
        data: bytes,
        tx_options: TransmitOptions = TransmitOptions.ACK | TransmitOptions.AUTO_ROUTE,
    ) -> None:
        buf = (c_uint8 * len(data))(*data)
        rc = lib().zwave_harness_send_data(
            self._handle,
            c_uint8(node_id),
            buf,
            c_uint8(len(data)),
            c_uint8(int(tx_options)),
        )
        if rc != 0:
            raise RuntimeError(self._last_error())

    def get_last_app_frame(self) -> AppFrame:
        frame = (c_uint8 * 255)()
        frame_len = c_uint8(255)
        rx_status = c_uint8()
        source = c_uint8()
        rc = lib().zwave_harness_get_last_app_frame(
            self._handle,
            frame,
            byref(frame_len),
            byref(rx_status),
            byref(source),
        )
        if rc != 0:
            raise RuntimeError(self._last_error())
        return AppFrame(
            data=bytes(frame[:frame_len.value]),
            rx_status=rx_status.value,
            source_node=source.value,
        )

    def get_last_tx_status(self) -> int:
        status = c_uint8()
        rc = lib().zwave_harness_get_last_tx_status(self._handle, byref(status))
        if rc != 0:
            raise RuntimeError(self._last_error())
        return status.value

    def version_get(self) -> bytes:
        """FUNC_ID_ZW_GET_VERSION (0x15) — raw Serial API."""
        return self.send_frame(0x15)

    def soft_reset(self) -> None:
        """FUNC_ID_SERIAL_API_SOFT_RESET (0x08)."""
        self.send_frame(0x08)
