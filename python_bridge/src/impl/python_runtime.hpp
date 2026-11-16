#pragma once

namespace libtokamap::python
{

/**
 * Make sure a Python interpreter is running in this process, starting one if needed. Thread safe.
 *
 * If Python is already initialised (e.g. the host is a Python process) the bridge attaches to it. Otherwise the
 * interpreter is started with signal handlers disabled, and the GIL is released before returning so any thread can
 * acquire it. The interpreter is never finalised by the bridge.
 *
 * The interpreter's executable, and therefore its virtual environment, is taken from LIBTOKAMAP_PYTHON_EXECUTABLE,
 * or failing that from $VIRTUAL_ENV/bin/python, or failing that from the location of the loaded libpython.
 *
 * Throws libtokamap::ConfigurationError if the interpreter cannot be started.
 */
void ensure_interpreter();

} // namespace libtokamap::python
