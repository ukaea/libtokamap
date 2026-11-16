#include "python_conversions.hpp"

#include <algorithm>
#include <any>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "utils/typed_data_array.hpp"

namespace
{

using libtokamap::TypedDataArray;
using libtokamap::python::PyRef;
using libtokamap::python::PythonError;

PyRef checked(PyObject* object, const char* context)
{
    if (object == nullptr) {
        libtokamap::python::throw_python_error(context);
    }
    return PyRef::steal(object);
}

// Releases a Py_buffer on scope exit
class BufferView
{
  public:
    explicit BufferView(PyObject* object)
    {
        if (PyObject_GetBuffer(object, &m_view, PyBUF_RECORDS_RO) != 0) {
            libtokamap::python::throw_python_error("failed to read buffer of returned " +
                                                   libtokamap::python::type_name(object));
        }
    }
    ~BufferView() { PyBuffer_Release(&m_view); }

    BufferView(const BufferView&) = delete;
    BufferView(BufferView&&) = delete;
    BufferView& operator=(const BufferView&) = delete;
    BufferView& operator=(BufferView&&) = delete;

    [[nodiscard]] const Py_buffer& view() const { return m_view; }

  private:
    Py_buffer m_view{};
};

// Parsed single-element struct-module format string, e.g. "<d", "?", "<5w", "3s"
struct ElementFormat {
    char code = '\0';
    bool byte_swapped = false;
};

ElementFormat parse_format(const char* format)
{
    std::string text = format == nullptr ? "B" : format;
    ElementFormat result;

    size_t pos = 0;
    if (pos < text.size() && std::strchr("@=<>!", text[pos]) != nullptr) {
        char order = text[pos++];
        bool little_endian = std::endian::native == std::endian::little;
        result.byte_swapped = (order == '<' && !little_endian) || ((order == '>' || order == '!') && little_endian);
    }

    size_t digits_start = pos;
    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0) {
        ++pos;
    }
    bool has_count = pos > digits_start;

    if (pos + 1 != text.size()) {
        throw PythonError{"unsupported element format '" + text + "'"};
    }
    result.code = text[pos];

    if (has_count && result.code != 'w' && result.code != 's') {
        throw PythonError{"unsupported element format '" + text + "'"};
    }
    return result;
}

void byte_swap(std::vector<char>& buffer, size_t unit_size)
{
    if (unit_size <= 1) {
        return;
    }
    for (size_t offset = 0; offset + unit_size <= buffer.size(); offset += unit_size) {
        std::reverse(buffer.begin() + static_cast<std::ptrdiff_t>(offset),
                     buffer.begin() + static_cast<std::ptrdiff_t>(offset + unit_size));
    }
}

template <typename T> TypedDataArray make_array(const char* data, size_t count, std::vector<size_t> shape)
{
    // TypedDataArray copies from the pointer when owning, so the const_cast is never written through
    return TypedDataArray{const_cast<T*>(reinterpret_cast<const T*>(data)), count, std::move(shape)};
}

TypedDataArray make_signed(const char* data, size_t count, std::vector<size_t> shape, Py_ssize_t item_size)
{
    switch (item_size) {
        case 1:
            return make_array<int8_t>(data, count, std::move(shape));
        case 2:
            return make_array<int16_t>(data, count, std::move(shape));
        case 4:
            return make_array<int32_t>(data, count, std::move(shape));
        case 8:
            return make_array<int64_t>(data, count, std::move(shape));
        default:
            throw PythonError{"unsupported signed integer size " + std::to_string(item_size)};
    }
}

TypedDataArray make_unsigned(const char* data, size_t count, std::vector<size_t> shape, Py_ssize_t item_size)
{
    switch (item_size) {
        case 1:
            return make_array<uint8_t>(data, count, std::move(shape));
        case 2:
            return make_array<uint16_t>(data, count, std::move(shape));
        case 4:
            return make_array<uint32_t>(data, count, std::move(shape));
        case 8:
            return make_array<uint64_t>(data, count, std::move(shape));
        default:
            throw PythonError{"unsupported unsigned integer size " + std::to_string(item_size)};
    }
}

// Decode the single element of a NumPy 'U' (UCS4, code 'w') or 'S' (bytes, code 's') array, dropping NUL padding
TypedDataArray make_string(const char* data, size_t count, Py_ssize_t item_size, char code)
{
    if (count != 1) {
        throw PythonError{"string arrays must contain exactly one element, got " + std::to_string(count)};
    }
    if (code == 's') {
        std::string value{data, static_cast<size_t>(item_size)};
        value.erase(value.find_last_not_of('\0') + 1);
        return TypedDataArray{value};
    }

    std::vector<Py_UCS4> code_points(static_cast<size_t>(item_size) / sizeof(Py_UCS4));
    std::memcpy(code_points.data(), data, code_points.size() * sizeof(Py_UCS4));
    while (!code_points.empty() && code_points.back() == 0) {
        code_points.pop_back();
    }
    PyRef unicode = checked(PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, code_points.data(),
                                                      static_cast<Py_ssize_t>(code_points.size())),
                            "failed to decode unicode array element");
    Py_ssize_t size = 0;
    const char* utf8 = PyUnicode_AsUTF8AndSize(unicode.get(), &size);
    if (utf8 == nullptr) {
        libtokamap::python::throw_python_error("failed to encode unicode array element");
    }
    return TypedDataArray{std::string{utf8, static_cast<size_t>(size)}};
}

