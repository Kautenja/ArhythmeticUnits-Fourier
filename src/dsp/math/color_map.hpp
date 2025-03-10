// Functions for applying colormaps to values.
//
// Copyright 2025 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_COLOR_MAP_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_COLOR_MAP_HPP_

#include <exception>  // std::runtime_error
#include <string>     // std::string

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
inline Color interpolate_color(const Color& c1, const Color& c2, float t) {
    Color result;
    result.r = c1.r + t * (c2.r - c1.r);
    result.g = c1.g + t * (c2.g - c1.g);
    result.b = c1.b + t * (c2.b - c1.b);
    return result;
}

/// @brief Get a color from a colormap table.
/// @tparam T Type of the colormap table (array of Color structs).
/// @tparam N Size of the colormap table (number of entries).
/// @param colormap Array of colors representing the colormap.
/// @param value Scalar value in the range [0, 1] to map to a color.
/// @return The interpolated color.
template<typename T, size_t N>
inline Color get_colormap_value(const T (&colormap)[N], float value) {
    static const int TABLE_SIZE = N;
    // Clamp input to [0, 1] and find the segment in the color-map.
    float scaled_value = Math::clip(value, 0.f, 1.f) * (TABLE_SIZE - 1);
    int index = static_cast<int>(scaled_value);
    float t = scaled_value - index;
    // Interpolate between the two nearest colors
    if (index >= TABLE_SIZE - 1)
        return colormap[TABLE_SIZE - 1];
    return interpolate_color(colormap[index], colormap[index + 1], t);
}

/// @brief Map a scalar value to a color using the Viridis colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color viridis(float value) {
    static const Color colormap[] = {
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
    return get_colormap_value(colormap, value);
}

/// @brief Map a scalar value to a color using the Plasma colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color plasma(float value) {
    static const Color colormap[] = {
        {0.050383, 0.029803, 0.527975},
        {0.229739, 0.322361, 0.545706},
        {0.503385, 0.516162, 0.516759},
        {0.799329, 0.676878, 0.377779},
        {0.983868, 0.902323, 0.138825}
    };
    return get_colormap_value(colormap, value);
}

/// @brief Map a scalar value to a color using the Inferno colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color inferno(float value) {
    static const Color colormap[] = {
        {0.001462, 0.000466, 0.013866},
        {0.072073, 0.039782, 0.423641},
        {0.368507, 0.209199, 0.452253},
        {0.737683, 0.478540, 0.423388},
        {0.988362, 0.998364, 0.644924}
    };
    return get_colormap_value(colormap, value);
}

/// @brief Map a scalar value to a color using the Magma colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color magma(float value) {
    static const Color colormap[] = {
        {0.001462, 0.000466, 0.013866},
        {0.190631, 0.049697, 0.432227},
        {0.630797, 0.211718, 0.530316},
        {0.959269, 0.596947, 0.398884},
        {0.987053, 0.991438, 0.749504}
    };
    return get_colormap_value(colormap, value);
}

/// @brief Map a scalar value to a color using the Cividis colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color cividis(float value) {
    static const Color colormap[] = {
        {0.000000, 0.135112, 0.304751},
        {0.229739, 0.322361, 0.545706},
        {0.503385, 0.516162, 0.516759},
        {0.799329, 0.677704, 0.377779},
        {0.940015, 0.975158, 0.131326}
    };
    return get_colormap_value(colormap, value);
}

/// @brief Map a scalar value to a color using the Bone colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color bone(float value) {
    static const Color colormap[] = {
        {0.000000, 0.000000, 0.000000},
        {0.062745, 0.062745, 0.129411},
        {0.298039, 0.337254, 0.419607},
        {0.423529, 0.505882, 0.537254},
        {0.623529, 0.705882, 0.705882},
        {1.000000, 1.000000, 1.000000}
    };
    return get_colormap_value(colormap, value);
}

/// @brief Map a scalar value to a color using the Gray colormap.
/// @param value Scalar value in the range [0, 1].
/// @return The color corresponding to the input value.
inline Color gray(float value) {
    static const Color colormap[] = {
        {0.000000, 0.000000, 0.000000},
        {0.250980, 0.250980, 0.250980},
        {0.500000, 0.500000, 0.500000},
        {0.750980, 0.750980, 0.750980},
        {1.000000, 1.000000, 1.000000}
    };
    return get_colormap_value(colormap, value);
}

/// @brief Window function types.
enum class Function {
    Viridis = 0,
    Cividis,
    Magma,
    Plasma,
    Inferno,
    Bone,
    Gray,
    NumFunctions
};

/// @brief Compute the color for given color mapping and value.
/// @param color_map_ The color map to use when mapping values.
/// @param value The value to map to a color.
/// @returns The color from given mapping for given value.
inline Color color_map(const Function& color_map_, const float& value) {
    switch (color_map_) {
    case Function::Viridis:  return viridis(value);
    case Function::Cividis:  return cividis(value);
    case Function::Magma:    return magma(value);
    case Function::Plasma:   return plasma(value);
    case Function::Inferno:  return inferno(value);
    case Function::Bone:     return bone(value);
    case Function::Gray:     return gray(value);
    default: throw std::runtime_error("Unrecognized color_map_ input.");
    }
}

/// @brief Return the name of the given color map.
/// @param color_map_ The color map to serialize into a string representation.
/// @returns A descriptive string name for the color-map.
inline std::string name(const Function& color_map_) {
    switch (color_map_) {
    case Function::Viridis:  return "Viridis";
    case Function::Cividis:  return "Cividis";
    case Function::Magma:    return "Magma";
    case Function::Plasma:   return "Plasma";
    case Function::Inferno:  return "Inferno";
    case Function::Bone:     return "Bone";
    case Function::Gray:     return "Gray";
    default: throw std::runtime_error("Unrecognized color_map_ input.");
    }
}

}  // namespace ColorMap

}  // namespace Math

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_COLOR_MAP_HPP_
