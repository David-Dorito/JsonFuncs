# JSON Parser with Address Mapping

A C function that parses JSON files or strings, builds an object tree in memory, and writes requested values directly to user-provided memory addresses. Designed for flexible parsing with error handling.

## Features

- Parse JSON from:
  - Files
  - Raw strings
- Builds an in-memory object tree (heap-allocated)
- Map specific keys to memory addresses
- Returns explicit error codes for:
  - Invalid paths to values
  - Failed malloc calls
  - Malformed JSON
- Supports primitive JSON types: integers, floats, strings

>  **Note:** Nested objects and arrays are **not supported** in the current version.

##  Concept

This parser creates a full object tree to allow:

1. Flexible key lookups  
2. Safe memory mapping  
3. Detailed error reporting  

Example usage can be found inside the main.c, dont forget to delete that file if you plan on using the function
