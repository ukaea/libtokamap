// Minimal data source factories used to test the factory ABI version check

#include <libtokamap.hpp>

#ifndef TEST_FACTORY_ABI_OFFSET
#define TEST_FACTORY_ABI_OFFSET 0
#endif

LIBTOKAMAP_FACTORY_EXPORT int LibTokaMapFactoryABIVersion()
{
    return libtokamap::FactoryABIVersion + TEST_FACTORY_ABI_OFFSET;
}

LIBTOKAMAP_FACTORY_EXPORT void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& entry)
{
    entry.function = [](const libtokamap::DataSourceFactoryArgs&) -> std::unique_ptr<libtokamap::DataSource> {
        return nullptr;
    };
}
