#include <sumire/gui/prototypes/gui/panels/share/export_panel.hpp>

#include <sumire/gui/prototypes/gui/shared/alignment.hpp>
#include <sumire/gui/prototypes/gui/shared/drag_selectable.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/gui/shared/tick_mark.hpp>
#include <sumire/gui/prototypes/util/io/noc_impl.hpp>

namespace kbf {

    ExportPanel::ExportPanel(
        const std::string& name,
        const std::string& strID,
        const bool archive,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID), 
        archive{ archive },
        dataManager{ dataManager },
        wsSymbolFont{ wsSymbolFont }, 
		wsArmourFont{ wsArmourFont }
    {
        initializeBuffers();
    }

    void ExportPanel::initializeBuffers() {
        std::strcpy(exportNameBuffer, exportName.c_str());
        std::strcpy(exportPathBuffer, exportPath.string().c_str());
        std::strcpy(filterBuffer, "");
    }

    bool ExportPanel::draw() {
        bool open = true;
        processFocus();
        processCallbacks();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        const char* nameLabel = archive ? " Mod Name " : " Filename ";
        ImGui::InputText(" Export Name ", exportNameBuffer, IM_ARRAYSIZE(exportNameBuffer));
        exportName = std::string{ exportNameBuffer };

        // Export Path Widget
		ImGui::InputTextWithHint("##PathInput", "Export Path...", exportPathBuffer, IM_ARRAYSIZE(exportPathBuffer), ImGuiInputTextFlags_ReadOnly);
        ImGui::SetItemTooltip("Export path");
        ImGui::SameLine();
        if (exportPathDialogOpen) ImGui::BeginDisabled();
        if (ImGui::Button("Browse")) {
            std::thread([this]() {
                std::string filepath = getExportPathFileDialog();
                if (!filepath.empty()) {
                    postToMainThread([this, filepath]() {
                        exportPath = filepath;
						std::strcpy(exportPathBuffer, exportPath.string().c_str());
                    });
                }
                exportPathDialogOpen = false;
            }).detach();
        }
        ImGui::SetItemTooltip("Set Export path");
        if (exportPathDialogOpen) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("ExportTabs")) {
            if (ImGui::BeginTabItem("Preset Groups")) {
                drawPresetGroupList();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Preset Bundles")) {
                drawPresetBundleList();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Presets")) {
                drawPresetList();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Player Override")) {
                drawPlayerOverrideList();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        static constexpr const char* kCancelLabel = "Cancel";
        const char* kCreateLabel = archive ? "Create Archive" : "Create .KBF";

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = ImGui::CalcTextSize(kCancelLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = ImGui::CalcTextSize(kCreateLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + spacing;

        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (ImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        bool nameEmpty = exportName.empty();
        bool pathEmpty = exportPath.empty();
        bool pathNotExists = pathEmpty || !std::filesystem::exists(exportPath); // TODO Don't check this each frame

        const bool disableCreateButton = nameEmpty || pathNotExists;
        if (disableCreateButton) ImGui::BeginDisabled();
        if (ImGui::Button(kCreateLabel)) {
            KBFFileData data = getKbfFileData();
			std::string filename = archive ? exportName + ".zip" : exportName + ".kbf";
			std::filesystem::path filePath = exportPath / filename;
			INVOKE_REQUIRED_CALLBACK(createCallback, filePath.string(), data);
        }
        if (disableCreateButton) ImGui::EndDisabled();
        if (nameEmpty) ImGui::SetItemTooltip("Please provide a name");
		else if (pathEmpty) ImGui::SetItemTooltip("Please provide an export path");
        else if (pathNotExists) ImGui::SetItemTooltip("Export path does not exist");

        float contentHeight = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
        ImVec2 newSize = ImVec2(width, contentHeight);
        ImGui::SetWindowSize(newSize);

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    void ExportPanel::drawPresetList() {
        std::string filterStr{ filterBuffer };

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        ImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        ImGui::Text(exportHintStr);
        ImGui::Spacing();
        ImGui::PopStyleColor();

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);
        ImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<std::string> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO& io = ImGui::GetIO();

        // Handle drag logic outside the list box
        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            dragStart = io.MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && ImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

		const std::vector<const Preset*> presets = dataManager.getPresets(filterStr, false, false, true);

        if (presets.size() == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Presets Found.";
            ImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }

        for (const Preset* preset : presets)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();

            bool selected = checkPresetSelected(preset);

            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + preset->uuid).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPreset(preset);
                else          selectPreset(preset);
                selected = !selected;
                selectedDuringDrag.insert(preset->uuid);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(preset->uuid) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                if (selected) deselectPreset(preset);
                else          selectPreset(preset);
                selected = !selected;
                selectedDuringDrag.insert(preset->uuid);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + ImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                ImGui::PopStyleColor();
            }

            drawPresetSelector(preset, pos, selectableHeight);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (ImGui::Button("Clear")) selectedPresets.clear();
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            for (const Preset* preset : presets) selectPreset(preset);
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();
    }

    void ExportPanel::drawPresetSelector(const Preset* preset, ImVec2 pos, float selectableHeight) const {
        const float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        // Sex Mark
        std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        ImGui::PushFont(wsSymbolFont);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + listPaddingX +  3.0f * ImGui::GetStyle().ItemSpacing.x;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        ImGui::PopFont();

        ImGui::PushFont(dataManager.getRegularFontOverride());

        // Group name... floating because imgui has no vertical alignment STILL :(
        constexpr float playerNameSpacingAfter = 5.0f;
        ImVec2 playerNameSize = ImGui::CalcTextSize(preset->name.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset->name.c_str());

        std::string bundleStr = "(" + preset->bundle + ")";
        ImVec2 bundleStrSize = ImGui::CalcTextSize(bundleStr.c_str());
        ImVec2 bundleStrPos;
        bundleStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
        bundleStrPos.y = pos.y + (selectableHeight - bundleStrSize.y) * 0.5f;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::GetWindowDrawList()->AddText(bundleStrPos, ImGui::GetColorU32(ImGuiCol_Text), bundleStr.c_str());
        ImGui::PopStyleColor();

        ImGui::PopFont();

        // Legs Mark
        constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
        constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr float armourVerticalAlignOffset = 2.5f;
        ImGui::PushFont(wsSymbolFont);

        ImVec2 legMarkSize = ImGui::CalcTextSize(WS_FONT_LEGS);
        ImVec2 legMarkPos;
        legMarkPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - ImGui::GetStyle().ItemSpacing.x - listPaddingX;
        legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;

        ImGui::PushStyleColor(ImGuiCol_Text, preset->hasLegs() ? armourPresentCol : armourMissingCol);
        ImGui::GetWindowDrawList()->AddText(legMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
        ImGui::PopStyleColor();

        // Body Mark
        ImVec2 bodyMarkSize = ImGui::CalcTextSize(WS_FONT_BODY);
        ImVec2 bodyMarkPos;
        bodyMarkPos.x = legMarkPos.x - bodyMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
        bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;

        ImGui::PushStyleColor(ImGuiCol_Text, preset->hasBody() ? armourPresentCol : armourMissingCol);
        ImGui::GetWindowDrawList()->AddText(bodyMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
        ImGui::PopStyleColor();

        // Armour Sex Mark
        std::string armourSexMarkSymbol = preset->armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 armourSexMarkerCol = preset->armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 armourSexMarkerSize = ImGui::CalcTextSize(armourSexMarkSymbol.c_str());
        ImVec2 armourSexMarkerPos;
        armourSexMarkerPos.x = bodyMarkPos.x - armourSexMarkerSize.x - ImGui::GetStyle().ItemSpacing.x;
        armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
        ImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, ImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

        ImGui::PopFont();

        ImGui::PushFont(wsArmourFont);

        // Armour Name
        ImVec2 armourNameSize = ImGui::CalcTextSize(preset->armour.name.c_str());
        ImVec2 armourNamePos;
        armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - ImGui::GetStyle().ItemSpacing.x;
        armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

        ImVec4 armourNameCol = preset->armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
        ImGui::GetWindowDrawList()->AddText(armourNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset->armour.name.c_str());
        ImGui::PopStyleColor();

        ImGui::PopFont();
    }

    bool ExportPanel::checkPresetSelected(const Preset* preset) const {
        return selectedPresets.find(preset->uuid) != selectedPresets.end();
	}

    void ExportPanel::selectPreset(const Preset* preset) {
        selectedPresets.insert(preset->uuid);
	}

    void ExportPanel::deselectPreset(const Preset* preset) {
        selectedPresets.erase(preset->uuid);
    }

    void ExportPanel::drawPresetGroupList() {
        std::string filterStr{ filterBuffer };

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        ImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        ImGui::Text(exportHintStr);
        ImGui::Spacing();
        ImGui::PopStyleColor();

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);
        ImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<std::string> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO& io = ImGui::GetIO();

        // Handle drag logic outside the list box
        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            dragStart = io.MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && ImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

        std::vector<const PresetGroup*> presetGroups = dataManager.getPresetGroups(filterBuffer);

        if (presetGroups.size() == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Preset Groups Found.";
            ImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }

        for (const PresetGroup* presetGroup : presetGroups)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();

            bool selected = checkPresetGroupSelected(presetGroup);

            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + presetGroup->uuid).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPresetGroup(presetGroup);
				else          selectPresetGroup(presetGroup);
                selected = !selected;
                selectedDuringDrag.insert(presetGroup->uuid);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(presetGroup->uuid) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                if (selected) deselectPresetGroup(presetGroup);
                else          selectPresetGroup(presetGroup);
                selected = !selected;
                selectedDuringDrag.insert(presetGroup->uuid);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + ImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                ImGui::PopStyleColor();
            }

            drawPresetGroupSelector(presetGroup, pos, selectableHeight);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (ImGui::Button("Clear")) selectedPresetGroups.clear();
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            for (const PresetGroup* presetGroup : presetGroups) selectPresetGroup(presetGroup);
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();
    }

    void ExportPanel::drawPresetGroupSelector(const PresetGroup* presetGroup, ImVec2 pos, float selectableHeight) const {
        const float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        // Sex Mark
        std::string sexMarkSymbol = presetGroup->female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = presetGroup->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        ImGui::PushFont(wsSymbolFont);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + listPaddingX + ImGui::GetStyle().ItemSpacing.x * 3.0f;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        ImGui::PopFont();

        // Group name
        ImVec2 presetGroupNameSize = ImGui::CalcTextSize(presetGroup->name.c_str());
        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(presetGroupNamePos, ImGui::GetColorU32(ImGuiCol_Text), presetGroup->name.c_str());

        std::string presetCountStr;
        if (presetGroup->size() == 0) presetCountStr = "Empty";
        else if (presetGroup->size() == 1) presetCountStr = "1 Preset";
        else presetCountStr = std::to_string(presetGroup->size()) + " Presets";

        ImVec2 rightTextSize = ImGui::CalcTextSize(presetCountStr.c_str());
        float hunterIdCursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - ImGui::GetStyle().ItemSpacing.x - listPaddingX;
        float hunterIdCursorPosY = ImGui::GetItemRectMin().y + (selectableHeight - rightTextSize.y) * 0.5f;;  // Same Y as the selectable item, plus vertical alignment

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), ImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
        ImGui::PopStyleColor();
    }

