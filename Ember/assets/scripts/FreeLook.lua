local FreeLook = {}

-- Mouse and stick are tuned separately on purpose: a mouse reports how far it has already moved,
-- while a stick reports how fast to keep turning.
FreeLook.Sensitivity = 0.0025
FreeLook.StickYawRate = 220.0      -- degrees per second at full deflection
FreeLook.StickPitchRate = 160.0    -- lower than yaw, since pitch only spans twice PitchLimit
FreeLook.PitchLimit = 89.0         -- degrees up and down; short of 90 so the view cannot gimbal over
FreeLook.InvertY = false

function FreeLook:OnCreate(entity)
    self.Pitch = 0.0

    -- Inversion is a device setting, so one toggle covers the stick and the mouse together.
    Input.GetStickSettings(GamepadStick.Right).InvertY = self.InvertY
    Input.GetMouseSettings().InvertY = self.InvertY

    -- Cache our own transform handle (safe for the entity's lifetime; it re-resolves the
    -- live component internally) so OnUpdate doesn't do a string-keyed lookup every frame.
    self.transform = entity:GetComponent("TransformComponent")
end

function FreeLook:OnUpdate(entity, delta)
    -- The mouse delta already covers this frame, so it is an angle and must not be scaled by delta.
    local mouseDelta = Input.GetMouseDelta()

    -- The stick is a position held over time, so it is a rate and does need delta. The two are
    -- summed rather than switched between, so a mouse and a pad both work with no mode change.
    local stick = Input.GetAxis2D("LookLeft", "LookRight", "LookDown", "LookUp")

    local yaw = (mouseDelta.x * self.Sensitivity) + (stick.x * Math.Radians(self.StickYawRate) * delta)
    local pitch = -(mouseDelta.y * self.Sensitivity) + (stick.y * Math.Radians(self.StickPitchRate) * delta)

    -- YAW (Looking Left/Right)
    local parentEntity = entity:GetRootParent()
    if parentEntity then
        local parentTransform = parentEntity:GetComponent("TransformComponent")
        parentTransform.Rotation.y = parentTransform.Rotation.y - yaw
    end

    -- PITCH (Looking Up/Down)
    local pitchLimit = Math.Radians(self.PitchLimit)
    self.Pitch = Math.Clamp(self.Pitch + pitch, -pitchLimit, pitchLimit)

    local transform = self.transform
    transform.Rotation.x = self.Pitch
end

return FreeLook
