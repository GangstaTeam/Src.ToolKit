#include "render/ImageDecoder.hh"

#include "render/Wic.hh"

namespace tk
{

IWICImagingFactory* WicFactory()
{
    static IWICImagingFactory* factory = []
    {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        IWICImagingFactory* created = nullptr;
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&created));
        return created;
    }();
    return factory;
}

std::optional<Image> DecodeImage(const Bytes& encoded)
{
    IWICImagingFactory* factory = WicFactory();

    if (!factory || encoded.empty())
    {
        return std::nullopt;
    }

    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;

    if (FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(encoded.data()), static_cast<DWORD>(encoded.size()))) ||
        FAILED(factory->CreateDecoderFromStream(stream.ptr, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) || FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.ptr,
            GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom)))
    {
        return std::nullopt;
    }

    Image image;
    converter->GetSize(&image.width, &image.height);
    image.bgra.resize(static_cast<size_t>(image.width) * image.height * 4);
    UINT stride = image.width * 4;

    if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(image.bgra.size()), image.bgra.data())))
    {
        return std::nullopt;
    }

    return image;
}

}
