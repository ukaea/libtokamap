# Python Data Sources

LibTokaMap can load data sources written in Python into a C++ application through the optional Python bridge. The
application needs no Python-specific code: the bridge is an ordinary data source factory library, configured in the
same `data_source_factories` and `data_sources` sections as any other.

The core `libtokamap` library never depends on Python. Only processes that load the bridge do.

Python applications using the `libtokamap` Python package can keep registering data source objects directly with
`Mapper.register_python_data_source`; the bridge is for processes that C++ owns.

## Building

The bridge is off by default. Enable it with:

```bash
cmake -B build -DLIBTOKAMAP_BUILD_PYTHON_BRIDGE=ON -DPython3_EXECUTABLE=/path/to/venv/bin/python
cmake --build build
cmake --install build --prefix /opt/libtokamap
```

The bridge is built for the Python version of `Python3_EXECUTABLE` (3.11 or newer) and must be used with that same
`major.minor` version at runtime. Two libraries are installed side by side into `lib/`, and must stay together:

| Library | Purpose |
|---|---|
| `libtokamap_python_bridge` | The data source factory referenced from configuration. Does not depend on Python. |
| `libtokamap_python_bridge_impl` | The Python implementation, loaded by the factory library. |

CMake consumers can locate the factory library with the `LibTokaMap::python_bridge` imported target, e.g.
`$<TARGET_FILE:LibTokaMap::python_bridge>`.

Building on Windows is not yet supported. Free-threaded Python builds (e.g. 3.13t) are not supported.

## Writing a data source

A Python data source is any class with a `get` method. Inheriting from `libtokamap.DataSource` is not required.

```python
# my_package/my_datasource.py
import numpy as np

class MyDataSource:
    def __init__(self, server, port, options=None):
        self.client = connect(server, port)

    def get(self, args):
        # args holds the mapping's arguments merged with the request's runtime attributes,
        # e.g. {"signal": "ip", "shot": 42}
        return np.asarray(self.client.read(args["signal"], args["shot"]))

    def close(self):
        # Optional: called when LibTokaMap destroys the data source
        self.client.close()
```

`get` may return:

- a NumPy array or scalar, or any other object supporting the buffer protocol (e.g. `memoryview`, `array.array`),
  with a boolean, signed or unsigned 8–64 bit integer, `float32` or `float64` element type. Arrays may be
  non-contiguous, read-only or non-native byte order; the data is always copied. Booleans are returned as `uint8`;
- a `str`, or a NumPy `U`/`S` array with exactly one element, returned as a string;
- a NumPy object array (e.g. `np.array(list_of_dicts)`), returned as a zero-filled `int8` placeholder of the same
  shape. This lets `DIMENSION` mappings count the elements, but the elements themselves cannot be read.

Anything else, including complex numbers, multi-element string arrays and `None`, raises
`libtokamap::PythonCallbackError`, as does any exception raised by `get`. The error message includes the Python
exception type, message and the location it was raised from.

These rules are the same for data sources registered from Python with `Mapper.register_python_data_source`, and for
Python custom functions, except that custom functions may not return object arrays.

## Configuration

```toml
[data_source_factories]
python = "/opt/libtokamap/lib/libtokamap_python_bridge.so"

[data_sources.MY_SOURCE]
factory = "python"

[data_sources.MY_SOURCE.args]
module = "my_package.my_datasource"
class_name = "MyDataSource"
server = "data.example.org"
port = 1234
options = { retries = 3, hosts = ["a", "b"] }
```

`module` and `class_name` are required. All other arguments are passed to the class as keyword arguments; tables and
arrays become `dict` and `list`. Each `data_sources` entry creates a separate instance, and any number of them can use
the same factory.

Failures importing the module, finding the class or constructing it are raised from `MappingHandler::init` as
`libtokamap::ConfigurationError`.

## Runtime environment

All Python data sources in a process share one interpreter. The bridge chooses it as follows:

1. **Python already running in the process** (e.g. a Python application or a C++ extension module calling into
   LibTokaMap): the bridge uses that interpreter.
2. **Otherwise** the bridge loads `libpython` and starts an interpreter:
    - the library is the one found at build time, or the path in `LIBTOKAMAP_PYTHON_LIBRARY`. A shared libpython is
      required: pyenv users need to build Python with `--enable-shared`;
    - packages come from the environment of the executable in `LIBTOKAMAP_PYTHON_EXECUTABLE` or, if that is unset,
      `$VIRTUAL_ENV/bin/python`. Set one of these to use a virtual environment;
    - `PYTHONPATH` and `PYTHONHOME` are honoured as normal.

The interpreter the bridge starts:

- does not install signal handlers, so the host application keeps control of `SIGINT` etc.;
- is never finalised, so Python `atexit` handlers do not run. Use a `close()` method to release resources;
- is started on first use. Server applications that fork should create their `MappingHandler` after the final fork.

| Environment variable | Purpose |
|---|---|
| `LIBTOKAMAP_PYTHON_EXECUTABLE` | Python executable whose environment (e.g. virtual environment) the interpreter uses. |
| `VIRTUAL_ENV` | Used if `LIBTOKAMAP_PYTHON_EXECUTABLE` is unset. |
| `LIBTOKAMAP_PYTHON_LIBRARY` | Overrides the libpython loaded when no Python is running in the process. |
| `LIBTOKAMAP_PYTHON_BRIDGE_IMPL` | Overrides the location of `libtokamap_python_bridge_impl`. |

## Threads

Data sources can be called from any thread. Every call takes the Python global interpreter lock (GIL), so calls into
Python data sources run one at a time across the whole process. Note that `MappingHandler` itself is not thread safe;
use one handler per thread.

## Troubleshooting

- **`ModuleNotFoundError` for your module or for NumPy**: the interpreter is not using the environment you expect.
  Set `LIBTOKAMAP_PYTHON_EXECUTABLE` to the virtual environment's `python`, or add the module's directory to
  `PYTHONPATH`.
- **"failed to load the Python X.Y shared library"**: no shared libpython was found. Set
  `LIBTOKAMAP_PYTHON_LIBRARY`.
- **"Python bridge was built for Python X.Y but the process has Python A.B"**: rebuild the bridge with
  `Python3_EXECUTABLE` set to the Python version used at runtime.
- **"was built for factory ABI version"**: the bridge was built against a different LibTokaMap version than the
  application. Rebuild it from the same LibTokaMap version.
