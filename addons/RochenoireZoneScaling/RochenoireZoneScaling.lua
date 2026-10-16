local PREFIX = "RZScale"
local VERSION = "v1"
local LOCALIZED = {
    enUS = {
        LEVELS = "Levels %d-%d",
        DIFFICULTY = "Difficulty",
        DIFFICULTY_PERCENTAGE = "Difficulty: %d%%",
        WAITING = "Waiting for zone scaling data...",
    },
    frFR = {
        LEVELS = "Niveaux %d-%d",
        DIFFICULTY = "Difficulté",
        DIFFICULTY_PERCENTAGE = "Difficulté : %d%%",
        WAITING = "En attente des données de mise à niveau...",
    },
}
local L = LOCALIZED[GetLocale()] or LOCALIZED.enUS
local zoneRanges = {}
local zonePositions = {}
local mapLabels = {}
local mapLabelLayer
local zoneMapLabel
local areaMapLabel
local currentAreaName
local currentAreaMinimum
local currentAreaMaximum
local RefreshMapLabels
local scanningMap = false

local frame = CreateFrame("Frame", "RochenoireZoneScalingFrame", UIParent)
frame:SetWidth(240)
frame:SetHeight(64)
frame:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile = true,
    tileSize = 32,
    edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 },
})
frame:SetBackdropColor(0, 0, 0, 0.8)
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", function(self)
    self:StartMoving()
end)
frame:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local point, _, relativePoint, x, y = self:GetPoint()
    RochenoireZoneScalingDB = {
        point = point,
        relativePoint = relativePoint,
        x = x,
        y = y,
    }
end)

local zoneText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
zoneText:SetPoint("TOP", 0, -12)
zoneText:SetWidth(220)
zoneText:SetJustifyH("CENTER")

local rangeText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
rangeText:SetPoint("TOP", zoneText, "BOTTOM", 0, -3)
rangeText:SetWidth(220)
rangeText:SetJustifyH("CENTER")

local difficultyBar = CreateFrame("StatusBar", nil, frame)
difficultyBar:SetPoint("TOP", rangeText, "BOTTOM", 0, -6)
difficultyBar:SetWidth(200)
difficultyBar:SetHeight(14)
difficultyBar:SetMinMaxValues(0, 100)
difficultyBar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
difficultyBar:EnableMouse(true)

local difficultyBackground = difficultyBar:CreateTexture(nil, "BACKGROUND")
difficultyBackground:SetAllPoints(difficultyBar)
difficultyBackground:SetTexture("Interface\\TargetingFrame\\UI-StatusBar")
difficultyBackground:SetVertexColor(0.15, 0.15, 0.15, 0.8)

local difficultyText = difficultyBar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
difficultyText:SetPoint("CENTER")
difficultyText:SetText(L.DIFFICULTY)
difficultyText:SetTextColor(1, 1, 1)
difficultyBar:Hide()

difficultyBar:SetScript("OnEnter", function(self)
    if self.difficulty then
        GameTooltip:SetOwner(self, "ANCHOR_CURSOR")
        GameTooltip:SetText(string.format(L.DIFFICULTY_PERCENTAGE, self.difficulty), 1, 1, 1)
        GameTooltip:Show()
    end
end)
difficultyBar:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)

local function UpdateDifficulty(difficulty)
    if difficulty then
        local value = math.max(0, math.min(100, difficulty))
        local red = value / 100
        difficultyBar.difficulty = value
        difficultyBar:SetValue(value)
        difficultyBar:SetStatusBarColor(red, 1 - red, 0)
        difficultyBar:Show()
        frame:SetHeight(86)
    else
        difficultyBar.difficulty = nil
        difficultyBar:Hide()
        frame:SetHeight(64)
    end
end

local function UpdateDisplay(zoneName, minimum, maximum)
    currentAreaName = zoneName
    currentAreaMinimum = minimum
    currentAreaMaximum = maximum
    UpdateDifficulty(nil)
    zoneText:SetText(zoneName)
    rangeText:SetText(string.format(L.LEVELS, minimum, maximum))
    rangeText:SetTextColor(1, 1, 1)
    frame:Show()
    if RefreshMapLabels then
        RefreshMapLabels()
    end
end

local function ApplySavedPosition()
    frame:ClearAllPoints()
    local saved = RochenoireZoneScalingDB
    if saved and saved.point and saved.relativePoint then
        frame:SetPoint(saved.point, UIParent, saved.relativePoint, saved.x or 0, saved.y or 0)
    else
        frame:SetPoint("TOP", UIParent, "TOP", 0, -160)
    end
