#pragma once

constexpr auto ConfigSchema = R"(
{
    "type": "object",
    "properties": {
        "mapping_directory": {
            "type": "string"
        },
        "schemas_directory": {
            "type": "string"
        },
        "trace_enabled": {
            "type": "boolean"
        },
        "cache_enabled": {
            "type": "boolean"
        },
        "mapping_cache_enabled": {
            "type": "boolean"
        },
        "data_source_cache_enabled": {
            "type": "boolean"
        },
        "ram_cache_enabled": {
            "type": "boolean"
        },
        "cache_size": {
            "type": "integer",
            "minimum": 0
        },
        "custom_function_libraries": {
            "type": "array",
            "items": {
                "type": "string"
            }
        },
        "data_source_factories": {
            "type": "object",
            "patternProperties": {
                "^.*$": { "type": "string" }
            }
        },
        "data_sources": {
            "type": "object",
            "patternProperties": {
                "^.*$": { "$ref": "#/$defs/data_source" }
            }
        }
    },
    "required": ["mapping_directory", "schemas_directory"],
    "additionalProperties": false,
    "$defs": {
        "data_source": {
            "type": "object",
            "properties": {
                "factory": { "type": "string" },
                "args": { "type": "object" }
            },
            "required": ["factory"],
            "additionalProperties": false
        }
    }
}
)";
