#include "rig/ImportSession.hh"

#include "rig/BoneMapping.hh"
#include "core/FileIo.hh"
#include "rig/GltfImport.hh"

#include <algorithm>

namespace tk
{

bool ImportSession::Load(const std::filesystem::path& path, std::string& error)
{
    auto source = ImportGltf(path, error);

    if (!source)
    {
        return false;
    }

    m_path = path;
    m_source = std::move(*source);
    m_texture = DecodeImage(m_source.texture).value_or(Image());
    m_orientation = {};
    m_request = {};
    m_request.source = m_source.HasSkin() ? RigSource::ModelSkeleton : RigSource::Markers;
    m_request.mapping = GuessBoneMapping(m_source.bones);
    m_loaded = true;
    ++m_textureRevision;
    Prepare();
    GuessMarkers();
    ClearMarkers();
    return true;
}

bool ImportSession::LoadTexture(const std::filesystem::path& path, std::string& error)
{
    std::string extension = path.extension().u8string();
    bool model = extension == ".glb" || extension == ".gltf" || extension == ".GLB" || extension == ".GLTF";
    std::optional<Bytes> encoded = model ? ImportGltf(path, error).value_or(SourceModel()).texture : ReadFile(path);
    std::optional<Image> image = encoded ? DecodeImage(*encoded) : std::nullopt;

    if (!image)
    {
        if (error.empty())
        {
            error = "no image found in " + path.filename().u8string();
        }

        return false;
    }

    m_texture = std::move(*image);
    ++m_textureRevision;
    return true;
}

void ImportSession::SetHeight(float height)
{
    float clamped = std::clamp(height, 0.2f, 10.0f);

    if (clamped == m_model.height)
    {
        return;
    }

    float ratio = clamped / m_model.height;
    m_model.height = clamped;
    Prepare();

    for (MarkerPoint& marker : m_request.markers)
    {
        marker = {marker.x * ratio, marker.y * ratio};
    }
}

void ImportSession::Rotate(int axis, int quarterTurns)
{
    m_orientation.quarterTurns[axis] = (m_orientation.quarterTurns[axis] + quarterTurns + 4) % 4;
    Prepare();
    GuessMarkers();
    ClearMarkers();
}

void ImportSession::ResetOrientation()
{
    m_orientation = {};
    Prepare();
    GuessMarkers();
    ClearMarkers();
}

void ImportSession::MoveMarker(Marker marker, float x, float y, bool symmetric)
{
    bool centre = MirrorOf(marker) == marker;
    m_request.markers[static_cast<size_t>(marker)] = {symmetric && centre ? 0.0f : x, y};
    m_request.placed[static_cast<size_t>(marker)] = true;

    if (symmetric && !centre)
    {
        m_request.markers[static_cast<size_t>(MirrorOf(marker))] = {-x, y};
        m_request.placed[static_cast<size_t>(MirrorOf(marker))] = true;
    }

    DiscardRig();
}

void ImportSession::GuessMarkers()
{
    m_request.markers = tk::GuessMarkers(m_model);

    for (size_t i = 0; i < kMarkerCount; ++i)
    {
        m_request.placed[i] = !IsOptional(static_cast<Marker>(i));
    }

    DiscardRig();
}

void ImportSession::ClearMarkers()
{
    m_request.placed.fill(false);
    DiscardRig();
}

bool ImportSession::AllMarkersPlaced() const
{
    for (size_t i = 0; i < kMarkerCount; ++i)
    {
        if (!m_request.placed[i] && !IsOptional(static_cast<Marker>(i)))
        {
            return false;
        }
    }

    return true;
}

void ImportSession::SetRig(std::optional<RiggedCharacter> rig)
{
    m_rig = std::move(rig);
    ++m_revision;
}

bool ImportSession::Build(const GameSkeleton& base, std::string& error)
{
    m_rig = BuildRig(m_model, m_request, base, error);
    ++m_revision;
    return m_rig.has_value();
}

void ImportSession::DiscardRig()
{
    if (!m_rig)
    {
        return;
    }

    m_rig.reset();
    ++m_revision;
}

void ImportSession::Prepare()
{
    m_model = PrepareModel(m_source, m_orientation, m_model.height);
    m_rig.reset();
    ++m_revision;
    ++m_modelRevision;
}

}
