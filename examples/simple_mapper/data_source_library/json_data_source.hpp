#include <filesystem>
#include <libtokamap.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>

extern "C" void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& factory);

class JSONDataSource : public libtokamap::DataSource
{
  public:
    explicit JSONDataSource(std::filesystem::path data_root) : m_data_root{std::move(data_root)}
    {
        if (!std::filesystem::exists(m_data_root)) {
            throw libtokamap::FileError{"data root does not exist"};
        }
        if (!std::filesystem::is_directory(m_data_root)) {
            throw libtokamap::FileError{"data root is not a directory"};
        }
    }
    libtokamap::TypedDataArray get(const libtokamap::DataSourceArgs& map_args,
                                   const libtokamap::MapArguments& arguments, libtokamap::RamCache* ram_cache) override;

  private:
    std::filesystem::path m_data_root;
    std::unordered_map<std::filesystem::path::string_type, nlohmann::json> m_data;
};
