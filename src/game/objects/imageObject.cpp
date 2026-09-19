#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/EditorUI.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include "../../../include/main.hpp"
#include "../../../include/impl.hpp"
#include <fstream>
#include <vector>

using namespace geode::prelude;
using namespace GameObjectsFactory;

static const char* B64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(std::vector<uint8_t> const& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(B64_TABLE[(n >> 18) & 63]);
        out.push_back(B64_TABLE[(n >> 12) & 63]);
        out.push_back(B64_TABLE[(n >> 6) & 63]);
        out.push_back(B64_TABLE[n & 63]);
        i += 3;
    }
    if (i + 1 == data.size()) {
        uint32_t n = data[i] << 16;
        out.push_back(B64_TABLE[(n >> 18) & 63]);
        out.push_back(B64_TABLE[(n >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (i + 2 == data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
        out.push_back(B64_TABLE[(n >> 18) & 63]);
        out.push_back(B64_TABLE[(n >> 12) & 63]);
        out.push_back(B64_TABLE[(n >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

static int b64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static std::vector<uint8_t> base64Decode(std::string const& in) {
    std::vector<uint8_t> out;
    out.reserve(in.size() * 3 / 4);
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        int d = b64Value(c);
        if (d < 0) continue;
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static matjson::Value getObjectData(GameObject* obj) {
    if (auto data = obj->getUserObject("image-data")) {
        if (auto str = typeinfo_cast<CCString*>(data)) {
            auto res = matjson::parse(str->getCString());
            if (res) return res.unwrap();
        }
    }
    return matjson::Value::object();
}

static void setObjectData(GameObject* obj, matjson::Value const& data) {
    obj->setUserObject("image-data", CCString::create(data.dump()));
}

static std::string getRawIcon(GameObject* obj) {
    auto data = getObjectData(obj);
    if (data.contains("rawIcon") && data["rawIcon"].isString()) {
        return data["rawIcon"].asString().unwrapOr("");
    }
    return "";
}

static void setRawIcon(GameObject* obj, std::string const& b64) {
    auto data = getObjectData(obj);
    data["rawIcon"] = b64;
    setObjectData(obj, data);
}

static CCSpriteFrame* tryGetSpriteFrame(GameObject* obj) {
    std::string b64 = getRawIcon(obj);
    if (b64.empty()) return nullptr;

    auto bytes = base64Decode(b64);
    if (bytes.size() < 8) return nullptr;
    if (bytes[0] != 0x89 || bytes[1] != 0x50 || bytes[2] != 0x4E || bytes[3] != 0x47)
        return nullptr;

    auto img = new CCImage();
    bool ok = img->initWithImageData(
        const_cast<uint8_t*>(bytes.data()),
        static_cast<int>(bytes.size())
    );
    if (!ok) {
        img->release();
        return nullptr;
    }

    auto tex = new CCTexture2D();
    if (!tex->initWithImage(img)) {
        tex->release();
        img->release();
        return nullptr;
    }
    img->release();
    tex->autorelease();

    auto size = tex->getContentSize();
    return CCSpriteFrame::createWithTexture(
        tex,
        CCRect{ 0.f, 0.f, size.width, size.height }
    );
}

static void trySetupCustomSprite(GameObject* obj) {
    if (!obj) return;

    if (auto frame = tryGetSpriteFrame(obj)) {
        for (auto c : obj->getChildrenExt()) {
            c->setVisible(false);
        }
        obj->removeChildByTag("image"_h);

        obj->setContentSize({ 30.f, 30.f });

        auto image = CCSprite::createWithSpriteFrame(frame);
        limitNodeSize(image, obj->getContentSize(), 1337.f, 0.f);
        image->setPosition(obj->getContentSize() / 2);
        image->setColor(obj->getColor());
        image->setOpacity(obj->getOpacity());
        obj->addChild(image, 1, "image"_h);
    }
    else {
        for (auto c : obj->getChildrenExt()) {
            c->setVisible(true);
        }
        obj->removeChildByTag("image"_h);
    }

    obj->m_width = obj->getContentWidth();
    obj->m_height = obj->getContentHeight();
    obj->updateOrientedBox();
}

static std::optional<std::string> readFileAsBase64(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;

    file.seekg(0, std::ios::end);
    auto len = file.tellg();
    if (len <= 0) return std::nullopt;
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes(static_cast<size_t>(len));
    file.read(reinterpret_cast<char*>(bytes.data()), len);
    if (!file) return std::nullopt;

    if (bytes.size() < 8 ||
        bytes[0] != 0x89 || bytes[1] != 0x50 || bytes[2] != 0x4E || bytes[3] != 0x47) {
        return std::nullopt;
    }

    return base64Encode(bytes);
}

class ImageSelectPopup : public Popup {
protected:
    Ref<GameObject> m_target;
    CCLabelBMFont* m_statusLabel = nullptr;

    bool init(GameObject* obj) {
        if (!Popup::init(300.f, 170.f, "GJ_square02.png"))
            return false;

        m_target = obj;
        this->setTitle("Image Object");

        auto winSize = m_mainLayer->getContentSize();

        bool hasImage = !getRawIcon(obj).empty();
        m_statusLabel = CCLabelBMFont::create(
            hasImage ? "Custom image loaded" : "Not image selected :(",
            "chatFont.fnt"
        );
        m_statusLabel->setScale(0.45f);
        m_statusLabel->setPosition({ winSize.width / 2.f, winSize.height / 2.f + 28.f });
        m_mainLayer->addChild(m_statusLabel);

        auto chooseBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Choose PNG", "goldFont.fnt", "GJ_button_01.png", 0.8f),
            this,
            menu_selector(ImageSelectPopup::onChoose)
        );

        auto resetBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Reset", "goldFont.fnt", "GJ_button_06.png", 0.8f),
            this,
            menu_selector(ImageSelectPopup::onReset)
        );

        auto menu = CCMenu::create();
        menu->setPosition({ winSize.width / 2.f, winSize.height / 2.f - 20.f });
        chooseBtn->setPosition({ 0.f, 20.f });
        resetBtn->setPosition({ 0.f, -25.f });
        menu->addChild(chooseBtn);
        menu->addChild(resetBtn);
        m_mainLayer->addChild(menu);

        return true;
    }

    void onChoose(CCObject*) {
        file::FilePickOptions options;
        options.filters = {
            file::FilePickOptions::Filter{
                .description = "PNG Images",
                .files = { "*.png" }
            }
        };

        async::spawn(
            file::pick(file::PickMode::OpenFile, options),
            [this](Result<std::optional<std::filesystem::path>> result) {
                if (!result.isOk()) return;
                auto opt = result.unwrap();
                if (!opt) return;

                auto b64 = readFileAsBase64(*opt);
                if (!b64) {
                    if (m_statusLabel)
                        m_statusLabel->setString("Error: not a valid PNG");
                    return;
                }

                setRawIcon(m_target, *b64);
                trySetupCustomSprite(m_target);

                if (m_statusLabel)
                    m_statusLabel->setString("Custom PNG loaded");
            }
        );
    }

    void onReset(CCObject*) {
        setRawIcon(m_target, "");
        trySetupCustomSprite(m_target);
        if (m_statusLabel)
            m_statusLabel->setString("Not image selected :(");
    }

public:
    static ImageSelectPopup* create(GameObject* obj) {
        auto ret = new ImageSelectPopup();
        if (ret->init(obj)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

void registerImageObject() {
    auto config = createObjectConfig(
        UNIQ_ID("image-object"),
        "unImagedBlock.png"_spr,
        [](GameObject* obj) {
            trySetupCustomSprite(obj);
        }, 914
    );

    config->objectFromVector(
        [](GameObject* obj, gd::vector<gd::string>&, gd::vector<void*>&, GJBaseGameLayer*, bool) {
            queueInMainThread([obj]() {
                trySetupCustomSprite(obj);
            });
            
            return obj;
        }
    );

    config->onEditObjectSpecial(
        [](EditorUI*, GameObject* obj) -> bool {
            if (auto popup = ImageSelectPopup::create(obj)) {
                popup->show();
            }

            return true;
        }
    );

    config->resetObject(
        [](GameObject* obj) {
            trySetupCustomSprite(obj);
        }
    );

    config->registerMe();
}

$execute {
    registerImageObject();
}