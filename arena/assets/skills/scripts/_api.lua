-- Skill scripting API (auto-generated)
-- Reference file for Lua editor intellisense.
--
-- Available functions:
--   deal_damage(amount): deal damage to target
--   deal_power_damage(): deal damage using power formula
--   deal_power_damage_scaled(multiplier): deal damage using power formula * multiplier
--   heal_self(amount): heal self
--   heal_target(amount): heal target
--   minus_pp(amount): consume extra PP
--   get_self_last_damage_taken(): last damage taken by self
--   get_target_last_damage_taken(): last damage taken by target
--   get_self_turn_damage_taken(): damage taken by self this turn
--   get_target_turn_damage_taken(): damage taken by target this turn
--   set_self_damage_multiplier(multiplier): set incoming damage multiplier for self
--   set_target_damage_multiplier(multiplier): set incoming damage multiplier for target
--   set_self_damage_multiplier_turns(multiplier, turns): set incoming damage multiplier for self with duration
--   set_target_damage_multiplier_turns(multiplier, turns): set incoming damage multiplier for target with duration
--   set_self_damage_immunity_one_turn(): immune to damage for one turn
--   set_target_damage_immunity_one_turn(): immune to damage for one turn
--   set_self_flat_damage_reduction(amount): flat reduction to incoming damage for self
--   set_target_flat_damage_reduction(amount): flat reduction to incoming damage for target
--   set_self_per_hit_damage_reduction(amount, turns): per-hit reduction with duration (turns)
--   set_target_per_hit_damage_reduction(amount, turns): per-hit reduction with duration (turns)
--   rand_int(min, max): random integer in [min, max]
--   apply_self_ailment(name): apply ailment to self, returns bool
--   apply_target_ailment(name): apply ailment to target, returns bool
--   clear_self_ailments(): clear all ailments on self
--   clear_target_ailments(): clear all ailments on target
--   has_self_ailment(name): check ailment on self
--   has_target_ailment(name): check ailment on target
--   get_self_primary_ailment(): get primary ailment name
--   get_self_secondary_ailment(): get secondary ailment name
--   get_target_primary_ailment(): get primary ailment name
--   get_target_secondary_ailment(): get secondary ailment name
--   get_self_stage(stat): get stage for stat ("Atk","Def","SpA","SpD","Spe","Acc","Eva","Cri")
--   get_target_stage(stat): get stage for stat ("Atk","Def","SpA","SpD","Spe","Acc","Eva","Cri")
--   change_self_stage(stat, delta): change stage for stat
--   change_target_stage(stat, delta): change stage for stat
--   raise_self_stage(stat, amount): increase stage for stat
--   lower_self_stage(stat, amount): decrease stage for stat
--   raise_target_stage(stat, amount): increase stage for stat
--   lower_target_stage(stat, amount): decrease stage for stat
--   get_attr_multiplier(atk_attr, def_attr1, def_attr2): attr multiplier
--
-- Available globals:
--   skill_id: number
--   skill_name: string
--   skill_type: string ("Physical","Magical","Status")
--   skill_attr: string (e.g. "Fire")
--   skill_power: number
--   skill_priority: number
--   attacker_hp: number
--   attacker_max_hp: number
--   attacker_level: number
--   attacker_attr1: string
--   attacker_attr2: string
--   attacker_attack: number (staged)
--   attacker_defense: number (staged)
--   attacker_sp_attack: number (staged)
--   attacker_sp_defense: number (staged)
--   attacker_speed: number (staged)
--   attacker_attack_base: number
--   attacker_defense_base: number
--   attacker_sp_attack_base: number
--   attacker_sp_defense_base: number
--   attacker_speed_base: number
--   target_hp: number
--   target_max_hp: number
--   target_level: number
--   target_attr1: string
--   target_attr2: string
--   target_attack: number (staged)
--   target_defense: number (staged)
--   target_sp_attack: number (staged)
--   target_sp_defense: number (staged)
--   target_speed: number (staged)
--   target_attack_base: number
--   target_defense_base: number
--   target_sp_attack_base: number
--   target_sp_defense_base: number
--   target_speed_base: number
--
-- Entry point:
--   on_cast()

