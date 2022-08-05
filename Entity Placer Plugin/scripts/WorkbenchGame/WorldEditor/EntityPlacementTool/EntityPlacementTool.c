[WorkbenchToolAttribute(name: "Entity Placement Tool", description: "The Entity Placement Tool allows you to bulk-place entities at positions specified in a JSON file.\nFor the coordinate format, see ept_example.json.", wbModules: { "WorldEditor" }, awesomeFontCode: 0xf074)]
class EntityPlacementTool: WorldEditorTool
{
	[Attribute("", UIWidgets.ResourceAssignArray, "Prefab Pool. Avoid having empty elements.", "et")]
	protected ref array<ResourceName> m_PrefabVariants;
	
	[Attribute("-1", UIWidgets.EditBox, "How close to the terrain snapping should occur (if less than 0, never).", "-1, 100000")]
	float m_terrainSnapDistance;
	
	[Attribute("1", UIWidgets.EditBox, "Min Randomized Size.", "0.001 1000")]
	float m_minSize;
	
	[Attribute("1", UIWidgets.EditBox, "Max Randomized Size.", "0.001 1000")]
	float m_maxSize;
	
	[Attribute("0", UIWidgets.CheckBox, "JSON uses vector angles (not degrees) for rotation.")]
	bool m_vectorAngles;
	
	[Attribute("0", UIWidgets.CheckBox, "Randomize rotatation of placed objects (otherwise from JSON).")]
	bool m_RandomRotation;
	
	[Attribute("0", UIWidgets.EditBox, "Random Pitch.", "0 180")]
	float m_pitch;
	
	[Attribute("0", UIWidgets.EditBox, "Random Yaw.", "0 180")]
	float m_yaw;
	
	[Attribute("0", UIWidgets.EditBox, "Random Roll.", "0 180")]
	float m_roll;
	
	[Attribute("", UIWidgets.ResourceNamePicker, "Coordinate File of format: \nlist:[{PosX,PosY,PosZ,Yaw,Pitch,Roll},{...}]", "json")]
	ResourceName m_JSONFile;

	ref array<IEntity> m_arrayOfEntities;
	
	
	[ButtonAttribute("Place")]
	void PlaceEntities()
	{
		
		SCR_JsonLoadContext context = new SCR_JsonLoadContext();
		context.LoadFromFile(m_JSONFile.GetPath());

		array<ref EPT_CoordElement> coords;
		context.ReadValue("list", coords);
		
		
		int len = coords.Count();
		
		for (int i = 0; i < len; i++) {
			
			vector pos = vector.Zero;
			
			float terrainY;
			bool terrain = m_API.TryGetTerrainSurfaceY(0, 0, terrainY);
			
			if (terrain) {
				if (m_terrainSnapDistance >= Math.AbsFloat(terrainY-coords[i].PosY)) {
					// Snap To Terrain
					pos = Vector(coords[i].PosX, terrainY, coords[i].PosZ);
				} else {
					// Don't Snap
					pos = Vector(coords[i].PosX, coords[i].PosY, coords[i].PosZ);
				}
			} else {
				// Don't Snap
				pos = Vector(coords[i].PosX, coords[i].PosY, coords[i].PosZ);
			}
			
			vector rot = vector.Zero;
			
			if (m_RandomRotation) {
				rot = Vector(Math.RandomFloat(0, m_pitch), Math.RandomFloat(0, m_yaw), Math.RandomFloat(0, m_roll));
				rot.AnglesToVector();
			} else {
				rot = Vector(coords[i].Pitch, coords[i].Yaw, coords[i].Roll);
				if (m_vectorAngles) {
					rot.AnglesToVector();
				}
			}
			
			m_API.BeginEntityAction("Processing " + i);

			// Create entity using one of the selected random prefabs
			IEntity entity = m_API.CreateEntity(m_PrefabVariants.GetRandomElement(), "", m_API.GetCurrentEntityLayerId(), null,	pos, rot);
			m_arrayOfEntities.Insert(entity);

			m_API.ModifyEntityKey(entity, "scale", (Math.RandomFloat(m_minSize,m_maxSize)).ToString());

			m_API.EndEntityAction();
		}
	}
	
	// Replace all button
	[ButtonAttribute("Replace all")]
	void ReplaceAll()
	{
		DeleteAll();
		PlaceEntities();
	}

	// Delete all button
	[ButtonAttribute("Delete all")]
	void DeleteAll()
	{
		// Do nothing if array is empty
		if(!m_arrayOfEntities)
			return;

		// Delete all entities created by this tool
		m_API.BeginEntityAction("Deleting entities");
		m_API.DeleteEntities(m_arrayOfEntities);
		m_API.EndEntityAction();
		m_arrayOfEntities.Clear();
	}
	
	// Save all button
	[ButtonAttribute("Save all")]
	void SaveAll()
	{
		if (m_arrayOfEntities != null) {
			m_arrayOfEntities.Clear();
		}
	}


	override void OnActivate()
	{
		m_arrayOfEntities = new array<IEntity>;
	}

	override void OnDeActivate()
	{
		if (m_arrayOfEntities != null) {
			m_arrayOfEntities.Clear();
		}
	}
}