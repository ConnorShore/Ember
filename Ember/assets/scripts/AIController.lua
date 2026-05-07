local AIController = {}

function AIController:OnCreate(entity)

end

function AIController:OnUpdate(entity, delta)
    local pathComp = entity:GetComponent("AIPathComponent")
    local transform = entity:GetComponent("TransformComponent")
    local controller = entity:GetComponent("CharacterControllerComponent")
    
    -- Check if we have waypoints to follow
    if #pathComp.Waypoints == 0 then 
        return 
    end
    
    local targetPos = pathComp:GetNextWaypointPosition() 
    local currentPos = transform.WorldPosition
    
    -- Calculate distance
    local dx = targetPos.x - currentPos.x
    local dz = targetPos.z - currentPos.z
    local distance = math.sqrt(dx*dx + dz*dz)
    
    -- Did we arrive?
    if distance <= pathComp.ArrivalTolerance then
        -- Cycle to the next waypoint
        pathComp.CurrentWaypointIndex = (pathComp.CurrentWaypointIndex + 1) % #pathComp.Waypoints
        return 
    end
    
    -- Normalize the direction
    local dirX = dx / distance
    local dirZ = dz / distance
    
    -- Create the movement vector
    local moveVec = Vector3f.new(dirX * pathComp.Speed * delta, 0.0, dirZ * pathComp.Speed * delta)
    
    -- Move using the Character Controller
    controller:Move(moveVec)

    -- Rotate the AI to face the waypoint
    local targetAngle = math.deg(math.atan(dirX, dirZ)) 
    transform.Rotation.y = targetAngle
end

return AIController