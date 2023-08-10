# JJS: JustJavaScript

Small, modern JavaScript engine for embedding in C/C++ applications.

# Motivation

JavaScript engines, like V8, are fantastic, but they bring long compile times, large memory footprints, and 
can be difficult or impossible to compile for older platforms. The small engines, like QuickJS, are not
frequently updated and lack modern JavaScript support. JJS aims to address the use case of just embedding a 
small, modern JavaScript engine into an application. 

# Background

JJS uses the old [JerryScript](https://github.com/jerryscript-project/jerryscript) project as a base. The 
JerryScript code was a rigorous implementation of the ECMA spec with solid test coverage and
many modern ECMA proposals implemented. Despite all of this work and project potential, the project appears
to be abandoned. Also, the name was very... problematic from a perception and a cultural standpoint.

The only reasonable option was to fork the project and rebrand to continue development.

# Features

* Easy to embed or bind C99 library.
* Small memory footprint.
* Modern JavaScript (partially ECMA 2022 compliant).
* Snapshots.

# Roadmap

* Optional Chaining
* Top Level Await
* WeakRef and FinalizationRegistry
* C++ Bindings
* Performance improvements
* Fix parse errors with private fields
* Add module support to snapshots
* Improve snapshot tooling
* Fix ECMA conformance tests
* Zig compiler support

# Changes

* Set the initial JJS version to 4.0.0.
* Add heap configuration at engine initialization.
* Add jjs_has_pending_jobs() API to check for pending async tasks.
* Fix import/export parsing bugs.
* Fix global variable scope bug when importing.
* Rebranded API to remove problematic "jerry" prefix.
* Add MSVC compiler support.

# Out of Scope

* Continued support for microcontrollers.
* WASM.
* Internalization and Localization.
* Temporal.

# License

JJS is licensed under the Apache 2.0 license. See [LICENSE](LICENSE) for details.
