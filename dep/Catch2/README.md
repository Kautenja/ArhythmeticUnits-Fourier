# Catch2

Fourier vendors the unmodified Catch2 **2.13.10** amalgamated single header
in `catch.hpp`. Tests and benchmarks include it directly and define
`CATCH_CONFIG_MAIN` in each executable. No submodule, CMake build, or
separately installed Catch2 is required.

This is the last v2 release and supports Fourier's C++11 baseline. Catch2
v3 requires C++14; its amalgamated distribution contains both a header and
a `.cpp` file. See the upstream [migration guide][migration].

## Provenance And License

-   Release: [v2.13.10][release]
-   Commit: `182c910b4b63ff587a3440e08f84f70497e49a81`
-   Header: [`single_include/catch2/catch.hpp`][header]
-   License: [Boost Software License 1.0](LICENSE_1_0.txt), copied from the
    upstream [`LICENSE.txt`][license]

SHA-256 checksums:

```text
3725c0f0a75f376a5005dde31ead0feb8f7da7507644c201b814443de8355170  catch.hpp
c9bff75738922193e67fa726fa225535870d2aa1059f91452c411736284ad566  LICENSE_1_0.txt
```

## Updating

Download the header and license from an exact upstream release commit,
preserving their contents and attribution. Update the version, commit,
links, and checksums here and the version in `CONTRIBUTING.md`. Do not edit
the generated header or restore unused reporter headers.

From the repository root, verify the downloaded files against upstream and
run the test and benchmark builds:

```shell
shasum -a 256 dep/Catch2/catch.hpp dep/Catch2/LICENSE_1_0.txt
scons -j4 test benchmark-build
make -j4 all test-rack benchmark-rack-build
```

The Make targets require a configured Rack SDK. Smoke-test benchmark
execution as described in the
[contributor guide](../../CONTRIBUTING.md#benchmarks); a successful build
does not verify benchmark execution or establish a performance improvement.

[release]: https://github.com/catchorg/Catch2/releases/tag/v2.13.10
[header]: https://raw.githubusercontent.com/catchorg/Catch2/182c910b4b63ff587a3440e08f84f70497e49a81/single_include/catch2/catch.hpp
[license]: https://raw.githubusercontent.com/catchorg/Catch2/182c910b4b63ff587a3440e08f84f70497e49a81/LICENSE.txt
[migration]: https://github.com/catchorg/Catch2/blob/devel/docs/migrate-v2-to-v3.md
