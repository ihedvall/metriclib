/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <string>
namespace metric {
/** \brief Converts a float to string without loosing precision.
 *
 * Converts a float value to a string without loosing any precision.
 * @param value Float value to convert
 * @return The float value as text
 */
[[nodiscard]] std::string FloatToString(float value);

/** \brief Converts a double to string without loosing precision.
 *
 * Converts a double value to a string without loosing any precision.
 * @param value Double value to convert
 * @return The double value as text
 */
[[nodiscard]] std::string DoubleToString(double value);
}  // namespace metric