    bool ExportPanel::checkPresetGroupSelected(const PresetGroup* presetGroup) const {
        bool selected = selectedPresetGroups.find(presetGroup->uuid) != selectedPresetGroups.end();
        if (selected) {
            for (const auto& [_, id] : presetGroup->bodyPresets) {
                if (selectedPresets.find(id) == selectedPresets.end()) {
                    selected = false;
                    break;
                }
            }
            for (const auto& [_, id] : presetGroup->legsPresets) {
                if (selectedPresets.find(id) == selectedPresets.end()) {
                    selected = false;
                    break;
                }
            }
        }

        return selected;
    }

    void ExportPanel::selectPresetGroup(const PresetGroup* presetGroup) {
        selectedPresetGroups.insert(presetGroup->uuid);
        std::ranges::for_each(presetGroup->bodyPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
        std::ranges::for_each(presetGroup->legsPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
    }

    void ExportPanel::deselectPresetGroup(const PresetGroup* presetGroup) {
        selectedPresetGroups.erase(presetGroup->uuid);
    }

    void ExportPanel::drawPresetBundleList() {
        std::string filterStr{ filterBuffer };

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        ImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        ImGui::Text(exportHintStr);
        ImGui::Spacing();
        ImGui::PopStyleColor();

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);
        ImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<std::string> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO& io = ImGui::GetIO();

        // Handle drag logic outside the list box
        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            dragStart = io.MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && ImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

		std::vector<std::pair<std::string, size_t>> bundles = dataManager.getPresetBundlesWithCounts(filterBuffer);


        if (bundles.size() == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Bundles Found.";
            ImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }

        for (const auto& bundleInfo : bundles)
        {
			std::vector<std::string> bundleIds = dataManager.getPresetsInBundle(bundleInfo.first);

			std::string bundleName = bundleInfo.first;
            size_t bundleCount     = bundleInfo.second;

            ImVec2 pos = ImGui::GetCursorScreenPos();

			bool selected = checkPresetBundleSelected(bundleIds);

            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + bundleName).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPresetBundle(bundleIds);
                else          selectPresetBundle(bundleIds);
                selected = !selected;
                selectedDuringDrag.insert(bundleName);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(bundleName) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                if (selected) deselectPresetBundle(bundleIds);
                else          selectPresetBundle(bundleIds);
                selected = !selected;
                selectedDuringDrag.insert(bundleName);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + ImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                ImGui::PopStyleColor();
            }

			drawPresetBundleSelector(bundleName, pos, selectableHeight);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (ImGui::Button("Clear")) selectedPresets.clear();
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            for (const auto& bundleInfo : bundles) {
                std::vector<std::string> bundleIds = dataManager.getPresetsInBundle(bundleInfo.first);
                std::ranges::for_each(bundleIds, [&](auto& id) { selectedPresets.insert(id); });
            }
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();
    }

