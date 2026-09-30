-- Rochenoire command registration.
--
-- Adds the custom `.npc` command used to modify creature level variation.

-- NPC level variation commands.
REPLACE INTO `command` (`name`, `help`) VALUES('npc set levelvar','Syntax: .npc set levelvar var (-n .. n) template (0/1)');
