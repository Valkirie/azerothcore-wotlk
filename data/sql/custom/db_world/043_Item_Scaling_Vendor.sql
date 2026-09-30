-- Normalize vendor consumables and ammunition for the Rochenoire item-scaling system.
--
-- Vendors that sell one item from a family receive the other items in that family.
-- The generated rows inherit the source row's slot, stock, restock, cost, and build
-- metadata. The item list is also registered as exempt from vendor item scaling.
--
-- The transformation is built separately and swapped in only after it is complete.
-- This keeps the live npc_vendor table available if staging fails.

DROP TABLE IF EXISTS rochenoire_new_npc_vendor;
DROP TABLE IF EXISTS rochenoire_save_npc_vendor;

CREATE TEMPORARY TABLE rochenoire_vendor_source LIKE npc_vendor;
INSERT INTO rochenoire_vendor_source
SELECT *
FROM npc_vendor;

-- Ammunition families whose equivalent items should be offered together.
CREATE TEMPORARY TABLE rochenoire_vendor_family (
    family_name VARCHAR(32) NOT NULL,
    item INT NOT NULL,
    PRIMARY KEY (family_name, item)
) ENGINE=InnoDB;

INSERT INTO rochenoire_vendor_family (family_name, item) VALUES
('bullets', 2516),
('bullets', 2519),
('bullets', 3033),
('bullets', 11284),
('bullets', 28061),
('bullets', 28060),
('bullets', 31735),
('bullets', 32883),
('bullets', 41584),
('bullets', 32882),
('bullets', 30612),
('bullets', 19317),
('bullets', 34582),
('arrows', 2512),
('arrows', 2515),
('arrows', 3030),
('arrows', 11285),
('arrows', 28056),
('arrows', 28053),
('arrows', 41586),
('arrows', 31737),
('arrows', 30611),
('arrows', 31949),
('arrows', 19316),
('arrows', 34581);

-- Classify all vendor-only food and drink by name before adding it to the family expansion.
-- Existing family entries are retained by INSERT IGNORE below.
CREATE TEMPORARY TABLE rochenoire_vendor_food_drink (
    item INT NOT NULL PRIMARY KEY
) ENGINE=InnoDB;

INSERT INTO rochenoire_vendor_food_drink (item) VALUES
(44616), (44618), (2595), (44574), (17402), (2594), (44573), (38432),
(2596), (2593), (17403), (40036), (44575), (44571), (9260), (4595),
(29112), (3703), (21151), (18287), (18288), (19222), (21721), (28284),
(38350), (159), (787), (44855), (46793), (46797), (44854), (46796),
(46784), (11109), (4604), (16166), (2070), (17344), (20857), (4536),
(46690), (117), (4540), (17196), (19223), (2894), (2723), (44570),
(2686), (40035), (44617), (1179), (2287), (4541), (4537), (4592),
(414), (4605), (17404), (18633), (19304), (16167), (17119), (17406),
(3770), (4593), (4538), (16170), (1205), (4542), (7228), (4606),
(19299), (422), (19305), (1707), (4594), (4544), (16169), (4607),
(17407), (4600), (18632), (3771), (4539), (19224), (1708), (38466),
(19221), (4602), (4599), (17408), (1645), (19300), (3927), (4608),
(19306), (4601), (18635), (21030), (21552), (16168), (8932), (8957),
(8766), (19225), (8950), (13724), (8948), (8953), (22324), (23160),
(8952), (11444), (13810), (21033), (38429), (21031), (19301), (27667),
(27855), (27666), (27856), (27859), (24009), (27854), (28486), (38427),
(27858), (27657), (27857), (24008), (24539), (29412), (32455), (32721),
(29393), (28399), (29454), (38430), (35949), (33454), (27860), (29395),
(32453), (29450), (33452), (33449), (33042), (37252), (29401), (29448),
(29449), (29452), (35954), (29394), (30355), (32668), (32685), (40356),
(40357), (29453), (32667), (33443), (34780), (38428), (40358), (40359),
(33451), (37253), (29451), (32686), (32722), (40042), (33444), (38698),
(43086), (44941), (42430), (42428), (35948), (35953), (35947), (35951),
(33445), (41731), (44072), (35950), (38706), (42429), (42431), (43236),
(40202), (41729), (42777), (44071), (42778), (44049), (35952), (42779),
(44940);

