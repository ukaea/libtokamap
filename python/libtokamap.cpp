#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <numpy/arrayobject.h>
#include <numpy/ndarrayobject.h>
#include <numpy/ndarraytypes.h>
#include <numpy/npy_common.h>
#include <numpy/numpyconfig.h>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <libtokamap.hpp>

namespace
{
PyObject* LibTokaMapError = nullptr;

PyObject* libtokamap_create(PyObject* module, PyObject* args);
PyObject* libtokamap_register_data_source_factory(PyObject* module, PyObject* args);
PyObject* libtokamap_register_data_source(PyObject* module, PyObject* args);
PyObject* libtokamap_register_python_data_source(PyObject* module, PyObject* args);
PyObject* libtokamap_load_custom_function_library(PyObject* module, PyObject* args);
PyObject* libtokamap_register_custom_function(PyObject* module, PyObject* args);
PyObject* libtokamap_map(PyObject* module, PyObject* const* args, Py_ssize_t nargs);

PyMethodDef libtokamap_methods[] = {
    {"create", libtokamap_create, METH_O, "Create a new Mapper."},
    {"register_data_source_factory", libtokamap_register_data_source_factory, METH_VARARGS,
     "Register a DataSource factory."},
    {"register_data_source", libtokamap_register_data_source, METH_VARARGS, "Register a DataSource."},
    {"register_python_data_source", libtokamap_register_python_data_source, METH_VARARGS,
     "Register a Python DataSource."},
    {"load_custom_function_library", libtokamap_load_custom_function_library, METH_VARARGS,
     "Load a custom function library."},
    {"register_custom_function", libtokamap_register_custom_function, METH_VARARGS, "Register a custom function."},
    {"map", reinterpret_cast<PyCFunction>(libtokamap_map), METH_FASTCALL, "Map the given path to data."},
    {nullptr, nullptr, 0, nullptr} /* Sentinel */
};

libtokamap::TypedDataArray object_to_typed_data_array(PyObject* object)
{
    if (object == nullptr) {
        return {};
    }
    if (!PyArray_Check(object)) {
        PyErr_SetString(LibTokaMapError, "Expected a NumPy array");
        return {};
    }

    auto* array = reinterpret_cast<PyArrayObject*>(object);
    void* data = PyArray_DATA(array);
    npy_intp size = PyArray_SIZE(array);
    int rank = PyArray_NDIM(array);
    npy_intp* shape = PyArray_DIMS(array);
    int typenum = PyArray_TYPE(array);

    std::vector<size_t> shape_vec(shape, shape + rank);

    if (typenum == NPY_FLOAT64) {
        // TODO: bind lifetime of array to self and avoid memory copy
        return libtokamap::TypedDataArray(reinterpret_cast<double*>(data), static_cast<size_t>(size), shape_vec);
    }
    if (typenum == NPY_UNICODE) {
        npy_intp item_size = PyArray_ITEMSIZE(array);
        npy_intp num_chars = item_size / 4;
        PyObject* py_unicode = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, data, num_chars);
        if (py_unicode == nullptr) {
            PyErr_SetString(LibTokaMapError, "Failed to create Unicode object");
            return {};
        }
        Py_ssize_t size;
        const char* utf8_str = PyUnicode_AsUTF8AndSize(py_unicode, &size);
        if (utf8_str == nullptr) {
            PyErr_SetString(LibTokaMapError, "Failed to convert Unicode object to UTF-8");
            Py_DECREF(py_unicode);
            return {};
        }

        std::string string{utf8_str, static_cast<size_t>(size)};
        Py_DECREF(py_unicode);
        return libtokamap::TypedDataArray(string);
    }
    if (typenum == NPY_OBJECT) {
        // Return empty TypedDataArray<char> to allow for taking of dimensions
        std::vector<char> vec(size);
        return libtokamap::TypedDataArray(vec, shape_vec);
    }

    PyErr_SetString(LibTokaMapError, "Wrong data type");
    return {};
}