    void ExportPanel::drawPresetBundleSelector(const std::string& bundleName, ImVec2 pos, float selectableHeight) const {
        size_t count = dataManager.getPresetBundleCount(bundleName);

        // Group name... floating because imgui has no vertical alignment STILL :(
        ImVec2 labelSize = ImGui::CalcTextSize(bundleName.c_str());
        ImVec2 labelPos;
        labelPos.x = pos.x + listPaddingX + ImGui::GetStyle().ItemSpacing.x * 3.0f;
        labelPos.y = pos.y + (selectableHeight - labelSize.y) * 0.5f;

        ImGui::GetWindowDrawList()->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), bundleName.c_str());

        // Similarly for preset count
        const size_t nPresets = count;
        std::string rightText = std::to_string(nPresets);
        if (nPresets == 0) rightText = "Empty";
        else if (nPresets == 1) rightText += " Preset";
        else rightText += " Presets";

        ImVec2 rightTextSize = ImGui::CalcTextSize(rightText.c_str());
        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        float cursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - ImGui::GetStyle().ItemSpacing.x - listPaddingX;

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::GetWindowDrawList()->AddText(ImVec2(cursorPosX, labelPos.y), ImGui::GetColorU32(ImGuiCol_Text), rightText.c_str());
        ImGui::PopStyleColor();
    }

    bool ExportPanel::checkPresetBundleSelected(const std::vector<std::string>& bundleIds) const {
        bool selected = true;
        for (const auto& id : bundleIds) {
            if (selectedPresets.find(id) == selectedPresets.end()) {
                selected = false;
                break;
            }
        }
        return selected;
	}

    void ExportPanel::selectPresetBundle(const std::vector<std::string>& bundleIds) {
        std::ranges::for_each(bundleIds, [&](auto& id) { selectedPresets.insert(id); });
    }

    void ExportPanel::deselectPresetBundle(const std::vector<std::string>& bundleIds) {
        std::ranges::for_each(bundleIds, [&](auto& id) { selectedPresets.erase(id); });
    }

    void ExportPanel::drawPlayerOverrideList() {
        std::string filterStr{ filterBuffer };

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        constexpr char const* exportHintStr = "Click & Drag to select items to include in the export.";
        ImGui::Spacing();
        preAlignCellContentHorizontal(exportHintStr);
        ImGui::Text(exportHintStr);
        ImGui::Spacing();
        ImGui::PopStyleColor();

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, childItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, childWindowPadding);
        ImGui::BeginChild("ItemSelectorGroupChild", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        static bool isDragging = false;
        static bool dragStarted = false;
        static std::unordered_set<PlayerData> selectedDuringDrag{};

        ImVec2 dragStart, dragEnd;
        ImGuiIO& io = ImGui::GetIO();

        // Handle drag logic outside the list box
        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
            dragStart = io.MousePos;
            isDragging = true;
            dragStarted = true;
        }
        if (isDragging && ImGui::IsMouseReleased(0)) {
            isDragging = false;
            selectedDuringDrag.clear();
        }

        std::vector<const PlayerOverride*> playerOverrides = dataManager.getPlayerOverrides(filterBuffer, true);

        if (playerOverrides.size() == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Overrides Found.";
            ImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }

        for (const PlayerOverride* override : playerOverrides)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();

            bool selected = checkPlayerOverrideSelected(override);

            // Need to add the preset if added, and just remove override if not.
            bool hovered = false;
            if (DragSelectable(("##Selectable_Preset_" + override->player.string()).c_str(), selected, selectableHeight, &hovered)) {
                if (selected) deselectPlayerOverride(override);
                else          selectPlayerOverride(override);
                selected = !selected;
                selectedDuringDrag.insert(override->player);
            }

            bool alreadySelectedThisDrag = selectedDuringDrag.find(override->player) != selectedDuringDrag.end();
            if (isDragging && hovered && !alreadySelectedThisDrag) {
                const PresetGroup* presetGroup = dataManager.getPresetGroupByUUID(override->presetGroup);
                if (selected) deselectPlayerOverride(override);
				else          selectPlayerOverride(override);
                selected = !selected;
                selectedDuringDrag.insert(override->player);
            }

            ImVec2 tickMarkPos;
            tickMarkPos.x = pos.x + listPaddingX + ImGui::GetStyle().ItemSpacing.x;
            tickMarkPos.y = pos.y + (selectableHeight) * 0.5f;

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImTickMark(tickMarkPos, 1.2f, 2.0f);
                ImGui::PopStyleColor();
            }

            drawPlayerOverrideSelector(override, pos, selectableHeight);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        if (ImGui::Button("Clear")) selectedPlayerOverrides.clear();
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            for (const PlayerOverride* override : playerOverrides) selectPlayerOverride(override);
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();
    }

    void ExportPanel::drawPlayerOverrideSelector(const PlayerOverride* override, ImVec2 pos, float selectableHeight) const {
        // Sex Mark
        std::string sexMarkSymbol = override->player.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        ImVec4 sexMarkerCol = override->player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        ImGui::PushFont(wsSymbolFont);

        constexpr float sexMarkerSpacingAfter = 5.0f;
        constexpr float sexMarkerVerticalAlignOffset = 5.0f;
        ImVec2 sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
        ImVec2 sexMarkerPos;
        sexMarkerPos.x = pos.x + listPaddingX + 3.0f * ImGui::GetStyle().ItemSpacing.x;
        sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
        ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

        ImGui::PopFont();

        // Player name
        constexpr float playerNameSpacingAfter = 5.0f;
        ImVec2 playerNameSize = ImGui::CalcTextSize(override->player.name.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), override->player.name.c_str());

        // Preset Group
        const PresetGroup* activeGroup = dataManager.getPresetGroupByUUID(override->presetGroup);

        // Sex Mark
        bool female = false;
        float presetGroupSexMarkSpacing = ImGui::GetStyle().ItemSpacing.x;
        float presetGroupSexMarkSpacingBefore = 0.0f;
        float endPos = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        if (activeGroup) {
            bool female = activeGroup->female;
            presetGroupSexMarkSpacing = 15.0f;
            presetGroupSexMarkSpacingBefore = 10.0f;

            std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 presetGroupSexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 presetGroupSexMarkerSize = ImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
            ImVec2 presetGroupSexMarkerPos;
            presetGroupSexMarkerPos.x = endPos - presetGroupSexMarkSpacingBefore - ImGui::GetStyle().ItemSpacing.x - listPaddingX;
            presetGroupSexMarkerPos.y = pos.y + (selectableHeight - presetGroupSexMarkerSize.y) * 0.5f;

            ImGui::PushFont(wsSymbolFont);
            ImGui::GetWindowDrawList()->AddText(presetGroupSexMarkerPos, ImGui::GetColorU32(presetGroupSexMarkerCol), presetGroupSexMarkSymbol.c_str());
            ImGui::PopFont();
        }

        // Group Name
        std::string presetGroupName = override->presetGroup.empty() || activeGroup == nullptr ? "Default" : activeGroup->name;
        ImVec2 currentGroupStrSize = ImGui::CalcTextSize(presetGroupName.c_str());
        ImVec2 currentGroupStrPos;
        currentGroupStrPos.x = endPos - (currentGroupStrSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore + listPaddingX);
        currentGroupStrPos.y = pos.y + (selectableHeight - currentGroupStrSize.y) * 0.5f;

        ImVec4 presetGroupCol = activeGroup == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, presetGroupCol);
        ImGui::GetWindowDrawList()->AddText(currentGroupStrPos, ImGui::GetColorU32(ImGuiCol_Text), presetGroupName.c_str());
        ImGui::PopStyleColor();

        // Hunter ID
        std::string hunterIdStr = "(" + override->player.hunterId + ")";
        ImVec2 hunterIdStrSize = ImGui::CalcTextSize(hunterIdStr.c_str());
        ImVec2 hunterIdStrPos;
        hunterIdStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
        hunterIdStrPos.y = pos.y + (selectableHeight - hunterIdStrSize.y) * 0.5f;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::GetWindowDrawList()->AddText(hunterIdStrPos, ImGui::GetColorU32(ImGuiCol_Text), hunterIdStr.c_str());
        ImGui::PopStyleColor();
    }

    bool ExportPanel::checkPlayerOverrideSelected(const PlayerOverride* override) const {
        bool selected = selectedPlayerOverrides.find(override->player) != selectedPlayerOverrides.end();
        if (selected && !override->presetGroup.empty() && selectedPresetGroups.find(override->presetGroup) == selectedPresetGroups.end()) {
            selected = false;
        }

        return selected;
    }

    void ExportPanel::selectPlayerOverride(const PlayerOverride* override) {
        selectedPlayerOverrides.insert(override->player);
        if (!override->presetGroup.empty()) {
            const PresetGroup* presetGroup = dataManager.getPresetGroupByUUID(override->presetGroup);
            selectedPresetGroups.insert(presetGroup->uuid);
            std::ranges::for_each(presetGroup->bodyPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
            std::ranges::for_each(presetGroup->legsPresets, [&](auto& entry) { selectedPresets.insert(entry.second); });
        }
	}

    void ExportPanel::deselectPlayerOverride(const PlayerOverride* override) {
        selectedPlayerOverrides.erase(override->player);
	}

    KBFFileData ExportPanel::getKbfFileData() const {
        KBFFileData kbfData;
        for (const auto& presetGroupId : selectedPresetGroups) {
            const PresetGroup* presetGroup = dataManager.getPresetGroupByUUID(presetGroupId);
            if (presetGroup) kbfData.presetGroups.push_back(*presetGroup);
        }
        for (const auto& presetId : selectedPresets) {
            const Preset* preset = dataManager.getPresetByUUID(presetId);
            if (preset) kbfData.presets.push_back(*preset);
		}
        for (const auto& playerData : selectedPlayerOverrides) {
            const PlayerOverride* override = dataManager.getPlayerOverride(playerData);
            if (override) kbfData.playerOverrides.push_back(*override);
        }
		return kbfData;
    }

    std::string ExportPanel::getExportPathFileDialog() {
        exportPathDialogOpen = true;
        std::string pth = noc_browse_folder_open(exportPath.string());

        return pth;
    }

    void ExportPanel::postToMainThread(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackQueue.push(std::move(func));
    }

    void ExportPanel::processCallbacks() {
        std::lock_guard<std::mutex> lock(callbackMutex);
        while (!callbackQueue.empty()) {
            auto func = std::move(callbackQueue.front());
            callbackQueue.pop();
            func();
        }
    }

}