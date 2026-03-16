import clibtokamap

from abc import ABC, abstractmethod
from pathlib import Path
import numpy as np
from typing import Callable, Any


__version__ = clibtokamap.__version__
LibrarySuffix = clibtokamap.LibrarySuffix


class DataSource(ABC):
    """Abstract base class for data sources.

    Subclasses should implement the `get` method to retrieve data. Any attempt to register a data source that
    is not a subclass of DataSource will raise a Exception.
    """
    @abstractmethod
    def get(self, args: dict[str, str]) -> np.ndarray:
        """Get the data from the data source.

        Args:
            args: A dictionary of arguments provided by the mapping which can be used to identify the data to retrieve.

        Returns:
            A numpy array containing the data.
        """
        ...

class Mapper:
    """Mapper class for mapping data from data sources.

    This class provides a way to map data from multiple data sources based on a mapping configuration.
    """

    def __init__(self,
                 mapping_path: str | None = None,
                 schemas_path: str | None = None,
                 config_path: str | None = None
                 ):
        """Initialize a new Mapper instance.

        A Mapper can be initialized in one of two ways:

        1. TOML configuration
           Provide ``config_path`` pointing to a ``config.toml`` file. In this
           mode all configuration (including mapping and schema locations) is
           read from the TOML file.

        2. Directory-based configuration
           Provide ``mapping_path`` and optionally ``schemas_path``. The mapper
           will load mapping definitions from the mapping directory and use the
           provided schemas directory, if supplied, for schema validation. If no
           schema location is provided a default path is attempted.
    
        These two initialization routes are mutually exclusive: if ``config_path``
        is provided, ``mapping_path`` and ``schemas_path`` are ignored.

        Args:
            mapping_path: The path to the directory containing the mapping files.
            schemas_path: The path to the directory containing the JSON-schema definition.
            config_path: The path to a config.toml file defining all set-up parameters.
        """
        if config_path:
            self._mapper = clibtokamap.create_from_toml(config_path)
            # self._mapper = clibtokamap.PyMapper_init(config_path=config_path)
        elif mapping_path and schemas_path:
            self._mapper = clibtokamap.create(mapping_path, schemas_path=schemas_path)
        elif mapping_path:
            self._mapper = clibtokamap.create(mapping_path)

    def register_data_source_factory(self, factory_name: str, factory_library: str) -> None:
        clibtokamap.register_data_source_factory(self._mapper, factory_name, factory_library)

    def register_data_source(self, name: str, factory_name: str, args: dict[str, str]) -> None:
        clibtokamap.register_data_source(self._mapper, name, factory_name, args)

    def register_python_data_source(self, name: str, data_source: DataSource) -> None:
        """Register a data source with the mapper.

        Args:
            name: The name of the data source.
            data_source: The data source to register.

        Throws:
            LibTokaMapError: If the data source is not a subclass of DataSource.
        """
        clibtokamap.register_python_data_source(self._mapper, name, data_source)

    def load_custom_function_library(self, library_path: Path) -> None:
        """Load a custom function library.

        Args:
            library_path: The path to the library.
        """
        clibtokamap.load_custom_function_library(self._mapper, library_path)

    def register_custom_function(self, library_name: str, function_name: str, function: Callable[[dict[str, np.array], dict[str, Any]], np.array]) -> None:
        """Register a custom function with the mapper.

        Args:
            library_name: The name of the library for the function.
            function_name: The name of the function to register.
            function: The function to register.
        """
        clibtokamap.register_custom_function(self._mapper, library_name, function_name, function)

    def map(self, experiment: str, path: str, attributes: dict[str, str] | None = None) -> np.ndarray:
        """Map the data from the data sources.

        Args:
            experiment: The name of the experiment.
            path: The path to the data.
            attributes: A dictionary of attributes to pass to the data sources.

        Returns:
            A numpy array containing the mapped data.
        """
        if attributes is None:
            attributes = {}
        return clibtokamap.map(self._mapper, experiment, path, attributes)
