#include <sumire/gui/prototypes/gui/panels/presets/import_fbs_presets_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/ids/preset_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <format>

namespace kbf {

    ImportFbsPresetsPanel::ImportFbsPresetsPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {
        initializeBuffers();
		presetLoadFailed = !getPresetsFromFBS(); // TODO: Maybe thread this.
    }

    void ImportFbsPresetsPanel::initializeBuffers() {
        std::strcpy(presetBundleBuffer, bundleName.c_str());
    }

    bool ImportFbsPresetsPanel::draw() {
        bool open = true;
        processFocus();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        if (presetLoadFailed) {
            const char* failureStr = "Failed to load FBS presets.";
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(failureStr).x) * 0.5f);
            ImGui::Text(failureStr);
            ImGui::PopStyleColor();
            ImGui::BeginDisabled();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        ImGui::InputText(" Bundle Name ", presetBundleBuffer, IM_ARRAYSIZE(presetBundleBuffer));
        bundleName = std::string{ presetBundleBuffer };
        ImGui::SetItemTooltip("Enables sorting similar presets under one title");

        ImGui::Spacing();
        std::string sexComboValue = presetsFemale ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                presetsFemale = false;
            }
            if (ImGui::Selectable("Female")) {
                presetsFemale = true;
            };
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

		static bool autoswitchPresetsOnly = true;
        ImGui::Checkbox("Import Autoswitch Presets Only", &autoswitchPresetsOnly);

        ImGui::Spacing();

        drawPresetList(presets, autoswitchPresetsOnly, presetsFemale);

        ImGui::Spacing();
        ImGui::Spacing();

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kCreateLabel = "Create";

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = ImGui::CalcTextSize(kCancelLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = ImGui::CalcTextSize(kCreateLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + spacing;

        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (presetLoadFailed) ImGui::EndDisabled();

        if (ImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        if (presetLoadFailed) ImGui::BeginDisabled();

        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        const bool bundleEmpty = bundleName.empty();
        if (bundleEmpty) ImGui::BeginDisabled();
        if (ImGui::Button(kCreateLabel)) {
			std::vector<Preset> presetsToCreate = createPresetList(autoswitchPresetsOnly);
            INVOKE_REQUIRED_CALLBACK(createCallback, presetsToCreate);
        }
        if (bundleEmpty) ImGui::EndDisabled();
        if (bundleEmpty) ImGui::SetItemTooltip("Please provide a bundle name");

        if (presetLoadFailed) ImGui::EndDisabled();

        float contentHeight = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
        ImVec2 newSize = ImVec2(width, contentHeight);
        ImGui::SetWindowSize(newSize);

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    bool ImportFbsPresetsPanel::getPresetsFromFBS() {
		return dataManager.getFBSpresets(&presets);
    }

    void ImportFbsPresetsPanel::drawPresetList(const std::vector<FBSPreset>& presets, bool autoSwitchOnly, const bool female) {
        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("PresetGroupChild", ImVec2(0, 250), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        const float selectableHeight = ImGui::GetTextLineHeight();

        if (presets.size() == 0) {
            const char* noneFoundStr = "No FBS Presets Found";
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            ImGui::Text(noneFoundStr);
            ImGui::PopStyleColor();
		}

        for (const FBSPreset& fbspreset : presets)
        {
			const Preset& preset = fbspreset.preset;
			bool autoSwitch = fbspreset.autoswitchingEnabled;

			if (!autoSwitch && autoSwitchOnly) continue;

            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::Selectable(("##Selectable_Preset_" + preset.name).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

            // Sex Mark
            std::string sexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImGui::PushFont(wsSymbolFont);

            constexpr float sexMarkerSpacingAfter = 5.0f;
            constexpr float sexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
            ImVec2 sexMarkerPos;
            sexMarkerPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
            ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

            ImGui::PopFont();

            ImGui::PushFont(dataManager.getRegularFontOverride());

            // Group name... floating because imgui has no vertical alignment STILL :(
            constexpr float playerNameSpacingAfter = 5.0f;
            ImVec2 playerNameSize = ImGui::CalcTextSize(preset.name.c_str());
            ImVec2 playerNamePos;
            playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset.name.c_str());

            ImGui::PopFont();

            // Legs Mark
            constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
            constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
            constexpr float armourVerticalAlignOffset = 2.5f;
            ImGui::PushFont(wsSymbolFont);

            ImVec2 legMarkSize = ImGui::CalcTextSize(WS_FONT_LEGS);
            ImVec2 legMarkPos;
            legMarkPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
            legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;

            ImGui::PushStyleColor(ImGuiCol_Text, preset.hasLegs() ? armourPresentCol : armourMissingCol);
            ImGui::GetWindowDrawList()->AddText(legMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
            ImGui::PopStyleColor();

            // Body Mark
            ImVec2 bodyMarkSize = ImGui::CalcTextSize(WS_FONT_BODY);
            ImVec2 bodyMarkPos;
            bodyMarkPos.x = legMarkPos.x - bodyMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
            bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;

            ImGui::PushStyleColor(ImGuiCol_Text, preset.hasBody() ? armourPresentCol : armourMissingCol);
            ImGui::GetWindowDrawList()->AddText(bodyMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
            ImGui::PopStyleColor();

            // Armour Sex Mark
            std::string armourSexMarkSymbol = preset.armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 armourSexMarkerCol = preset.armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 armourSexMarkerSize = ImGui::CalcTextSize(armourSexMarkSymbol.c_str());
            ImVec2 armourSexMarkerPos;
            armourSexMarkerPos.x = bodyMarkPos.x - armourSexMarkerSize.x - ImGui::GetStyle().ItemSpacing.x;
            armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
            ImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, ImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

            ImGui::PopFont();

            ImGui::PushFont(wsArmourFont);

            // Armour Name
            ImVec2 armourNameSize = ImGui::CalcTextSize(preset.armour.name.c_str());
            ImVec2 armourNamePos;
            armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - ImGui::GetStyle().ItemSpacing.x;
            armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

            ImVec4 armourNameCol = preset.armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            ImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
            ImGui::GetWindowDrawList()->AddText(armourNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset.armour.name.c_str());
            ImGui::PopStyleColor();

            ImGui::PopFont();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

    }

    std::vector<Preset> ImportFbsPresetsPanel::createPresetList(bool autoswitchOnly) const {
        std::vector<Preset> presetsToCreate{};
        for (const FBSPreset& fbspreset : presets) {
            if (autoswitchOnly && !fbspreset.autoswitchingEnabled) continue;
            Preset processedPreset = fbspreset.preset;
            processedPreset.bundle = bundleName;
            presetsToCreate.push_back(processedPreset);
        }
        dataManager.resolveNameConflicts(presetsToCreate);

        return presetsToCreate;
    }

}