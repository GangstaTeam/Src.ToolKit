#pragma once

#include "core/Bytes.hh"

#include <cstdint>
#include <optional>

namespace tk
{

struct Image
{
    uint32_t width = 0;
    uint32_t height = 0;
    Bytes bgra;
};

std::optional<Image> DecodeImage(const Bytes& encoded);

}
