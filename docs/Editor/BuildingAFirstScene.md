# Building a First Scene

This walkthrough creates a small playable test scene using built-in presets and editor primitives.
It is intentionally simple: a ground area, a few objects, a light, a camera/controller, and a script
hook you can expand.

## 1. Create a Project and Scene

1. Launch `Ember-Forge`.
2. Choose **New Project** and create a project in a writable folder.
3. Use the default `Default.ebs` scene, or create one with `File > New Scene`.
4. Save with `File > Save Scene`.

## 2. Add Ground and Props

1. In the Scene Hierarchy, right-click empty space.
2. Choose `Create Primitive > Cube`.
3. Rename it `Ground`.
4. In the Inspector, set the Transform scale to something broad and flat, for example `10, 0.25, 10`.
5. Add a few more cubes or spheres as props. Move them with the translate gizmo (`W`) and scale them
   with the scale gizmo (`R`).

Models and prefabs dragged into the viewport land on the surface under the cursor, snapped to the
grid; entities made from the hierarchy menu land at the camera's focal point. `F` frames whatever is
selected, `Ctrl+Z` undoes, and `Ctrl` + click adds another entity to the selection so the gizmo moves
several at once. Snap increments live under the toolbar's **Gizmos** dropdown.

For imported meshes, right-click the Asset Manager and choose `Import Asset > Model`, then assign the
resulting mesh asset to a `Static Mesh Component` or use the imported model's generated assets.

## 3. Add Lighting

1. In the Scene Hierarchy, right-click empty space.
2. Choose `Create Light > Directional Light` for broad scene lighting.
3. Add a `Point Light` or `Spot Light` if you want a visible local light source.
4. Select the light and adjust color, intensity, radius, or cutoff angles in the Inspector.

Use the Environment panel to adjust skybox intensity, bloom, fog, vignette, FXAA, and color grading.

## 4. Add a Camera or Controller

For a free camera view:

1. Navigate the editor camera to the view you want.
2. In the hierarchy context menu, choose `Create Camera > Camera From View`.
3. Select the camera and confirm its `Camera Component` is active.

For a quick playable controller:

1. In the hierarchy context menu, choose `Create Controller > 1st Person Character`.
2. Ember copies the built-in `CharacterMovement.lua` and `MouseLook.lua` scripts into
   `GameData/Assets/Scripts` if they are not already present.
3. The preset creates a character entity with a character controller, kinematic rigid body, capsule
   collider, child head pivot, camera, and movement scripts.

Make sure the floor has a collider if the character should stand on it. A cube primitive can use a
`Rigid Body Component` plus `Box Collider Component`; keep static world geometry non-moving.

## 5. Add a Basic Interaction Target

Create a small pickup or target object:

1. Add a sphere or cube and name it `Pickup`.
2. Add a collider component sized around it.
3. Add a `Script Component` and create a new script named `Pickup`.
4. Use the script guide to make it rotate, disappear, play audio, or update saved data when touched.

## 6. Playtest

1. Save the scene.
2. Press the editor Play button, or use `Ctrl+Enter`.
3. Stop Play mode before making structural scene edits.

If the Game view is empty, check that the active scene has an active camera. If physics does not
behave as expected, enable `Editor > Debug > Show Physics Colliders` and confirm the objects have the
colliders you intended.