bool set_dictionary_item(PyObject* dict, const std::string& key, const nlohmann::json& value)
{
    PyObject* py_value = nullptr;
    if (value.is_string()) {
        py_value = PyUnicode_FromString(value.get<std::string>().c_str());
    } else if (value.is_number_integer()) {
        py_value = PyLong_FromLongLong(value.get<int64_t>());
    } else if (value.is_number_float()) {
        py_value = PyFloat_FromDouble(value.get<double>());
    } else {
        std::string msg = "Value '" + key + "' has an invalid type";
        PyErr_SetString(LibTokaMapError, msg.c_str());
        return false;
    }
    PyDict_SetItemString(dict, key.c_str(), py_value);
    Py_DECREF(py_value);
    return true;
}

PyArray_Descr* get_s1_descr()
{
    // Create a Python dtype object for 'S1'
    PyObject* dtype_obj = PyUnicode_FromString("S1");
    if (!dtype_obj) {
        return nullptr;
    }

    PyArray_Descr* descr = nullptr;
    if (PyArray_DescrConverter(dtype_obj, &descr) == NPY_FAIL) {
        Py_DECREF(dtype_obj);
        return nullptr;
    }

    Py_DECREF(dtype_obj);
    return descr;
}

void free_memory(PyObject* capsule)
{
    void* ptr = PyCapsule_GetPointer(capsule, nullptr);
    free(ptr);
}

PyObject* wrap_array(const std::vector<npy_intp>& dims, int npy_type, libtokamap::TypedDataArray& data)
{
    PyArray_Descr* descr = nullptr;
    if (npy_type == NPY_BYTE) {
        descr = get_s1_descr();
    } else {
        descr = PyArray_DescrFromType(npy_type);
    }

    bool is_owning = data.is_owning();

    // Create the array
    const int ndim = static_cast<int>(dims.size());
    void* data_ptr = data.release();
    constexpr npy_intp* strides = nullptr;
    constexpr int flags = NPY_ARRAY_CARRAY;
    PyObject* array = PyArray_NewFromDescr(&PyArray_Type, descr, ndim, dims.data(), strides, data_ptr, flags, nullptr);
    if (array == nullptr) {
        return nullptr;
    }

    // Attach a Python object that will free the memory
    PyObject* capsule = PyCapsule_New(data_ptr, nullptr, is_owning ? free_memory : nullptr);
    if (capsule == nullptr) {
        Py_DECREF(array);
        return nullptr;
    }

    PyArray_SetBaseObject(reinterpret_cast<PyArrayObject*>(array), capsule);

    return array;
}

PyObject* array_to_numpy(libtokamap::TypedDataArray& array)
{
    auto type = array.data_type();
    const auto& shape = array.shape();
    std::vector<npy_intp> dims(shape.size());
    std::ranges::copy(shape, dims.begin());
    using libtokamap::DataType;
    switch (type) {
        case DataType::Double:
            return wrap_array(dims, NPY_FLOAT64, array);
        case DataType::Float:
            return wrap_array(dims, NPY_FLOAT32, array);
        case DataType::Int64:
            return wrap_array(dims, NPY_INT64, array);
        case DataType::Int32:
            return wrap_array(dims, NPY_INT32, array);
        case DataType::Int16:
            return wrap_array(dims, NPY_INT16, array);
        case DataType::Int8:
            return wrap_array(dims, NPY_INT8, array);
        case DataType::UInt64:
            return wrap_array(dims, NPY_UINT64, array);
        case DataType::UInt32:
            return wrap_array(dims, NPY_UINT32, array);
        case DataType::UInt16:
            return wrap_array(dims, NPY_UINT16, array);
        case DataType::UInt8:
            return wrap_array(dims, NPY_UINT8, array);
        case DataType::Unknown:
            return nullptr;
    }
    return nullptr;
}

bool set_dictionary_item(PyObject* dict, const std::string& key, libtokamap::TypedDataArray& value)
{
    PyObject* py_array = array_to_numpy(value);
    if (py_array == nullptr) {
        return false;
    }
    PyDict_SetItemString(dict, key.c_str(), py_array);
    Py_DECREF(py_array);
    return true;
}

class PythonDataSource : public libtokamap::DataSource
{
  public:
    explicit PythonDataSource(PyObject* py_data_source) : _py_data_source{py_data_source}
    {
        Py_XINCREF(py_data_source);
    }

    ~PythonDataSource() override { Py_XDECREF(_py_data_source); }

    PythonDataSource(PythonDataSource&) = delete;
    PythonDataSource(PythonDataSource&&) = delete;
    PythonDataSource& operator=(PythonDataSource&) = delete;
    PythonDataSource& operator=(PythonDataSource&&) = delete;

