#pragma once

#include "render/ImageDecoder.hh"

#include <cstdint>
#include <optional>

namespace tk
{

std::optional<Bytes> EncodePngRgb(const Image& image, uint32_t size);

}
