# Level-context audit

This audit records why remaining native `GetLevel()` calls are intentional instead of using `getLevelForTarget()`.

## Core spell handling

- `src/server/game/Spells/SpellEffects.cpp`
  - Shield of Righteousness uses the caster's native level for its base block-value formula.
  - Restore Energy, Blood Fury, and Burst of Energy use the caster's native progression level for their historical level penalties.
  - Mad Alchemist's Potion filters elixirs using the target's native progression level and spell rank data.
  - Honor rewards use the player's native level because honor brackets and reward scaling are progression-based.
  - Game-object level assignment uses the caster's native level.
  - `minLevel` checks are explicit native-level spell requirements.
  - Guardian summons start from the caster's native level unless the item or spell supplies an explicit level; the native-level comparison determines whether summon stats need initialization.

- `src/server/game/Spells/Spell.cpp`
  - Aura rank eligibility and aura-rank selection use the target's native progression level.
  - Pet spell learning checks use the pet's persisted native level.
  - Recruit-a-Friend eligibility uses the player's real progression level.
  - Damage and level-cap formulas that already call `getLevelForTarget()` remain target-relative.

- `src/server/game/Spells/Auras/SpellAuraEffects.cpp`
  - Caster level is retained for aura-created base statistics and caster-level state; these formulas are not target-relative.

## Spell scripts

- `src/server/scripts/Spells/spell_generic.cpp`
  - Reduced-above-60 effects, disabled-above-63 effects, and related proc thresholds are historical native-level spell mechanics.
  - The ethereal pet comparison intentionally compares the persisted levels of the aura owner and proc target; it is not a creature scaling formula.
  - Level-80 eligibility is a direct native-level requirement.

- `src/server/scripts/Spells/spell_item.cpp`
  - Runescroll, mount, item-aura, recall, and spell-reflector thresholds are item or progression requirements based on native levels.
  - Green Whelp Armor and Mind Control Cap use `getLevelForTarget()` where the target is evaluated relative to the player.

- `src/server/scripts/Spells/spell_hunter.cpp`
  - Tame Beast compares the creature's target-relative level against the player's native level, preserving the one-level-above-player rule.

- `src/server/scripts/Spells/spell_dk.cpp`
  - Ghoul level, pet-level table lookup, and weapon damage initialization use the ghoul's persisted native level.

## Confirmed target-relative uses

The following reviewed mechanics intentionally use `getLevelForTarget()` because the value is evaluated from one unit's perspective against another unit:

- Shield Slam block scaling.
- Tamed-pet level capping and visual level initialization.
- Skinning skill requirements.
- Spell armor reduction and target-level caps.
- Tame Beast target validation.
- Item effects that explicitly evaluate a creature relative to a player.

## Validation

- Full workspace build succeeded after the audit.
- Changed-file diagnostics reported no errors.
- `git diff --check` passed.
- Reference comparison found no additional confirmed target-relative mismatch in the reviewed spell, aura, core, or spell-script scope.
