local ADDON_NAME = "RochenoireScaledItems"
local SCALED_ITEM_MIN_ID = 70000
local SCALED_ITEM_MAX_ID = 4294967295

local SCALED_ITEM_DESCRIPTION = {
    enUS = "It fits you surprisingly well.",
    enGB = "It fits you surprisingly well.",
    frFR = "Il vous va étonnamment bien.",
    deDE = "Es passt erstaunlich gut zu dir.",
    esES = "Te queda sorprendentemente bien.",
    esMX = "Te queda sorprendentemente bien.",
    ptBR = "Fica surpreendentemente bem em você.",
    ruRU = "На вас это удивительно хорошо смотрится.",
    koKR = "놀라울 정도로 잘 어울립니다.",
    zhCN = "它出奇地适合你。",
    zhTW = "它出奇地適合你。",
}

local function IsScaledItem(itemLink)
    if not itemLink then
        return false
    end

    local itemId = tonumber(string.match(itemLink, "item:(%d+)"))
    return itemId and itemId >= SCALED_ITEM_MIN_ID and itemId <= SCALED_ITEM_MAX_ID
end

local function AddScaledItemDescription(tooltip)
    local _, itemLink = tooltip:GetItem()
    if IsScaledItem(itemLink) then
        local description = SCALED_ITEM_DESCRIPTION[GetLocale()] or SCALED_ITEM_DESCRIPTION.enUS
        tooltip:AddLine(description, 1, 0.82, 0)
        tooltip:Show()
    end
end

local function HookTooltip(tooltip)
    tooltip:HookScript("OnTooltipSetItem", AddScaledItemDescription)
end

HookTooltip(GameTooltip)
HookTooltip(ItemRefTooltip)
