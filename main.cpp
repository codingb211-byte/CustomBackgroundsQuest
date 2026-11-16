// CustomBackgrounds - rewrite for Beat Saber 1.40.6 (Scotland2 / beatsaber-hook 6.x / bs-cordl)
// Shows a 360 image (equirectangular) as a skybox sphere behind the environment.
//
// Put images (.png/.jpg/.jpeg) in:
//   /sdcard/ModData/com.beatgames.beatsaber/Mods/CustomBackgrounds/
// Optional: create selected.txt in that folder containing one filename to force that image.
// Otherwise the first image (alphabetically) is used.

#include "main.hpp"

#include "UnityEngine/Collider.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/PrimitiveType.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/Shader.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

static const std::string kDir = "/sdcard/ModData/com.beatgames.beatsaber/Mods/CustomBackgrounds/";
static constexpr float kSphereRadius = 400.0f;

static UnityW<UnityEngine::GameObject> backgroundSphere;

static std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::optional<std::string> PickImage() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(kDir, ec);

    // Explicit choice via selected.txt
    std::ifstream sel(kDir + "selected.txt");
    if (sel) {
        std::string name;
        std::getline(sel, name);
        while (!name.empty() && (name.back() == '\r' || name.back() == '\n' || name.back() == ' ')) name.pop_back();
        if (!name.empty() && fs::exists(kDir + name, ec)) return kDir + name;
    }

    std::vector<std::string> files;
    for (auto const& e : fs::directory_iterator(kDir, ec)) {
        if (!e.is_regular_file()) continue;
        auto ext = Lower(e.path().extension().string());
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") files.push_back(e.path().string());
    }
    if (files.empty()) return std::nullopt;
    std::sort(files.begin(), files.end());
    return files.front();
}

static UnityEngine::Texture2D* LoadTexture(std::string const& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return nullptr;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (data.empty()) return nullptr;

    ArrayW<uint8_t> bytes(data.size());
    std::copy(data.begin(), data.end(), bytes.begin());

    auto tex = UnityEngine::Texture2D::New_ctor(2, 2);
    if (!UnityEngine::ImageConversion::LoadImage(tex, bytes, false)) return nullptr;
    return tex;
}

static void EnsureBackground() {
    if (backgroundSphere.isAlive()) return;

    auto path = PickImage();
    if (!path) {
        PaperLogger.info("No images found in {}", kDir);
        return;
    }

    auto tex = LoadTexture(*path);
    if (!tex) {
        PaperLogger.error("Failed to load image {}", *path);
        return;
    }

    // Sprites/Default is used because it is almost certainly shipped in the game build
    // (custom unlit shaders may be stripped) and it has culling off, so the inside of the sphere is visible.
    auto shader = UnityEngine::Shader::Find("Sprites/Default");
    if (!shader) shader = UnityEngine::Shader::Find("UI/Default");
    if (!shader) {
        PaperLogger.error("No usable shader found");
        return;
    }

    auto go = UnityEngine::GameObject::CreatePrimitive(UnityEngine::PrimitiveType::Sphere);
    go->set_name("CustomBackgroundSphere");

    // No physics on a skybox
    if (auto col = go->GetComponent<UnityEngine::Collider*>()) UnityEngine::Object::Destroy(col);

    auto mat = UnityEngine::Material::New_ctor(shader);
    mat->set_mainTexture(tex);

    auto renderer = go->GetComponent<UnityEngine::MeshRenderer*>();
    renderer->set_material(mat);

    // Negative X scale mirrors the image so it reads correctly from the inside
    go->get_transform()->set_localScale({-kSphereRadius, kSphereRadius, kSphereRadius});
    go->get_transform()->set_position({0.0f, 0.0f, 0.0f});

    UnityEngine::Object::DontDestroyOnLoad(go);
    backgroundSphere = go;

    PaperLogger.info("Background created from {}", *path);
}

MAKE_HOOK_MATCH(
    SceneManager_ActiveSceneChanged,
    &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged,
    void,
    UnityEngine::SceneManagement::Scene prevScene,
    UnityEngine::SceneManagement::Scene nextScene
) {
    SceneManager_ActiveSceneChanged(prevScene, nextScene);
    EnsureBackground();
}

extern "C" void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(PaperLogger.tag);
}

extern "C" void late_load() noexcept {
    il2cpp_functions::Init();
    INSTALL_HOOK(PaperLogger, SceneManager_ActiveSceneChanged);
    PaperLogger.info("CustomBackgrounds loaded");
}
