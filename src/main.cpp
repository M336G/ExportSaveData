#include <Geode/modify/AccountLayer.hpp>

class $modify(MyAccountLayer, AccountLayer) {
    struct Fields {
        const std::filesystem::path m_saveDirectory = geode::dirs::getSaveDir();

        const std::unordered_map<std::string, std::filesystem::path> m_saveFiles = {
            { "CCGameManager.dat", m_saveDirectory / "CCGameManager.dat" },
            { "CCLocalLevels.dat", m_saveDirectory / "CCLocalLevels.dat" }
        };
    };

    void customSetup() {
        AccountLayer::customSetup();

        if (!this->m_isLoggedIn)
            return;

        auto *layer = this->m_mainLayer;

        auto *menu = cocos2d::CCMenu::create();
        menu->setID("export-menu"_spr);

        auto *button = CCMenuItemSpriteExtra::create(
            cocos2d::CCSprite::createWithSpriteFrameName("GJ_duplicateBtn_001.png"),
            this,
            menu_selector(MyAccountLayer::onExportButton)
        );
        button->setID("export-button"_spr);
        menu->addChild(button);

        // https://github.com/HJfod/Backups/blob/682c9b0c97cdd9a8ae1463853fb488791a93e48d/src/main.cpp#L151C1-L166C1
		menu->setAnchorPoint({ .5f, 0 });
        menu->setLayout(geode::ColumnLayout::create()->setAxisAlignment(geode::AxisAlignment::Start));
		layer->addChildAtPosition(menu, geode::Anchor::BottomRight, { -100, 20 }, false);
    };

    void showError(std::string logMessage, std::string alertMessage) {
        geode::log::error("{}", logMessage);
        FLAlertLayer::create("Oops!", alertMessage, "OK")->show();
    };

    void onExportButton(CCObject *) {
        geode::async::spawn(
            geode::utils::file::pick(geode::utils::file::PickMode::OpenFolder, {}),
            [this](arc::Result<std::optional<std::filesystem::path>> result) {
                // Handle if the result is not Ok for some reason
                if (!result.ok()) {
                    showError(
                        fmt::format("Unknown error: {}", result.unwrapErr()),
                        "<cr>Unknown error</c>"
                    );
                    return;
                }

                auto exportDirectory = result.unwrap();

                // If no folder was selected don't go further
                if (!exportDirectory)
                    return;

                // Go straight to doExport() if the warning popup is disabled
                if (geode::Mod::get()->getSettingValue<bool>("disable-warning")) {
                    doExport(*exportDirectory);
                    return;
                }

                // Just a warning popup for the player to acknowledge the risks of
                // sharing the save files
                geode::createQuickPopup(
                    "Warning",
                    "These files contain <cr>sensitive information</c> (such as <co>your password</c>); do <cr>not</c> share them to <cr>anyone</c> unless you're absolutely <co>sure of what you're doing</c>.\n<co>Are you sure you want to proceed?</c>",
                    "Cancel", "Save",
                    [this, exportDirectory](auto, bool btn2) {
                        if (btn2)
                            doExport(*exportDirectory);
                    }
                );
            }
        );
    };

    void doExport(std::filesystem::path exportDirectory) {
        // Iterates over CCGameManager.dat & CCLocalLevels.dat, more can always be added in
        // the future (check the map m_fields->m_saveFiles above)
        for (auto& [filename, filepath] : m_fields->m_saveFiles) {
            // Try reading the save file
            auto data = geode::utils::file::readBinary(filepath);
            if (!data.ok()) {
                showError(
                    fmt::format("Failed to get {}'s data: {}", filename, data.unwrapErr()),
                    fmt::format("Failed to get <co>{}</c>'s data!", filename)
                );
                return;
            }

            // Try writing the save file to the export destination
            auto writeResult = geode::utils::file::writeBinary(
                exportDirectory / filename,
                data.unwrap()
            );
            if (!writeResult.isOk()) {
                showError(
                    fmt::format("Failed to export {}'s data: {}", filename, writeResult.unwrapErr()),
                    fmt::format("Failed to export <co>{}</c>'s data!", filename)
                );
                return;
            }
        }

        FLAlertLayer::create(
            "Success!",
            fmt::format(
                "Exported <cb>CCGameManager.dat</c> & <cb>CCLocalLevels.dat</c> to <cl>{}</c>!",
                exportDirectory
            ),
            "OK"
        )->show();
    };
};