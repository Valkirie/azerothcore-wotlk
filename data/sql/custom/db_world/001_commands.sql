-- Rochenoire command registration.
--
-- Adds the custom `.npc` and `.patch` commands used to inspect and modify
-- level variation and patch ranges for creatures, quests, items, and gameobjects.

-- NPC level variation commands.
REPLACE INTO `command` (`name`, `help`) VALUES('npc set levelvar','Syntax: .npc set levelvar var (-n .. n) template (0/1)');

-- Patch range commands.
-- Set patch ranges.
REPLACE INTO `command` (`name`, `help`) VALUES('patch set npc','Syntax: .patch set npc min max template (0/1)');
REPLACE INTO `command` (`name`, `help`) VALUES('patch set quest','Syntax: .patch set quest entry min max');
REPLACE INTO `command` (`name`, `help`) VALUES('patch set item','Syntax: .patch set item entry min max');
REPLACE INTO `command` (`name`, `help`) VALUES('patch set go','Syntax: .patch set gobject entry min max');

-- Get patch ranges.
REPLACE INTO `command` (`name`, `help`) VALUES('patch get npc','Syntax: .patch get npc');
REPLACE INTO `command` (`name`, `help`) VALUES('patch get quest','Syntax: .patch get quest entry');
REPLACE INTO `command` (`name`, `help`) VALUES('patch get item','Syntax: .patch get item entry');
REPLACE INTO `command` (`name`, `help`) VALUES('patch get go','Syntax: .patch get gobject entry');


-- Remove patch ranges.
REPLACE INTO `command` (`name`, `help`) VALUES('patch remove npc','Syntax: .patch remove npc template (0/1)');
REPLACE INTO `command` (`name`, `help`) VALUES('patch remove item','Syntax: .patch remove item entry');
REPLACE INTO `command` (`name`, `help`) VALUES('patch remove go','Syntax: .patch remove gobject entry');
REPLACE INTO `command` (`name`, `help`) VALUES('patch remove quest','Syntax: .patch remove quest entry');
