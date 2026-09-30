# Fourier Licensing

Fourier uses separate licenses for source code and visual assets. These terms
apply to their respective materials; they are not alternative licenses for the
entire repository. The standard software license text is in [LICENSE](LICENSE),
kept separate from this scope guide for GitHub license detection. This layout
does not change the license terms.

## Source Code

All **source code** in the `src/` folder is copyright © 2025 Arhythmetic Units
and contributors. This program is free software: you can redistribute it and/or
modify it under the terms of the
[GNU General Public License](LICENSE) as
published by the [Free Software Foundation](https://www.fsf.org/), either
version 3 of the License, or (at your option) any later version.

## Visual Assets

The **visual design of the modules** is copyright © 2025 Arhythmetic Units and
licensed under
[CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).
Commercial use and derivative works **ARE NOT** allowed. This includes all
graphics in the `res/`, `docs/manual-fourier/`, and `docs/manual-spectre/`
folders, and the visual artwork represented by
`src/rack_extensions/panel_artwork.hpp` and `src/rack_extensions/panel.hpp`.

The **Arhythmetic Units logo and icon** are copyright © 2025 Arhythmetic Units
and licensed under
[CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).
Commercial use and derivative works **ARE NOT** allowed. This includes all
graphics in the `res/`, `docs/manual-fourier/`, and `docs/manual-spectre/`
folders, and the branding represented by `src/rack_extensions/panel_artwork.hpp`.
Moving the artwork into drawing code does not change these visual-design terms;
the source code itself remains under the GPL terms above.

## Bundled Dependencies

Bundled dependencies retain their own notices. The test and benchmark
executables use [Catch2](dep/Catch2/README.md) under the
[Boost Software License 1.0](dep/Catch2/LICENSE_1_0.txt); Catch2 is not linked
into the Rack plugin. See those dependency notes for provenance and checksums.

The plugin package includes `LICENSE` and this scope guide. Portable benchmark
bundles include both files beside their tooling; historical evidence archives
retain their original license files.
