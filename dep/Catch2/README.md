# Catch2

Fourier vendors the unmodified Catch2 **3.16.0** amalgamated distribution:
`catch_amalgamated.hpp` and `catch_amalgamated.cpp`. No submodule, CMake
build, or separately installed Catch2 is required.

Tests and Catch2 benchmarks use C++14. The shipped Rack plugin and reusable
DSP retain their C++11 baseline. Catch2 is linked only into the test and
benchmark executables, never the plugin.

The amalgamated source supplies `main` and is compiled once per build
configuration. SCons keeps ordinary tests, optimized benchmarks, and each
instrumentation mode in separate object directories. Make does the same
for headless Rack tests and benchmarks. Suites include the header without
`CATCH_CONFIG_MAIN` or `CATCH_CONFIG_ENABLE_BENCHMARKING`.

## Provenance And License

-   Release: [v3.16.0][release]
-   Commit: `317ac1ed4c0bb6e6b91eafc817e05c488feffcb3`
-   Header: [`extras/catch_amalgamated.hpp`][header]
-   Source: [`extras/catch_amalgamated.cpp`][source]
-   License: [Boost Software License 1.0](LICENSE_1_0.txt), copied from the
    upstream [`LICENSE.txt`][license]

SHA-256 checksums:

```text
d4cc143ea76ae212204363922d8adf376d66a1fda5a33ac73f93a7d1c119f4e0  catch_amalgamated.hpp
1fe7f10334e0ae5494419cfa84c270f15235eada0fc02bdb493d1def537601f3  catch_amalgamated.cpp
c9bff75738922193e67fa726fa225535870d2aa1059f91452c411736284ad566  LICENSE_1_0.txt
```

## Updating

Download the amalgamated header, source, and license from an exact upstream
release commit, preserving their contents and attribution. Update the
version, commit, links, and checksums here and the version in
`CONTRIBUTING.md`. Do not edit the generated files. Consult the upstream
[migration guide][migration] when changing major versions.

From the repository root, verify the downloaded files against upstream and
run the test and benchmark builds:

```shell
shasum -a 256 dep/Catch2/catch_amalgamated.hpp dep/Catch2/catch_amalgamated.cpp dep/Catch2/LICENSE_1_0.txt
scons -j4 test benchmark-build
make -j4 all test-rack benchmark-rack-build
```

The Make targets require a configured Rack SDK. Smoke-test benchmark
execution and check instrumented builds as described in the
[contributor guide](../../CONTRIBUTING.md#development-and-testing). A
successful build does not verify benchmark execution or establish a
performance improvement.

[release]: https://github.com/catchorg/Catch2/releases/tag/v3.16.0
[header]: https://raw.githubusercontent.com/catchorg/Catch2/317ac1ed4c0bb6e6b91eafc817e05c488feffcb3/extras/catch_amalgamated.hpp
[source]: https://raw.githubusercontent.com/catchorg/Catch2/317ac1ed4c0bb6e6b91eafc817e05c488feffcb3/extras/catch_amalgamated.cpp
[license]: https://raw.githubusercontent.com/catchorg/Catch2/317ac1ed4c0bb6e6b91eafc817e05c488feffcb3/LICENSE.txt
[migration]: https://github.com/catchorg/Catch2/blob/v3.16.0/docs/migrate-v2-to-v3.md
