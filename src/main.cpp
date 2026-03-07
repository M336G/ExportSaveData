#include <Geode/modify/AccountLayer.hpp>
#include "ModManager.h"

using namespace geode::prelude;

class $modify(MyAccountLayer, AccountLayer) {
    enum ExportMode {
        Account,
        Levels,
        Both
    };

    class ExportPopup : public Popup {
    protected:
        MyAccountLayer* m_layer;

        bool init(MyAccountLayer* layer) {
            if (!Popup::init(360.f, 200.f))
                return false;

            m_layer = layer;

            setTitle("Export Data");

            auto* label = CCLabelBMFont::create("What do you want to export?", "bigFont.fnt");
            label->setScale(0.45f);
            m_mainLayer->addChildAtPosition(label, Anchor::Center, { 0, 45 });

            auto* accountButton = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("Account"),
                this,
                menu_selector(ExportPopup::onAccount)
            );
            m_buttonMenu->addChildAtPosition(accountButton, Anchor::Center, { -60, 0 });

            auto* levelsButton = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("Levels"),
                this,
                menu_selector(ExportPopup::onLevels)
            );
            m_buttonMenu->addChildAtPosition(levelsButton, Anchor::Center, { 60, 0 });

            auto* bothButton = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("Both"),
                this,
                menu_selector(ExportPopup::onBoth)
            );
            m_buttonMenu->addChildAtPosition(bothButton, Anchor::Center, { 0, -40 });

            return true;
        };

        void onAccount(CCObject* sender) {
            this->onClose(sender);
            m_layer->doExport(ExportMode::Account);
        };

        void onLevels(CCObject* sender) {
            this->onClose(sender);
            m_layer->doExport(ExportMode::Levels);
        };

        void onBoth(CCObject* sender) {
            this->onClose(sender);
            m_layer->doExport(ExportMode::Both);
        };

    public:
        static ExportPopup* create(MyAccountLayer* layer) {
            auto ret = new ExportPopup();
            if (ret->init(layer)) {
                ret->autorelease();
                return ret;
            }

            delete ret;
            return nullptr;
        };
    };

    struct Fields {
        bool m_alreadySaved = false;
    };

    void customSetup() {
        AccountLayer::customSetup();

        // Don't do anything if the user isn't logged in in the first place
        if (!m_isLoggedIn)
            return;

        auto* menu = CCMenu::create();
        menu->setID("export-menu"_spr);

        auto* button = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_duplicateBtn_001.png"),
            this,
            menu_selector(MyAccountLayer::onExport));
        button->setID("export-button"_spr);
        menu->addChild(button);

        // https://github.com/HJfod/Backups/blob/682c9b0c97cdd9a8ae1463853fb488791a93e48d/src/main.cpp#L151C1-L166C1
        menu->setAnchorPoint({ .5f, 0 });
        menu->setLayout(ColumnLayout::create()->setAxisAlignment(AxisAlignment::Start));
        m_mainLayer->addChildAtPosition(menu, Anchor::BottomRight, { -100, 20 }, false);
    };

    void onExport(CCObject*) {
        ExportPopup::create(this)->show();
    };

    void doExport(ExportMode mode) {
        async::spawn(
            utils::file::pick(utils::file::PickMode::OpenFolder, {}),
            [self = Ref<MyAccountLayer>(this), mode](arc::Result<std::optional<std::filesystem::path>> result) {
                // Handle if the result is not Ok for some reason
                if (!result.isOk()) {
                    log::error("Unknown error: {}", result.unwrapErr());
                    FLAlertLayer::create("Oops!", "An <cr>Unknown error</c> occured. Please try again.", "OK")->show();
                    return;
                }

                auto exportDirectory = result.unwrap();

                // If no folder was selected don't go further
                if (!exportDirectory)
                    return;

                auto doSaveAndExport = [self, exportDirectory, mode]() {
                    if (!self->m_fields->m_alreadySaved) {
                        ModManager::GameManager->save();
                        ModManager::LocalLevelManager->save();
                        self->m_fields->m_alreadySaved = true;
                    }

                    // Iterates over CCGameManager.dat & CCLocalLevels.dat (then it'll
                    // check which one(s) are selected)
                    for (auto& [filename, filepath] : ModManager::SaveFiles) {
                        if (mode == ExportMode::Account && filename != "CCGameManager.dat")
                            continue;
                        if (mode == ExportMode::Levels && filename != "CCLocalLevels.dat")
                            continue;

                        // Try reading the save file
                        auto readResult = utils::file::readBinary(filepath);
                        if (!readResult.isOk()) {
                            log::error("Failed to get {}'s data: {}", filename, readResult.unwrapErr());
                            FLAlertLayer::create("Oops!", fmt::format("Could not get <co>{}</c>'s data!", filename), "OK")->show();
                            return;
                        }

                        // Try writing the save file to the export destination
                        auto writeResult = utils::file::writeBinary(*exportDirectory / filename, readResult.unwrap());
                        if (!writeResult.isOk()) {
                            log::error("Failed to export {}: {}", filename, writeResult.unwrapErr());
                            FLAlertLayer::create("Oops!", fmt::format("Could not export <co>{}</c>!", filename), "OK")->show();
                            return;
                        }

                        log::debug("Exported {}!", filename);
                    }

                    createQuickPopup(
                        "Success!",
                        fmt::format("Successfully exported to <cl>{}</c>!", *exportDirectory),
                        "Open Folder", "OK",
                        [exportDirectory](auto, bool btn2) {
                            if (!btn2)
                                utils::file::openFolder(*exportDirectory);
                        }
                    );
                    log::info("Exported save files to {}!", *exportDirectory);
                };

                if (!ModManager::DisableWarning) {
                    // Just a warning popup for the player to acknowledge the risks of
                    // sharing the save files
                    createQuickPopup(
                        "Warning",
                        "These files contain <cr>sensitive information</c> (such as <co>your password</c>); do <cr>not</c> share them to <cr>anyone</c> unless you're absolutely <co>sure of what you're doing</c>.\n<co>Are you sure you want to proceed?</c>",
                        "Cancel", "Save",
                        [doSaveAndExport](auto, bool btn2) {
                            if (btn2)
                                doSaveAndExport();
                        }
                    );
                } else {
                    doSaveAndExport();
                }
            }
        );
    };
};