#pragma once

#include "render/ImageDecoder.hh"
#include "rig/RigBuilder.hh"
#include "rig/RigModel.hh"
#include "rig/SourceModel.hh"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace tk
{

class ImportSession
{
public:
    bool Load(const std::filesystem::path& path, std::string& error);
    bool LoadTexture(const std::filesystem::path& path, std::string& error);
    bool Loaded() const { return m_loaded; }
    const std::filesystem::path& SourcePath() const { return m_path; }
    const SourceModel& Source() const { return m_source; }
    const RigModel& Model() const { return m_model; }
    const Image& Texture() const { return m_texture; }
    uint64_t Revision() const { return m_revision; }
    uint64_t ModelRevision() const { return m_modelRevision; }
    uint64_t TextureRevision() const { return m_textureRevision; }

    float Height() const { return m_model.height; }
    void SetHeight(float height);
    void Rotate(int axis, int quarterTurns);
    void ResetOrientation();

    RigRequest& Request() { return m_request; }
    const RigRequest& Request() const { return m_request; }
    void MoveMarker(Marker marker, float x, float y, bool symmetric);
    void GuessMarkers();
    void ClearMarkers();
    bool MarkerPlaced(Marker marker) const { return m_request.placed[static_cast<size_t>(marker)]; }
    bool AllMarkersPlaced() const;

    bool Build(const GameSkeleton& base, std::string& error);
    const std::optional<RiggedCharacter>& Rig() const { return m_rig; }
    RiggedCharacter* MutableRig() { return m_rig ? &*m_rig : nullptr; }
    void SetRig(std::optional<RiggedCharacter> rig);
    void DiscardRig();

private:
    void Prepare();

    std::filesystem::path m_path;
    SourceModel m_source;
    RigModel m_model;
    Image m_texture;
    Orientation m_orientation;
    RigRequest m_request;
    std::optional<RiggedCharacter> m_rig;
    uint64_t m_revision = 0;
    uint64_t m_modelRevision = 0;
    uint64_t m_loadCount = 0;
    uint64_t m_textureRevision = 0;
    bool m_loaded = false;
};

}
