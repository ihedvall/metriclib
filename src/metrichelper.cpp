/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <format>
#include "metrichelper.h"

namespace metric {
std::string FloatToString(float value) {
  return std::format("{}", value);
}

std::string DoubleToString(double value) {
  return std::format("{}", value);
}
}  // namespace metric