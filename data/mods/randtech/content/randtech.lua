local advanced_fields = {
    biology = 1,
    power = 2,
    physics_hyper_advanced = 3,
    construction_hyper_advanced = 4,
    fields_hyper_advanced = 5,
    chemistry_hyper_advanced = 6,
    computers_hyper_advanced = 7,
    sociology = 8,
}

local function random(bound)
    return game.rng:integer(0, bound)
end

local function random_pick(values, maximum)
    if maximum > #values then
        maximum = #values
    end
    return table.remove(values, random(maximum))
end

local function branch_roots(fields)
    local roots = {}
    local substitutions = {
        engineering = "advanced_engineering",
        chemistry = "advanced_metallurgy",
        nuclear_fission = "advanced_fusion",
        physics = "fusion_physics",
        electronics = "optronics",
    }
    for _, field in ipairs(fields) do
        if field.id ~= "starting_tech" and field.previous.id == "starting_tech" then
            local terminal = field
            local seen = {}
            while not advanced_fields[terminal.id] do
                if seen[terminal.id] then error("detected technology loop") end
                seen[terminal.id] = true
                terminal = terminal.next
            end
            local replacement = substitutions[field.id]
            local replacement_field = replacement and
                game.technology.fields:get(replacement) or field
            roots[advanced_fields[terminal.id]] = {
                field = replacement_field,
                previous = replacement and replacement_field.previous or nil,
            }
        end
    end
    return roots
end

return function(event)
    if event.setup_sequence ~= 1 then error("invalid technology setup sequence") end
    local technology = game.technology
    local fields = technology.fields:all()
    local applications = technology.applications:all()
    local roots = branch_roots(fields)
    if #roots ~= 8 then error("expected eight technology branches") end

    local branches = {}
    local randomized_fields = {}
    for index, root in ipairs(roots) do
        local field = root.field
        local seen = {}
        while not advanced_fields[field.id] do
            if seen[field.id] then error("detected technology loop") end
            seen[field.id] = true
            randomized_fields[field.id] = true
            field = field.next
        end
        branches[index] = {
            field = root.field,
            previous = root.previous,
            advanced = field,
            output = {},
            slots = 0,
        }
    end

    local fields_by_cost = {}
    for _, field in ipairs(fields) do
        if randomized_fields[field.id] then
            local is_root = false
            for _, root in ipairs(roots) do
                is_root = is_root or root.field == field
            end
            if not is_root then
                table.insert(fields_by_cost, field)
            end
        end
    end
    table.sort(fields_by_cost, function(left, right)
        if left.cost == right.cost then return left.id < right.id end
        return left.cost < right.cost
    end)

    local applications_by_cost = {}
    for _, application in ipairs(applications) do
        local field = application.field
        if field and field.id ~= "advanced_governments" and
                randomized_fields[field.id] then
            table.insert(applications_by_cost, {
                application = application,
                cost = field.cost,
            })
        end
    end
    table.sort(applications_by_cost, function(left, right)
        if left.cost == right.cost then
            return left.application.id < right.application.id
        end
        return left.cost < right.cost
    end)
    for _, entry in ipairs(applications_by_cost) do
        entry.application.field = nil
    end

    local active = {}
    for index, branch in ipairs(branches) do active[index] = branch end
    local repeat_index = nil
    while #applications_by_cost > 0 and #active > 0 do
        local branch_index = repeat_index or random(#active)
        repeat_index = nil
        local branch = active[branch_index]
        local field = branch.field
        if field.id == "advanced_governments" then
            local governments = {
                "federation",
                "confederation",
                "galactic_unification",
                "imperium",
            }
            for _, id in ipairs(governments) do
                technology.applications:get(id).field = field
            end
            branch.slots = 4
        else
            local pick_empty = false
            if branch.slots > 0 then
                local slots = #fields_by_cost * 3
                for _, open_branch in ipairs(active) do
                    slots = slots + 4 - open_branch.slots
                end
                local remaining = #applications_by_cost - #fields_by_cost
                pick_empty = random(slots) > remaining
            end
            if pick_empty then
                branch.slots = branch.slots + 1
            else
                local picked = random_pick(applications_by_cost, 3)
                picked.application.field = field
                branch.slots = branch.slots + 1
            end
        end

        if branch.slots == 4 then
            table.insert(branch.output, field)
            if #fields_by_cost > 0 then
                branch.field = random_pick(fields_by_cost, 1)
                branch.slots = 0
                repeat_index = branch_index
            else
                table.remove(active, branch_index)
            end
        end
    end

    for _, branch in ipairs(active) do table.insert(branch.output, branch.field) end
    while #fields_by_cost > 0 do
        table.insert(branches[random(8)].output, table.remove(fields_by_cost, 1))
    end
    for _, branch in ipairs(branches) do
        table.insert(branch.output, branch.advanced)
        for index, field in ipairs(branch.output) do
            field.previous = index == 1 and branch.previous or
                branch.output[index - 1]
            field.next = index == #branch.output and nil or branch.output[index + 1]
        end
    end
end