#include "ModManager.h"

using namespace geode::prelude;

#include <Geode/modify/AccountLayer.hpp>
class $modify(MyAccountLayer, AccountLayer) {
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
        async::spawn(
            utils::file::pick(utils::file::PickMode::OpenFolder, {}),
            [self = WeakRef<MyAccountLayer>(this)](arc::Result<std::optional<std::filesystem::path>> result) {
                auto locked = self.lock();
                if (!locked)
                    return;
                
                // Handle if the result is not Ok for some reason
                if (!result.ok()) {
                    log::error("Unknown error: {}", result.err());
                    FLAlertLayer::create(
                        "Oops!",
                        "An <cr>Unknown error</c> occured. Please try again.",
                        "OK"
                    )->show();
                    return;
                }

                auto exportDirectory = result.unwrap();

                // If no folder was selected don't go further
                if (!exportDirectory)
                    return;

                if (!ModManager::DisableWarning) {
                    // Just a warning popup for the player to acknowledge the risks of
                    // sharing the save files
                    createQuickPopup(
                        "Warning",
                        "These files contain <cr>sensitive information</c> (such as <co>your password</c>); do <cr>not</c> share them to <cr>anyone</c> unless you're absolutely <co>sure of what you're doing</c>.\n<co>Are you sure you want to proceed?</c>",
                        "Cancel", "Save",
                        [locked, exportDirectory](auto, bool btn2) {
                            if (btn2)
                                locked->doExport(*exportDirectory);
                        }
                    );
                } else {
                    locked->doExport(*exportDirectory);
                }
            }
        );
    };

    void doExport(std::filesystem::path exportDirectory) {
        if (!m_fields->m_alreadySaved) {
            ModManager::GameManager->save();
            ModManager::LocalLevelManager->save();

            m_fields->m_alreadySaved = true;
        }

        // Iterates over CCGameManager.dat & CCLocalLevels.dat, more can always be
        // added in the future
        for (auto& [filename, filepath] : ModManager::SaveFiles) {
            // Try reading the save file
            auto readResult = utils::file::readBinary(filepath);
            if (!readResult.ok()) {
                log::error("Failed to get {}'s data: {}", filename, readResult.err());
                FLAlertLayer::create(
                    "Oops!",
                    fmt::format(
                        "Could not get <co>{}</c>'s data!", filename
                    ),
                    "OK"
                )->show();
                return;
            }

            // Try writing the save file to the export destination
            auto writeResult = utils::file::writeBinary(
                exportDirectory / filename,
                readResult.unwrap()
            );
            if (!writeResult.isOk()) {
                log::error("Failed to export {}: {}", filename, writeResult.err());
                FLAlertLayer::create(
                    "Oops!",
                    fmt::format(
                        "Could not export <co>{}</c>!", filename
                    ),
                    "OK"
                )->show();
                return;
            }

            log::debug("Exported {}!", filename);
        }

        createQuickPopup(
            "Success!",
            fmt::format("Exported <cb>CCGameManager.dat</c> & <cb>CCLocalLevels.dat</c> to <cl>{}</c>!", exportDirectory),
            "Open Folder", "OK",
            [exportDirectory](auto, bool btn2) {
                if (!btn2)
                    utils::file::openFolder(exportDirectory);
            }
        );
        log::info("Exported save files to {}!", exportDirectory);
    };
};