"""Check the Python bridge attaches to the interpreter of a Python host process.

Usage: python test_attach.py <attach driver library> <bridge library>
"""

import ctypes
import sys

import bridge_test_sources


def main() -> int:
    driver_path, bridge_path = sys.argv[1:3]

    # ctypes.CDLL releases the GIL for the duration of the call, as a C++ extension calling into LibTokaMap should
    driver = ctypes.CDLL(driver_path)
    driver.run_bridge_attach_check.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]
    driver.run_bridge_attach_check.restype = ctypes.c_int

    message = ctypes.create_string_buffer(4096)
    if driver.run_bridge_attach_check(bridge_path.encode(), message, len(message)) != 0:
        print(f"bridge attach check failed: {message.value.decode()}", file=sys.stderr)
        return 1

    # The data source must have run in this interpreter, using the module object this process already imported
    if bridge_test_sources.ATTACH_CALLS != [{"caller": "attach_driver"}]:
        print(f"bridge did not use the host interpreter: {bridge_test_sources.ATTACH_CALLS}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
