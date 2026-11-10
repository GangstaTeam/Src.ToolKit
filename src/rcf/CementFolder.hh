#pragma once

#include "rcf/CementArchive.hh"
#include "rcf/CementWriter.hh"

#include <filesystem>
#include <string>
#include <vector>

#define CEMENTFOLDER_MANIFEST_HEADER "cement"

#define CEMENTFOLDER_CEMENT_MANIFEST_NAME "cement.manifest"

namespace tk
{

std::filesystem::path CementEntryPath(const std::filesystem::path& root, const std::string& name);

bool ExtractCementSources(const std::vector<CementSource>& sources,
    const std::filesystem::path& root,
    const std::string& manifestFor,
    uint32_t alignment,
    const CementProgress& progress,
    std::string& error);
bool ExtractCement(CementArchive& archive,
    const std::vector<const CementEntry*>& entries,
    const std::filesystem::path& root,
    bool writeManifest,
    const CementProgress& progress,
    std::string& error);

bool CollectCementFolder(const std::filesystem::path& root,
    std::vector<CementSource>& sources,
    uint32_t& alignment,
    std::string& error);

}
