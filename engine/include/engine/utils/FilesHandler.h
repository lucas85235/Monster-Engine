#include "engine/Log.h"
#include "se_pch.h"

namespace fs = std::filesystem;

static std::optional<fs::path> findAssetsFolder() {
    // 1) checar env var
    if (const char* env_p = std::getenv("ASSETS_DIR")) {
        fs::path p(env_p);
        if (fs::exists(p))
            return p;
    }

    // 2) ./assets (CWD)
    fs::path cwd_assets = fs::current_path() / "assets";
    if (fs::exists(cwd_assets))
        return cwd_assets;

    // 3) exe_dir/assets (assets colocados ao lado do binário)
    // obter exe path (simplificado: use argv[0] no main e converta para
    // canonical) aqui assumimos que 'exePath' será fornecido (veja nota abaixo)
    // exemplo: fs::path exe_dir = fs::canonical(exePath).parent_path();
    // fs::path exe_assets = exe_dir / "assets";
    // if (fs::exists(exe_assets)) return exe_assets;

    // 4) fallback fail
    return std::nullopt;
}

static std::optional<fs::path> findFolderInRoot(std::string_view location) {
    fs::path cwd_assets = fs::current_path() / location;
    if (fs::exists(cwd_assets))
        return cwd_assets;

    SE_LOG_INFO("Starting engine - build: {}", 0);
    return std::nullopt;
}

static std::string readFileToString(const fs::path& p) {
    std::ifstream ifs(p, std::ios::in | std::ios::binary);
    if (!ifs)
        throw std::runtime_error("Failed to open file: " + p.string());
    std::string content;
    ifs.seekg(0, std::ios::end);
    content.resize((size_t)ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(&content[0], content.size());
    return content;
}

static std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path.string() << std::endl;
        return {};
    }

    std::ostringstream contentStream;
    contentStream << file.rdbuf();
    return contentStream.str();
}