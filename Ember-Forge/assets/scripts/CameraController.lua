-- Define the "Class" table
local CameraController = {}

function CameraController:OnCreate(entity)
    -- 'self' is the unique memory for this specific camera
    self.Entity = entity
    self.Speed = 5.0
end

function CameraController:OnUpdate(delta)
    local transform = self.Entity:GetTransform()

    if IsKeyPressed(KeyCode.W) then 
        transform.Position.z = transform.Position.z - (self.Speed * delta)
    end
    if IsKeyPressed(KeyCode.S) then 
        transform.Position.z = transform.Position.z + (self.Speed * delta)
    end
    if IsKeyPressed(KeyCode.D) then 
        transform.Position.x = transform.Position.x + (self.Speed * delta)
    end
    if IsKeyPressed(KeyCode.A) then 
        transform.Position.x = transform.Position.x - (self.Speed * delta)
    end
end

-- We MUST return the table so C++ can grab it!
return CameraController