---@param amount number
function deal_damage(amount) end

function deal_power_damage() end

---@param multiplier number
function deal_power_damage_scaled(multiplier) end

---@param amount number
function heal_self(amount) end

---@param amount number
function heal_target(amount) end

---@param amount number
function minus_pp(amount) end

---@return number
function get_self_last_damage_taken() end

---@return number
function get_target_last_damage_taken() end

---@return number
function get_self_turn_damage_taken() end

---@return number
function get_target_turn_damage_taken() end

---@param multiplier number
function set_self_damage_multiplier(multiplier) end

---@param multiplier number
function set_target_damage_multiplier(multiplier) end

---@param multiplier number
---@param turns number
function set_self_damage_multiplier_turns(multiplier, turns) end

---@param multiplier number
---@param turns number
function set_target_damage_multiplier_turns(multiplier, turns) end

function set_self_damage_immunity_one_turn() end

function set_target_damage_immunity_one_turn() end

---@param amount number
function set_self_flat_damage_reduction(amount) end

---@param amount number
function set_target_flat_damage_reduction(amount) end

---@param amount number
---@param turns number
function set_self_per_hit_damage_reduction(amount, turns) end

---@param amount number
---@param turns number
function set_target_per_hit_damage_reduction(amount, turns) end

---@param min number
---@param max number
---@return number
function rand_int(min, max) end

---@param name string
---@return boolean
function apply_self_ailment(name) end

---@param name string
---@return boolean
function apply_target_ailment(name) end

function clear_self_ailments() end

function clear_target_ailments() end

---@param name string
---@return boolean
function has_self_ailment(name) end

---@param name string
---@return boolean
function has_target_ailment(name) end

---@return string
function get_self_primary_ailment() end

---@return string
function get_self_secondary_ailment() end

---@return string
function get_target_primary_ailment() end

---@return string
function get_target_secondary_ailment() end

---@param stat string
---@return number
function get_self_stage(stat) end

---@param stat string
---@return number
function get_target_stage(stat) end

---@param stat string
---@param delta number
---@return number
function change_self_stage(stat, delta) end

---@param stat string
---@param delta number
---@return number
function change_target_stage(stat, delta) end

---@param stat string
---@param amount number
---@return number
function raise_self_stage(stat, amount) end

---@param stat string
---@param amount number
---@return number
function lower_self_stage(stat, amount) end

---@param stat string
---@param amount number
---@return number
function raise_target_stage(stat, amount) end

---@param stat string
---@param amount number
---@return number
function lower_target_stage(stat, amount) end

---@param atk_attr string
---@param def_attr1 string
---@param def_attr2 string
---@return number
function get_attr_multiplier(atk_attr, def_attr1, def_attr2) end

---@type number
skill_id = 0

---@type string
skill_name = ""

---@type string
skill_type = ""

---@type string
skill_attr = ""

---@type number
skill_power = 0

---@type number
skill_priority = 0

---@type number
attacker_hp = 0

---@type number
attacker_max_hp = 0

---@type number
attacker_level = 0

---@type string
attacker_attr1 = ""

---@type string
attacker_attr2 = ""

---@type number
attacker_attack = 0

---@type number
attacker_defense = 0

---@type number
attacker_sp_attack = 0

---@type number
attacker_sp_defense = 0

---@type number
attacker_speed = 0

---@type number
attacker_attack_base = 0

---@type number
attacker_defense_base = 0

---@type number
attacker_sp_attack_base = 0

---@type number
attacker_sp_defense_base = 0

---@type number
attacker_speed_base = 0

---@type number
target_hp = 0

---@type number
target_max_hp = 0

---@type number
target_level = 0

---@type string
target_attr1 = ""

---@type string
target_attr2 = ""

---@type number
target_attack = 0

---@type number
target_defense = 0

---@type number
target_sp_attack = 0

---@type number
target_sp_defense = 0

---@type number
target_speed = 0

---@type number
target_attack_base = 0

---@type number
target_defense_base = 0

---@type number
target_sp_attack_base = 0

---@type number
target_sp_defense_base = 0

---@type number
target_speed_base = 0

function on_cast()
  -- Implement skill effects here.
end
