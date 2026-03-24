-- Define the "Class" table
local CameraController = {}

function CameraController:OnCreate(entity)
    -- 'self' is the unique memory for this specific camera
    self.Entity = entity
    self.Speed = 5.0
end

function CameraController:OnUpdate(delta)
    local transform = self.Entity:GetTransform()

    -- 87 is the W key (Assuming your KeyCodes match standard ASCII)
    if IsKeyPressed(87) then 
        transform.Position.z = transform.Position.z - (self.Speed * delta)
    end
end

-- We MUST return the table so C++ can grab it!
return CameraController