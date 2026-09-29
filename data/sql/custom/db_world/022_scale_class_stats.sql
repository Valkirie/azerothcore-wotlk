-- Rochenoire death-knight class-stat initialization.
--
-- Copies the warrior base stat progression to the death-knight class for
-- levels below 55, where the death-knight progression begins in the game.
REPLACE INTO player_class_stats (Class, LEVEL, BaseHP, BaseMana, Strength, Agility, Stamina, Intellect, Spirit)
SELECT 6, LEVEL, BaseHP, BaseMana, Strength, Agility, Stamina, Intellect, Spirit
FROM player_class_stats
WHERE Class = 1 AND LEVEL < 55;
