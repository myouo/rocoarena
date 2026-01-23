function on_cast()
    deal_power_damage()
    if rand_int(1, 100) <= 50 then
        apply_target_ailment("Fear")
    end
end