end

local function UpdateHoveredZoneLabel()
    if not WorldMapFrameAreaLabel or not WorldMapDetailFrame or not WorldMapDetailFrame:IsMouseOver() then
        return
    end

    local cursorX, cursorY = GetCursorPosition()
    local scale = WorldMapDetailFrame:GetEffectiveScale()
    cursorX = cursorX / scale
    cursorY = cursorY / scale

    local x = (cursorX - WorldMapDetailFrame:GetLeft()) / WorldMapDetailFrame:GetWidth()
    local y = (WorldMapDetailFrame:GetTop() - cursorY) / WorldMapDetailFrame:GetHeight()
    if x < 0 or x > 1 or y < 0 or y > 1 then
        return
    end

    local zoneName = UpdateMapHighlight(x, y)
    local range = zoneName and zoneRanges[zoneName]
    if range then
        WorldMapFrameAreaLabel:SetText(string.format("%s (%d-%d)", zoneName, range.minimum, range.maximum))
    end
end

local function HideMapLabels()
    for _, label in ipairs(mapLabels) do
        label:Hide()
    end
    if zoneMapLabel then
        zoneMapLabel:Hide()
    end
    if areaMapLabel then
        areaMapLabel:Hide()
    end
end

local function DiscoverZonePositions(continent)
    if zonePositions[continent] then
        return zonePositions[continent]
    end

    local positions = {}
    zonePositions[continent] = positions
    scanningMap = true

    for y = 0, 1, 0.01 do
        for x = 0, 1, 0.01 do
            local zoneName, _, _, _, width, height, offsetX, offsetY = UpdateMapHighlight(x, y)
            if zoneName and not positions[zoneName] and width and height and offsetX and offsetY then
                positions[zoneName] = {
                    x = offsetX + width / 2,
                    y = offsetY + height / 2,
                }
            end
        end
    end

    scanningMap = false
    if WorldMapHighlight then
        WorldMapHighlight:Hide()
    end
    return positions
end

RefreshMapLabels = function()
    if scanningMap or not WorldMapFrame or not WorldMapFrame:IsShown() then
        return
    end

    HideMapLabels()
    local continent = GetCurrentMapContinent()
    if continent <= 0 then
        return
    end

    local mapCanvas = WorldMapDetailFrame or WorldMapButton
    if not mapCanvas then
        return
    end

    if not mapLabelLayer then
        mapLabelLayer = CreateFrame("Frame", nil, mapCanvas)
        mapLabelLayer:SetAllPoints(mapCanvas)
        mapLabelLayer:SetFrameLevel(WorldMapFrame:GetFrameLevel() + 100)
    end

    local zone = GetCurrentMapZone()
    if zone ~= 0 then
        local zones = { GetMapZones(continent) }
        local zoneName = zones[zone]
        local range = zoneName and zoneRanges[zoneName]
        if range then
            if not zoneMapLabel then
                zoneMapLabel = mapLabelLayer:CreateFontString(nil, "OVERLAY")
                local font = GameFontNormal:GetFont()
                zoneMapLabel:SetFont(font, 18, "OUTLINE")
                zoneMapLabel:SetTextColor(1, 1, 1)
                zoneMapLabel:SetShadowColor(0, 0, 0, 1)
                zoneMapLabel:SetShadowOffset(1, -1)
                zoneMapLabel:SetPoint("TOP", mapLabelLayer, "TOP", 0, -18)
            end
            zoneMapLabel:SetText(string.format("%s (%d-%d)", zoneName, range.minimum, range.maximum))
            zoneMapLabel:Show()

            if currentAreaName and currentAreaName ~= zoneName and currentAreaMinimum and currentAreaMaximum then
                if not areaMapLabel then
                    areaMapLabel = mapLabelLayer:CreateFontString(nil, "OVERLAY")
                    local font = GameFontNormal:GetFont()
                    areaMapLabel:SetFont(font, 14, "OUTLINE")
                    areaMapLabel:SetTextColor(1, 1, 1)
                    areaMapLabel:SetShadowColor(0, 0, 0, 1)
                    areaMapLabel:SetShadowOffset(1, -1)
                    areaMapLabel:SetPoint("TOP", zoneMapLabel, "BOTTOM", 0, -4)
                end
                areaMapLabel:SetText(string.format("%s (%d-%d)", currentAreaName, currentAreaMinimum, currentAreaMaximum))
                areaMapLabel:Show()
            end
        end
        return
    end

    local positions = DiscoverZonePositions(continent)
    local labelIndex = 0
    for zoneName, range in pairs(zoneRanges) do
        local position = positions[zoneName]
        if position then
            labelIndex = labelIndex + 1
            local label = mapLabels[labelIndex]
            if not label then
                label = mapLabelLayer:CreateFontString(nil, "OVERLAY")
                local font = GameFontNormal:GetFont()
                label:SetFont(font, 13, "OUTLINE")
                label:SetTextColor(1, 1, 1)
                label:SetShadowColor(0, 0, 0, 1)
                label:SetShadowOffset(1, -1)
                mapLabels[labelIndex] = label
            end

            label:ClearAllPoints()
            label:SetPoint("CENTER", mapLabelLayer, "TOPLEFT", position.x * mapCanvas:GetWidth(), -position.y * mapCanvas:GetHeight())
            label:SetText(string.format("%d-%d", range.minimum, range.maximum))
            label:Show()
        end
    end
