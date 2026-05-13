local CharacterMovement = {}

function CharacterMovement:OnCreate(entity)
end

function CharacterMovement:OnUpdate(entity, delta)
    local controller = entity:GetComponent("CharacterControllerComponent")
    local transform = entity:GetComponent("TransformComponent")

    local forward = transform:GetForward()
    local right = transform:GetRight()
    
    -- Player Controller Movement
    local moveDir = Vector3f.new(0.0, 0.0, 0.0)

    -- Strafing
    if Input.IsKeyPressed(KeyCode.A) then 
        moveDir = moveDir - right
    elseif Input.IsKeyPressed(KeyCode.D) then 
        moveDir = moveDir + right
    end

    -- Forward / Backward
    if Input.IsKeyPressed(KeyCode.W) then 
        moveDir = moveDir + forward
    elseif Input.IsKeyPressed(KeyCode.S) then 
        moveDir = moveDir - forward
    end
    
    -- Normalize so diagonal movement isn't 1.4x faster!
    if Math.Length(moveDir) > 0 then
        moveDir = Math.Normalize(moveDir);
    end
    
    -- Move using the Character Controller
    controller:Move(moveDir * controller.WalkSpeed * delta)
    
    -- Jumping
    if Input.IsKeyPressed(KeyCode.Space) and controller.IsGrounded then
        controller:Jump()
    end
end

return CharacterMovement