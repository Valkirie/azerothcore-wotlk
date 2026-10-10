local PREFIX = "RZScale"
local VERSION = "v1"
local LOCALIZED = {
    enUS = {
        LEVELS = "Levels %d-%d",
        WAITING = "Waiting for zone scaling data...",
    },
    frFR = {
        LEVELS = "Niveaux %d-%d",
        WAITING = "En attente des données de mise à niveau...",
    },
}
local L = LOCALIZED[GetLocale()] or LOCALIZED.enUS

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

local function UpdateDisplay(zoneName, minimum, maximum)
    zoneText:SetText(zoneName)
    rangeText:SetText(string.format(L.LEVELS, minimum, maximum))
    rangeText:SetTextColor(1, 1, 1)
    frame:Show()
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

local function HandleMessage(message)
    if type(message) ~= "string" then
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
        frame:Hide()
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
