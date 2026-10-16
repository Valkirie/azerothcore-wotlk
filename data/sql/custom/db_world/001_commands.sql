-- Rochenoire command registration.
--
-- Adds the custom `.npc` command used to modify creature level variation.

-- NPC level variation commands.
REPLACE INTO `command` (`name`, `help`) VALUES('npc set levelvar','Syntax: .npc set levelvar var (-n .. n) template (0/1)');

-- Live instance scaling test commands.
REPLACE INTO `command` (`name`, `security`, `help`) VALUES
('instance scaling set', 3, 'Syntax: .instance scaling set <player count>\n\nForces the current instance scaling player count and immediately refreshes creature scaling, spawn suppression, and addon difficulty. The count is clamped to the configured instance range.'),
('instance scaling reset', 3, 'Syntax: .instance scaling reset\n\nClears the forced player count and immediately restores scaling based on the current non-GM player count.'),
('instance scaling status', 3, 'Syntax: .instance scaling status\n\nDisplays the current effective, actual, and forced player counts, configured range, difficulty percentage, and pending refresh state.');