INSERT IGNORE INTO rochenoire_vendor_family (family_name, item)
SELECT CASE
    WHEN LOWER(item_template.name) REGEXP 'water|juice|milk|ale|beer|wine|rum|brandy|spirit|brew|cider|mead|drink|coffee|tea|bourbon|tequila|port|grog|slammer|depth charge'
        THEN 'water'
    WHEN LOWER(item_template.name) REGEXP 'bread|bagel|doughnut|cupcake|brownie|muffin|pretzel'
        THEN 'bread'
    WHEN LOWER(item_template.name) REGEXP 'cheese|brie|cheddar|swiss|whey'
        THEN 'cheese'
    WHEN LOWER(item_template.name) REGEXP 'fish|mackerel|snapper|catfish|cod|crawdad|salmon|eel|trout|halibut|yellowtail|carp'
        THEN 'fish'
    WHEN LOWER(item_template.name) REGEXP 'apple|banana|berry|berries|grape|pumpkin|peach|plantain|cranberr|fruit'
        THEN 'fruit'
    WHEN LOWER(item_template.name) REGEXP 'jerky|meat|venison|beef|chicken|ham|quail|ribs|flank|drakeflesh|eagle|beast|sausage|caribou'
        THEN 'meat'
    WHEN LOWER(item_template.name) REGEXP 'mushroom|morel|truffle|bolete|lichen'
        THEN 'mushroom'
    WHEN LOWER(item_template.name) REGEXP 'candy|taffy|sucker|ice cream'
        THEN 'candy'
    ELSE 'meal'
END AS family_name, food.item
FROM rochenoire_vendor_food_drink food
JOIN item_template ON item_template.entry = food.item;

INSERT IGNORE INTO rochenoire_items_not_scaled_from_vendors (ItemId, `comment`)
SELECT item, family_name
FROM rochenoire_vendor_family;

-- Start with the current vendor data while preserving the original table schema and keys.
CREATE TABLE rochenoire_new_npc_vendor LIKE npc_vendor;

-- Keep the explicit non-food removals out of the staged vendor table.
INSERT INTO rochenoire_new_npc_vendor
SELECT *
FROM rochenoire_vendor_source
WHERE item NOT IN (40533, 39684);

-- Add one missing family item per vendor. Existing vendor rows remain authoritative.
-- When several source rows qualify, copy metadata from the deterministic first row.
INSERT INTO rochenoire_new_npc_vendor
    (entry, slot, item, maxcount, incrtime, ExtendedCost, VerifiedBuild)
SELECT entry, slot, item, maxcount, incrtime, ExtendedCost, VerifiedBuild
FROM (
    SELECT
        source.entry,
        source.slot,
        family.item,
        source.maxcount,
        source.incrtime,
        source.ExtendedCost,
        source.VerifiedBuild,
        ROW_NUMBER() OVER (
            PARTITION BY source.entry, family.item
            ORDER BY source.slot, source.item, source.ExtendedCost, COALESCE(source.VerifiedBuild, -1)
        ) AS candidate_rank
    FROM rochenoire_vendor_source source
    JOIN rochenoire_vendor_family family ON source.item = family.item
    WHERE NOT EXISTS (
        SELECT 1
        FROM rochenoire_new_npc_vendor existing
        WHERE existing.entry = source.entry
          AND existing.item = family.item
    )
) candidates
WHERE candidate_rank = 1;

-- Swap the completed replacement into place atomically, then remove the old table.
RENAME TABLE npc_vendor TO rochenoire_save_npc_vendor,
             rochenoire_new_npc_vendor TO npc_vendor;

DROP TABLE rochenoire_save_npc_vendor;

