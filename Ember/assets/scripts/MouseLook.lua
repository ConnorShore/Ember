local MouseLook = {}

MouseLook.Sensitivity = 0.005

function MouseLook:OnCreate(entity)
    self.Pitch = 0.0
end

function MouseLook:OnUpdate(entity, delta)
    local mouseDelta = Input.GetMouseDelta()

    -- YAW (Looking Left/Right)
    local parentEntity = entity:GetRootParent()
    if parentEntity then
        local parentTransform = parentEntity:GetComponent("TransformComponent")
        parentTransform.Rotation.y = parentTransform.Rotation.y - (mouseDelta.x * self.Sensitivity)
    end

    -- PITCH (Looking Up/Down)
    self.Pitch = self.Pitch - (mouseDelta.y * self.Sensitivity)
    self.Pitch = Math.Clamp(self.Pitch, Math.Radians(-89.0), Math.Radians(89.0))

    local transform = entity:GetComponent("TransformComponent")
    transform.Rotation.x = self.Pitch
end

return MouseLook