    libtokamap::TypedDataArray get(const libtokamap::DataSourceArgs& map_args,
                                   const libtokamap::MapArguments& Py_UNUSED(arguments),
                                   libtokamap::RamCache* Py_UNUSED(ram_cache)) override
    {
        PyObject* kwargs = PyDict_New();
        if (kwargs == nullptr) {
            PyErr_SetString(LibTokaMapError, "Failed to create dictionary");
            return {};
        }

        for (const auto& [key, value] : map_args) {
            if (!set_dictionary_item(kwargs, key, value)) {
                return {};
            }
        }

        PyObject* result = PyObject_CallMethod(_py_data_source, "get", "O", kwargs);
        return object_to_typed_data_array(result);
    }

  private:
    PyObject* _py_data_source;
};

std::optional<std::string> to_string(PyObject* object)
{
    if (!PyUnicode_Check(object)) {
        PyErr_SetString(LibTokaMapError, "Expected a string");
        return {};
    }

    // const char* c_path = PyUnicode_AsUTF8(path);
    Py_ssize_t len = 0;
    const char* c_path = PyUnicode_AsUTF8AndSize(object, &len);
    if (c_path == nullptr) {
        PyErr_SetString(LibTokaMapError, "Failed to decode path");
        return {};
    }

    return {std::string{c_path, static_cast<size_t>(len)}};
}

struct PyMapper {
    PyObject_HEAD;
    libtokamap::MappingHandler* cpp_mapper;
};

void PyMapper_dealloc(PyMapper* self)
{
    delete self->cpp_mapper;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* PyMapper_new(PyTypeObject* type, PyObject* Py_UNUSED(args), PyObject* Py_UNUSED(kwds))
{
    auto* self = reinterpret_cast<PyMapper*>(type->tp_alloc(type, 0));
    if (self != nullptr) {
        self->cpp_mapper = nullptr;
    }
    return reinterpret_cast<PyObject*>(self);
}

int PyMapper_init(PyMapper* self, PyObject* args, PyObject* Py_UNUSED(kwds))
{
    char* mapping_directory = nullptr;
    if (!PyArg_ParseTuple(args, "s", &mapping_directory)) {
        return -1;
    }

    try {
        self->cpp_mapper = new libtokamap::MappingHandler();

        auto root = std::filesystem::path{__FILE__};
        root = std::filesystem::absolute(root).parent_path().parent_path();
        auto schema_root = root / "schemas";
        nlohmann::json config = {{"mapping_directory", mapping_directory}, {"schemas_directory", schema_root.string()}};
        self->cpp_mapper->init(config);
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return -1;
    }

    return 0;
}

PyTypeObject PyMapperType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "clibtokamap.PyMapper",
    .tp_basicsize = sizeof(PyMapper),
    .tp_dealloc = (destructor)PyMapper_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_init = (initproc)PyMapper_init,
    .tp_new = PyMapper_new,
};

PyObject* libtokamap_create(PyObject* Py_UNUSED(module), PyObject* args)
{
    return PyObject_CallOneArg(reinterpret_cast<PyObject*>(&PyMapperType), args);
}

PyObject* get_pathlib_path_type()
{
    PyObject* pathlib = PyImport_ImportModule("pathlib");
    if (!pathlib) {
        PyErr_SetString(LibTokaMapError, "Failed to import pathlib module");
        return nullptr;
    }

    PyObject* path_type = PyObject_GetAttrString(pathlib, "Path");

    if (!path_type || !PyType_Check(path_type)) {
        Py_DECREF(pathlib);
        Py_XDECREF(path_type);
        PyErr_SetString(LibTokaMapError, "pathlib.Path type not found or is not a type object");
        return nullptr;
    }

    Py_DECREF(pathlib);
    return path_type;
}

