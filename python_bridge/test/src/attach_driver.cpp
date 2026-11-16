// Test driver loaded with ctypes from a Python process, to check the bridge attaches to an interpreter that is
// already running instead of starting another one. See test_attach.py.

#include <cstddef>
#include <cstdio>
#include <exception>
#include <libtokamap.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

LIBTOKAMAP_FACTORY_EXPORT int run_bridge_attach_check(const char* bridge_path, char* message, size_t message_size)
{
    try {
        auto factory = libtokamap::load_data_source_factory(bridge_path);
        libtokamap::DataSourceFactoryArgs args{{"module", std::string{"bridge_test_sources"}},
                                               {"class_name", std::string{"AttachProbe"}}};
        auto source = factory(args);

        const std::unordered_map<std::string, std::unique_ptr<libtokamap::Mapping>> entries;
        const nlohmann::json globals = nlohmann::json::object();
        const libtokamap::MapArguments arguments{entries, globals, libtokamap::DataType::Double, 1, false,
                                                 false,   nullptr};
        auto result = source->get({{"caller", "attach_driver"}}, arguments, nullptr);
        if (result.to_vector<double>() != std::vector<double>{0, 1, 2, 3, 4, 5}) {
            std::snprintf(message, message_size, "unexpected result %s", result.to_string().c_str());
            return 1;
        }
        return 0;
    } catch (const std::exception& error) {
        std::snprintf(message, message_size, "%s", error.what());
        return 1;
    }
}
