# `versioning` and versioning scripts

Versioning scripts are run when the engine loads an out-of-date versioned JSON file (such as a serialised player file). Versioning scripts must be located under `$assets/versioning` (subdirectories are allowe) and must have a name conforming to the format `<identifier>_<fromversion>_<toversion>.lua` or (in xStarbound v3.4.2+) `<identifier>_<fromversion>_<toversion>.pluto`.

When it loads an out-of-date file, the engine attempts to make the longest version jump possible when looking for a versioning script to run, ideally a jump straight from `$oldVersion` to `$newestVersion`.

If there is no such script, the engine attempts to find and run the script with the highest `$newerVersion` below the `$newestVersion` that it can find. The engine does not make recursive attempts, but will throw an exception if any found versioning script did not fully update the versioned JSON file.

> **xStarbound note:** If suitable versioning scripts for the same jump with both `.pluto` and `.lua` extensions exist, the `.pluto` script is preferred by xStarbound v3.4.2+.

----

## Invoked functions

The engine invokes only one `versioning` function in versioning scripts after loading them:

----

#### `Json<Top>` update(`Json<Top>` versionedJsonToUpdate)

Invoked once when the engine loads any versioned JSON file to which this versioning script applies (as per the version specifiers in its name), immediately after loading the script. The engine passes the old JSON as an argument and expects an updated version of the JSON to be returned.

The following context-specific callbacks are available:

- `root` table (`root.md`)
- `celestial` table (`celestial.md`)
- `versioning` table (this file)

----

## `versioning` callbacks

The `versioning` table is available only in versioning scripts. It contains only one callback:

----

#### `Json` versioning.loadVersionedJson(`FilePath` jsonPath)

Loads and deserialises a Starbound-format binary JSON file, returning the resulting data as a JSON-encoded Lua value. Will return `nil` and log a warning if an error occurs during loading or deserialisation, and will throw an error if a path outside of Starbound's storage directory (configured in `xsbinit.config`) is specified.

> **Technical note:** It is actually possible to bypass this "storage directory only" limitation using concatenating `..` specifiers in the path — neither xStarbound nor stock Starbound currently checks for those in the path. This isn't actually sandbox-breaking since this callback can only *load* a file.