PyObject* libtokamap_register_data_source(PyObject* Py_UNUSED(module), PyObject* args)
{
    PyObject* py_mapper = nullptr;
    const char* data_source_name = nullptr;
    const char* data_source_factory_name = nullptr;
    PyObject* py_factory_args = nullptr;

    if (!PyArg_ParseTuple(args, "OssO", &py_mapper, &data_source_name, &data_source_factory_name, &py_factory_args)) {
        return nullptr;
    }

    if (!PyObject_IsInstance(py_mapper, reinterpret_cast<PyObject*>(&PyMapperType))) {
        PyErr_SetString(LibTokaMapError, "First argument to register_data_source must be a PyMapper");
        return nullptr;
    }

    libtokamap::DataSourceFactoryArgs factory_args;

    if (!PyDict_Check(py_factory_args)) {
        PyErr_SetString(LibTokaMapError, "Fourth argument to register_data_source must be a dictionary");
        return nullptr;
    }

    PyObject* path_type = get_pathlib_path_type();

    PyObject* key = nullptr;
    PyObject* value = nullptr;
    Py_ssize_t pos = 0;
    while (PyDict_Next(py_factory_args, &pos, &key, &value)) {
        auto key_string = to_string(key);
        if (!key_string) {
            return nullptr;
        }
        if (PyUnicode_Check(value)) {
            auto value_string = to_string(value);
            if (!value_string) {
                return nullptr;
            }
            factory_args[key_string.value()] = value_string.value();
        } else if (PyLong_Check(value)) {
            factory_args[key_string.value()] = PyLong_AsLong(value);
        } else if (PyObject_IsInstance(value, path_type)) {
            PyObject* path_str = PyObject_Str(value);
            if (!path_str) {
                return nullptr;
            }
            factory_args[key_string.value()] = std::filesystem::path(to_string(path_str).value());
            Py_DECREF(path_str);
        } else {
            PyErr_SetString(LibTokaMapError, "Dictionary value must be a string or integer");
            return nullptr;
        }
    }

    auto* mapper = reinterpret_cast<PyMapper*>(py_mapper);
    try {
        mapper->cpp_mapper->register_data_source(data_source_name, data_source_factory_name, factory_args);
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return nullptr;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* libtokamap_register_data_source_factory(PyObject* Py_UNUSED(module), PyObject* args)
{
    PyObject* py_mapper = nullptr;
    const char* data_source_factory_name = nullptr;
    const char* data_source_library_path = nullptr;

    if (!PyArg_ParseTuple(args, "Oss", &py_mapper, &data_source_factory_name, &data_source_library_path)) {
        return nullptr;
    }

    if (!PyObject_IsInstance(py_mapper, reinterpret_cast<PyObject*>(&PyMapperType))) {
        PyErr_SetString(LibTokaMapError, "First argument to register_data_source must be a PyMapper");
        return nullptr;
    }

    auto* mapper = reinterpret_cast<PyMapper*>(py_mapper);
    try {
        mapper->cpp_mapper->register_data_source_factory(data_source_factory_name, data_source_library_path);
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return nullptr;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* get_data_source_type()
{
    PyObject* libtokamap = PyImport_ImportModule("libtokamap");
    if (!libtokamap) {
        PyErr_SetString(LibTokaMapError, "Failed to import libtokamap module");
        return nullptr;
    }

    PyObject* data_source_type = PyObject_GetAttrString(libtokamap, "DataSource");

    if (!data_source_type || !PyType_Check(data_source_type)) {
        Py_DECREF(libtokamap);
        Py_XDECREF(data_source_type);
        PyErr_SetString(LibTokaMapError, "DataSource type not found or is not a type object");
        return nullptr;
    }

    Py_DECREF(libtokamap);
    return data_source_type;
}

PyObject* libtokamap_register_python_data_source(PyObject* Py_UNUSED(module), PyObject* args)
{
    PyObject* py_mapper = nullptr;
    const char* data_source_name = nullptr;
    PyObject* data_source = nullptr;

    if (!PyArg_ParseTuple(args, "OsO", &py_mapper, &data_source_name, &data_source)) {
        return nullptr;
    }

    if (!PyObject_IsInstance(py_mapper, reinterpret_cast<PyObject*>(&PyMapperType))) {
        PyErr_SetString(LibTokaMapError, "First argument to libtokamap_register_python_data_source must be a PyMapper");
        return nullptr;
    }

    PyObject* data_source_type = get_data_source_type();
    if (data_source_type == nullptr) {
        return nullptr;
    }

    if (!PyObject_IsInstance(data_source, data_source_type)) {
        Py_XDECREF(data_source_type);
        PyErr_SetString(LibTokaMapError, "Given data source does not inherit from DataSource class");
        return nullptr;
    }

    auto py_data_source = std::make_unique<PythonDataSource>(data_source);

    auto* mapper = reinterpret_cast<PyMapper*>(py_mapper);
    try {
        mapper->cpp_mapper->register_data_source(data_source_name, std::move(py_data_source));
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return nullptr;
    }

    Py_XDECREF(data_source_type);
    Py_INCREF(Py_None);
    return Py_None;
}

libtokamap::TypedDataArray call_python_function(PyObject* function, libtokamap::CustomMappingInputs& inputs,
                                                const libtokamap::CustomMappingParams& params)
{
    auto* py_inputs = PyDict_New();
    for (auto& [key, value] : inputs) {
        if (!set_dictionary_item(py_inputs, key, value)) {
            return {};
        }
    }

    auto* py_params = PyDict_New();
    for (const auto& [key, value] : params.items()) {
        if (!set_dictionary_item(py_params, key, value)) {
            return {};
        }
    }

    auto* args = PyTuple_New(2);
    PyTuple_SET_ITEM(args, 0, py_inputs);
    PyTuple_SET_ITEM(args, 1, py_params);

    PyObject* result = PyObject_Call(function, args, nullptr);
    Py_DECREF(args);

    return object_to_typed_data_array(result);
}

class PythonFunctionWrapper : public libtokamap::LibraryFunctionWrapper
{
  public:
    explicit PythonFunctionWrapper(PyObject* function) : m_function(function) { Py_XINCREF(function); }
    ~PythonFunctionWrapper() override { Py_XDECREF(m_function); }

    PythonFunctionWrapper(const PythonFunctionWrapper&) = delete;
    PythonFunctionWrapper(PythonFunctionWrapper&&) = delete;
    PythonFunctionWrapper& operator=(const PythonFunctionWrapper&) = delete;
    PythonFunctionWrapper& operator=(PythonFunctionWrapper&&) = delete;

    libtokamap::TypedDataArray operator()(libtokamap::CustomMappingInputs& inputs,
                                          const libtokamap::CustomMappingParams& params) const override
    {
        return call_python_function(m_function, inputs, params);
    }

  private:
    PyObject* m_function;
};

PyObject* libtokamap_load_custom_function_library(PyObject* Py_UNUSED(module), PyObject* args)
{
    PyObject* py_mapper = nullptr;
    PyObject* py_library_path = nullptr;

    if (!PyArg_ParseTuple(args, "OO", &py_mapper, &py_library_path)) {
        return nullptr;
    }

    if (!PyObject_IsInstance(py_mapper, reinterpret_cast<PyObject*>(&PyMapperType))) {
        PyErr_SetString(LibTokaMapError, "First argument to register_data_source must be a PyMapper");
        return nullptr;
    }

    PyObject* path_type = get_pathlib_path_type();

    if (!PyObject_IsInstance(py_library_path, path_type)) {
        PyErr_SetString(LibTokaMapError, "Second argument to register_data_source must be a pathlib.Path");
        return nullptr;
    }

    PyObject* py_library_path_str = PyObject_Str(py_library_path);
    if (py_library_path_str == nullptr) {
        PyErr_SetString(LibTokaMapError, "Failed to convert library path to string");
        return nullptr;
    }
    auto library_path = to_string(py_library_path_str);
    if (!library_path) {
        PyErr_SetString(LibTokaMapError, "Failed to convert library path to string");
        return nullptr;
    }

    auto* mapper = reinterpret_cast<PyMapper*>(py_mapper);
    try {
        mapper->cpp_mapper->load_custom_function_library(library_path.value());
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return nullptr;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* libtokamap_register_custom_function(PyObject* Py_UNUSED(module), PyObject* args)
{
    PyObject* py_mapper = nullptr;
    const char* lib_name = nullptr;
    const char* func_name = nullptr;
    PyObject* function = nullptr;

    if (!PyArg_ParseTuple(args, "OssO", &py_mapper, &lib_name, &func_name, &function)) {
        return nullptr;
    }

    if (!PyObject_IsInstance(py_mapper, reinterpret_cast<PyObject*>(&PyMapperType))) {
        PyErr_SetString(LibTokaMapError, "Second argument to register_data_source must be a PyMapper");
        return nullptr;
    }

    auto function_wrapper = std::make_unique<PythonFunctionWrapper>(function);
    libtokamap::LibraryFunction library_function{lib_name, func_name, std::move(function_wrapper)};

    auto* mapper = reinterpret_cast<PyMapper*>(py_mapper);
    try {
        mapper->cpp_mapper->register_custom_function(std::move(library_function));
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return nullptr;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* libtokamap_map(PyObject* Py_UNUSED(module), PyObject* const* args, Py_ssize_t nargs)
{
    if (nargs != 4) {
        PyErr_SetString(LibTokaMapError, "Map must be called with 4 arguments");
        return nullptr;
    }

    PyObject* py_mapper = args[0];
    PyObject* experiment = args[1];
    PyObject* path = args[2];
    PyObject* py_attributes = args[3];

    if (!PyObject_IsInstance(py_mapper, reinterpret_cast<PyObject*>(&PyMapperType))) {
        PyErr_SetString(LibTokaMapError, "First argument to register must be a PyMapper");
        return nullptr;
    }

    auto experiment_string = to_string(experiment);
    if (!experiment_string) {
        return nullptr;
    }

    auto path_string = to_string(path);
    if (!path_string) {
        return nullptr;
    }

    if (!PyDict_Check(py_attributes)) {
        PyErr_SetString(LibTokaMapError, "Fourth argument to map must be a dictionary");
        return nullptr;
    }

    try {
        auto* mapper = reinterpret_cast<PyMapper*>(py_mapper);
        auto data_type = libtokamap::DataType::Double;
        int rank = 1;
        nlohmann::json attributes = {};
        if (py_attributes != nullptr) {
            PyObject* key = nullptr;
            PyObject* value = nullptr;
            Py_ssize_t pos = 0;
            while (PyDict_Next(py_attributes, &pos, &key, &value)) {
                auto key_string = to_string(key);
                if (!key_string) {
                    return nullptr;
                }
                if (PyUnicode_Check(value)) {
                    auto value_string = to_string(value);
                    if (!value_string) {
                        return nullptr;
                    }
                    attributes[key_string.value()] = value_string.value();
                } else if (PyLong_Check(value)) {
                    attributes[key_string.value()] = PyLong_AsLong(value);
                } else {
                    PyErr_SetString(LibTokaMapError, "Dictionary value must be a string or integer");
                    return nullptr;
                }
            }
        }
        auto result =
            mapper->cpp_mapper->map(experiment_string.value(), path_string.value(), data_type, rank, attributes);

        // FIXME : Handle data types!
        if (!result.empty()) {
            return array_to_numpy(result);
        }
    } catch (const std::exception& e) {
        PyErr_SetString(LibTokaMapError, e.what());
        return nullptr;
    }

    return nullptr;
}

int libtokamap_module_exec(PyObject* module)
{
    if (LibTokaMapError != nullptr) {
        PyErr_SetString(PyExc_ImportError, "Cannot initialize libtokamap module more than once");
        return -1;
    }
    LibTokaMapError = PyErr_NewException("libtokamap.LibTokaMapError", nullptr, nullptr);
    if (PyModule_AddObjectRef(module, "LibTokaMapError", LibTokaMapError) < 0) {
        return -1;
    }

    if (PyType_Ready(&PyMapperType) < 0) {
        return -1;
    }

    Py_INCREF(&PyMapperType);
    PyModule_AddObject(module, "PyMapper", (PyObject*)&PyMapperType);

    if (PyModule_AddStringConstant(module, "__version__", libtokamap::Version) < 0) {
        return -1;
    }

    if (PyModule_AddStringConstant(module, "LibrarySuffix", libtokamap::LibrarySuffix) < 0) {
        return -1;
    }

    return 0;
}

PyModuleDef_Slot libtokamap_module_slots[] = {{Py_mod_exec, (void*)libtokamap_module_exec}, {0, nullptr}};

struct PyModuleDef libtokamap_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "clibtokamap",
    .m_size = 0, // non-negative
    .m_methods = libtokamap_methods,
    .m_slots = libtokamap_module_slots,
};

} // namespace

PyMODINIT_FUNC PyInit_clibtokamap()
{
    import_array();
    return PyModuleDef_Init(&libtokamap_module);
}