end

local function HandleCatalogMessage(message)
    if message == "v2:catalog:start" then
        zoneRanges = {}
        HideMapLabels()
        return true
    end

    local data = string.match(message, "^v2:catalog:data:(.*)$")
    if data then
        for record in string.gmatch(data, "([^|]+)") do
            local minimum, maximum, zoneName = string.match(record, "^(%d+),(%d+),(.+)$")
            if minimum and maximum and zoneName then
                zoneRanges[zoneName] = {
                    minimum = tonumber(minimum),
                    maximum = tonumber(maximum),
                }
            end
        end
        return true
    end

    if message == "v2:catalog:done" then
        RefreshMapLabels()
        return true
    end

    return false
end

local function HandleInstanceMessage(message)
    if message == "v2:instance:clear" then
        UpdateDifficulty(nil)
        return true
    end

    local difficulty = tonumber(string.match(message, "^v2:instance:(%d+)$"))
    if difficulty then
        UpdateDifficulty(difficulty)
        return true
    end

    return false
end

local function HandleMessage(message)
    if type(message) ~= "string" then
        return
    end

    if HandleCatalogMessage(message) then
        return
    end

    if HandleInstanceMessage(message) then
        return
    end

    local values = {}
    for value in string.gmatch(message, "([^:]+)") do
        table.insert(values, value)
    end

    if values[1] ~= VERSION then
        return
    end

    if values[2] == "clear" then
        currentAreaName = nil
        currentAreaMinimum = nil
        currentAreaMaximum = nil
        UpdateDifficulty(nil)
        frame:Hide()
        HideMapLabels()
        return
    end

    if #values < 10 then
        return
    end

    local minimum = tonumber(values[5])
    local maximum = tonumber(values[6])
    if not minimum or not maximum then
        return
    end

    local areaName = table.concat(values, ":", 10)
    UpdateDisplay(areaName, minimum, maximum)
end

frame:SetScript("OnEvent", function(self, event, ...)
    if event == "PLAYER_LOGIN" then
        zoneText:SetText(GetZoneText())
        rangeText:SetText(L.WAITING)
        rangeText:SetTextColor(1, 1, 1)
        frame:Show()
        SendAddonMessage(PREFIX, "v2:catalog", "WHISPER", UnitName("player"))
        return
    end

    if event == "WORLD_MAP_UPDATE" then
        RefreshMapLabels()
        return
    end

    if event == "CHAT_MSG_ADDON" then
        local prefix, message = ...
        prefix = prefix or arg1
        message = message or arg2
        if prefix == PREFIX then
            HandleMessage(message)
        end
    end
end)

frame:RegisterEvent("CHAT_MSG_ADDON")
frame:RegisterEvent("PLAYER_LOGIN")
frame:RegisterEvent("WORLD_MAP_UPDATE")
if WorldMapButton then
    WorldMapButton:HookScript("OnUpdate", UpdateHoveredZoneLabel)
end
if RegisterAddonMessagePrefix then
    RegisterAddonMessagePrefix(PREFIX)
end
ApplySavedPosition()
frame:Hide()

SLASH_ROCHENOIREZONESCALING1 = "/rzscale"
SlashCmdList.ROCHENOIREZONESCALING = function(command)
    command = string.lower(command or "")
    if command == "reset" then
        RochenoireZoneScalingDB = nil
        ApplySavedPosition()
    elseif frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
    end
end
