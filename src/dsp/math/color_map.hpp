// Functions for applying colormaps to values.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef DSP_MATH_COLOR_MAP_HPP
#define DSP_MATH_COLOR_MAP_HPP

#include <stdio.h>
#include <math.h>

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Functions for applying colormaps to scalar values.
namespace ColorMap {

/// @brief Structure representing an RGB color.
/// Each component of the color is a floating-point value in the range [0, 1].
typedef struct {
    float r;  ///< Red component
    float g;  ///< Green component
    float b;  ///< Blue component
} Color;

/// @brief Linearly interpolate between two colors.
/// @param c1 The first color.
/// @param c2 The second color.
/// @param t Interpolation factor in the range [0, 1].
/// @return The interpolated color.
inline Color interpolate_color(Color c1, Color c2, float t) {
    Color result;
    result.r = c1.r + t * (c2.r - c1.r);
    result.g = c1.g + t * (c2.g - c1.g);
    result.b = c1.b + t * (c2.b - c1.b);
    return result;
}

/// @brief Get a color from a colormap table.
/// @tparam T Type of the colormap table (array of Color structs).
/// @tparam N Size of the colormap table (number of entries).
/// @param colormap_table Array of colors representing the colormap.
/// @param value Scalar value in the range [0, 1] to map to a color.
/// @return The interpolated color.
template<typename T, size_t N>
inline Color get_colormap_value(const T (&colormap_table)[N], float value) {
    static const int TABLE_SIZE = N;
    // Clamp input to [0, 1]
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    // Find the segment in the colormap
    float scaled_value = value * (TABLE_SIZE - 1);
    int index = static_cast<int>(scaled_value);
    float t = scaled_value - index;
    // Interpolate between the two nearest colors
    if (index >= TABLE_SIZE - 1)
        return colormap_table[TABLE_SIZE - 1];
    return interpolate_color(colormap_table[index], colormap_table[index + 1], t);
}

/// @brief Map a scalar value to a color using the Viridis colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color viridis(float value) {
    static const Color viridis_table[] = {
        {0.267004, 0.004874, 0.329415},
        {0.282327, 0.092374, 0.417331},
        {0.253935, 0.265254, 0.529983},
        {0.206756, 0.371758, 0.553117},
        {0.163625, 0.471133, 0.558148},
        {0.127568, 0.566949, 0.550556},
        {0.134692, 0.658636, 0.517649},
        {0.266941, 0.748751, 0.440573},
        {0.477504, 0.821444, 0.318195},
        {0.741388, 0.873449, 0.149561},
        {0.993248, 0.906157, 0.143936}
    };
    return get_colormap_value(viridis_table, value);
}

/// @brief Map a scalar value to a color using the Plasma colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color plasma(float value) {
    static const Color plasma_table[] = {
        {0.050383, 0.029803, 0.527975},
        {0.229739, 0.322361, 0.545706},
        {0.503385, 0.516162, 0.516759},
        {0.799329, 0.676878, 0.377779},
        {0.983868, 0.902323, 0.138825}
    };
    return get_colormap_value(plasma_table, value);
}

/// @brief Map a scalar value to a color using the Inferno colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color inferno(float value) {
    static const Color inferno_table[] = {
        {0.001462, 0.000466, 0.013866},
        {0.072073, 0.039782, 0.423641},
        {0.368507, 0.209199, 0.452253},
        {0.737683, 0.478540, 0.423388},
        {0.988362, 0.998364, 0.644924}
    };
    return get_colormap_value(inferno_table, value);
}

/// @brief Map a scalar value to a color using the Magma colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color magma(float value) {
    static const Color magma_table[] = {
        {0.001462, 0.000466, 0.013866},
        {0.190631, 0.049697, 0.432227},
        {0.630797, 0.211718, 0.530316},
        {0.959269, 0.596947, 0.398884},
        {0.987053, 0.991438, 0.749504}
    };
    return get_colormap_value(magma_table, value);
}

/// @brief Map a scalar value to a color using the Cividis colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color cividis(float value) {
    static const Color cividis_table[] = {
        {0.000000, 0.135112, 0.304751},
        {0.229739, 0.322361, 0.545706},
        {0.503385, 0.516162, 0.516759},
        {0.799329, 0.677704, 0.377779},
        {0.940015, 0.975158, 0.131326}
    };
    return get_colormap_value(cividis_table, value);
}

/// @brief Map a scalar value to a color using the Bone colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color bone(float value) {
    static const Color bone_table[] = {
        {0.000000, 0.000000, 0.000000},
        {0.250980, 0.250980, 0.250980},
        {0.500000, 0.500000, 0.500000},
        {0.750980, 0.750980, 0.750980},
        {1.000000, 1.000000, 1.000000}
    };
    return get_colormap_value(bone_table, value);
}

/// @brief Map a scalar value to a color using the Gray colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color gray(float value) {
    static const Color gray_table[] = {
        {0.000000, 0.000000, 0.000000},
        {0.250980, 0.250980, 0.250980},
        {0.500000, 0.500000, 0.500000},
        {0.750980, 0.750980, 0.750980},
        {1.000000, 1.000000, 1.000000}
    };
    return get_colormap_value(gray_table, value);
}

}  // namespace ColorMap

}  // namespace Math

#endif  // DSP_MATH_COLOR_MAP_HPP
