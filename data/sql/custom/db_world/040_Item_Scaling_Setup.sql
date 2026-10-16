-- Rochenoire item-scaling schema preparation.
--
-- Expands item_template.entry to INT so the item-scaling system can address
-- generated and extended item entries beyond the original integer range.
ALTER TABLE item_template MODIFY entry INT UNSIGNED;
