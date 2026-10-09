-- Rochenoire localized server strings.
--
-- Registers the text used by the custom patch and scaling commands. The locale
-- columns are explicitly cleared so these entries start with English text and
-- no custom translations unless they are added later.

REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21000','Forced Scaled Level: %i.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21001','Patch Min: %u.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21002','Patch Max: %u.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21003','Zone not opened yet',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21004','Patch info is set for creature_template :',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21005','Patch info is set for single creature :',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
REPLACE INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES('21006','No Patch info found.',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