TypedDataArray buffer_to_typed_data_array(PyObject* object)
{
    BufferView buffer{object};
    const Py_buffer& view = buffer.view();

    ElementFormat format = parse_format(view.format);
    if (format.code == 'O') {
        throw PythonError{"object arrays are not supported"};
    }

    std::vector<size_t> shape;
    for (int dim = 0; dim < view.ndim; ++dim) {
        shape.push_back(static_cast<size_t>(view.shape[dim]));
    }
    size_t count = view.itemsize > 0 ? static_cast<size_t>(view.len / view.itemsize) : 0;

    // Gather into C order and native byte order if needed. Strings are byte swapped per UCS4 code unit.
    std::vector<char> contiguous;
    const char* data = static_cast<const char*>(view.buf);
    if (PyBuffer_IsContiguous(&view, 'C') == 0) {
        contiguous.resize(static_cast<size_t>(view.len));
        if (PyBuffer_ToContiguous(contiguous.data(), &view, view.len, 'C') != 0) {
            libtokamap::python::throw_python_error("failed to copy non-contiguous buffer");
        }
        data = contiguous.data();
    }
    if (format.byte_swapped) {
        if (contiguous.empty()) {
            contiguous.assign(data, data + view.len);
        }
        size_t unit_size = format.code == 'w' ? sizeof(Py_UCS4) : (format.code == 's' ? 1 : view.itemsize);
        byte_swap(contiguous, unit_size);
        data = contiguous.data();
    }

    switch (format.code) {
        case '?':
            return make_unsigned(data, count, std::move(shape), view.itemsize);
        case 'b':
        case 'h':
        case 'i':
        case 'l':
        case 'q':
        case 'n':
            return make_signed(data, count, std::move(shape), view.itemsize);
        case 'B':
        case 'H':
        case 'I':
        case 'L':
        case 'Q':
        case 'N':
            return make_unsigned(data, count, std::move(shape), view.itemsize);
        case 'f':
            return make_array<float>(data, count, std::move(shape));
        case 'd':
            return make_array<double>(data, count, std::move(shape));
        case 'w':
        case 's':
            return make_string(data, count, view.itemsize, format.code);
        default:
            throw PythonError{"unsupported element format '" + std::string{view.format} + "'"};
    }
}

} // namespace

libtokamap::python::PyRef libtokamap::python::json_to_python(const nlohmann::json& value)
{
    switch (value.type()) {
        case nlohmann::json::value_t::null:
            return PyRef::borrow(Py_None);
        case nlohmann::json::value_t::boolean:
            return PyRef::borrow(value.get<bool>() ? Py_True : Py_False);
        case nlohmann::json::value_t::number_integer:
            return checked(PyLong_FromLongLong(value.get<int64_t>()), "failed to convert integer");
        case nlohmann::json::value_t::number_unsigned:
            return checked(PyLong_FromUnsignedLongLong(value.get<uint64_t>()), "failed to convert integer");
        case nlohmann::json::value_t::number_float:
            return checked(PyFloat_FromDouble(value.get<double>()), "failed to convert float");
        case nlohmann::json::value_t::string: {
            const auto& string = value.get_ref<const std::string&>();
            return checked(PyUnicode_FromStringAndSize(string.data(), static_cast<Py_ssize_t>(string.size())),
                           "failed to convert string");
        }
        case nlohmann::json::value_t::array: {
            PyRef list = checked(PyList_New(static_cast<Py_ssize_t>(value.size())), "failed to create list");
            Py_ssize_t index = 0;
            for (const auto& element : value) {
                // PyList_SetItem steals the reference
                PyList_SetItem(list.get(), index++, json_to_python(element).release());
            }
            return list;
        }
        case nlohmann::json::value_t::object: {
            PyRef dict = checked(PyDict_New(), "failed to create dict");
            for (const auto& [key, element] : value.items()) {
                PyRef item = json_to_python(element);
                if (PyDict_SetItemString(dict.get(), key.c_str(), item.get()) != 0) {
                    throw_python_error("failed to set dict item '" + key + "'");
                }
            }
            return dict;
        }
        case nlohmann::json::value_t::binary:
        case nlohmann::json::value_t::discarded:
            break;
    }
    throw PythonError{std::string{"unsupported JSON value type '"} + value.type_name() + "'"};
}

libtokamap::python::PyRef libtokamap::python::factory_arg_to_python(const std::string& name, const std::any& value)
{
    if (const auto* number = std::any_cast<double>(&value)) {
        return checked(PyFloat_FromDouble(*number), "failed to convert float");
    }
    if (const auto* number = std::any_cast<int>(&value)) {
        return checked(PyLong_FromLong(*number), "failed to convert integer");
    }
    if (const auto* boolean = std::any_cast<bool>(&value)) {
        return PyRef::borrow(*boolean ? Py_True : Py_False);
    }
    if (const auto* string = std::any_cast<std::string>(&value)) {
        return checked(PyUnicode_FromStringAndSize(string->data(), static_cast<Py_ssize_t>(string->size())),
                       "failed to convert string");
    }
    if (const auto* json = std::any_cast<nlohmann::json>(&value)) {
        return json_to_python(*json);
    }
    throw PythonError{"factory argument '" + name + "' has an unsupported type"};
}

libtokamap::TypedDataArray libtokamap::python::python_to_typed_data_array(PyObject* object)
{
    if (PyUnicode_Check(object)) {
        Py_ssize_t size = 0;
        const char* utf8 = PyUnicode_AsUTF8AndSize(object, &size);
        if (utf8 == nullptr) {
            throw_python_error("failed to encode returned str");
        }
        return TypedDataArray{std::string{utf8, static_cast<size_t>(size)}};
    }
    if (PyObject_CheckBuffer(object) == 0) {
        throw PythonError{"expected a NumPy array, buffer-protocol object or str, got '" + type_name(object) + "'"};
    }
    return buffer_to_typed_data_array(object);
}
