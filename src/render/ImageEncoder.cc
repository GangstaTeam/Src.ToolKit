#include "render/ImageEncoder.hh"

#include "render/Wic.hh"

#include <objidl.h>

namespace tk
{

namespace
{

bool WritePng(IWICImagingFactory* factory, IWICBitmapSource* source, uint32_t size, IStream* stream)
{
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    return SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
        SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(encoder->CreateNewFrame(&frame, &properties)) && SUCCEEDED(frame->Initialize(properties.ptr)) &&
        SUCCEEDED(frame->SetSize(size, size)) && SUCCEEDED(frame->SetPixelFormat(&format)) &&
        SUCCEEDED(frame->WriteSource(source, nullptr)) && SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit());
}

std::optional<Bytes> StreamBytes(IStream* stream)
{
    LARGE_INTEGER zero = {};
    ULARGE_INTEGER end = {};

    if (FAILED(stream->Seek(zero, STREAM_SEEK_END, &end)) || FAILED(stream->Seek(zero, STREAM_SEEK_SET, nullptr)))
    {
        return std::nullopt;
    }

    Bytes bytes(static_cast<size_t>(end.QuadPart));
    ULONG read = 0;

    if (FAILED(stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &read)) || read != bytes.size())
    {
        return std::nullopt;
    }

    return bytes;
}

}

std::optional<Bytes> EncodePngRgb(const Image& image, uint32_t size)
{
    IWICImagingFactory* factory = WicFactory();

    if (!factory || image.bgra.empty())
    {
        return std::nullopt;
    }

    ComPtr<IWICBitmap> bitmap;
    ComPtr<IWICBitmapScaler> scaler;
    ComPtr<IWICFormatConverter> converter;
    ComPtr<IStream> stream;
    UINT stride = image.width * 4;

    if (FAILED(factory->CreateBitmapFromMemory(image.width,
            image.height,
            GUID_WICPixelFormat32bppBGRA,
            stride,
            static_cast<UINT>(image.bgra.size()),
            const_cast<BYTE*>(image.bgra.data()),
            &bitmap)) ||
        FAILED(factory->CreateBitmapScaler(&scaler)) ||
        FAILED(scaler->Initialize(bitmap.ptr, size, size, WICBitmapInterpolationModeFant)) ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(scaler.ptr,
            GUID_WICPixelFormat24bppBGR,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom)) ||
        FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) || !WritePng(factory, converter.ptr, size, stream.ptr))
    {
        return std::nullopt;
    }

    return StreamBytes(stream.ptr);
}

}
