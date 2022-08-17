[WorkbenchToolAttribute(name: "Import Objects Extended", description: "Import Objects Extended allows you to bulk-place entities using data specified in a CSV file.\nIt is an extended version of the Import Objects Tool that also includes features such as:\n- Randomization\n- Terrain and Normal Snapping\n- Y Offset\nFor the CSV format, see ioe_example.csv.", wbModules: { "WorldEditor" }, awesomeFontCode: 0xf0c6)]
class ImportObjectsExtended: WorldEditorTool
{
	[Attribute("", UIWidgets.ResourceAssignArray, "Prefab Pool. If object hashes are already specified in the CSV file, this is not needed.\nAvoid empty elements.", "et")]
	protected ref array<ResourceName> m_PrefabVariants;
	
	[Attribute("-1", UIWidgets.EditBox, "How close to the terrain snapping should occur (if less than 0, never).", "-1, 100000")]
	float m_terrainSnapDistance;
	
	[Attribute("0", UIWidgets.EditBox, "Terrain Normal Snapping. Input is weight of terrain normals between 0 and 1.\nIf 0, do nothing. If 1, use terrain normals for rotation. If 0.5, take the average.\nOnly done if regular snapping occured.", "0, 1")]
	float m_normalSnapping;
	
	[Attribute("0", UIWidgets.EditBox, "Y Offset (post terrain snap, pre normal terrain snap).", "-1000, 1000")]
	float m_heightOffset;
	
	[Attribute("0", UIWidgets.CheckBox, "Randomize scale of placed objects (otherwise from CSV).")]
	bool m_randomScale;
	
	[Attribute("1", UIWidgets.EditBox, "Min Randomized Size.", "0.001 1000")]
	float m_minSize;
	
	[Attribute("1", UIWidgets.EditBox, "Max Randomized Size.", "0.001 1000")]
	float m_maxSize;
	
	[Attribute("0", UIWidgets.CheckBox, "Randomize rotatation of placed objects (otherwise from JSON).")]
	bool m_randomRotation;
	
	[Attribute("0", UIWidgets.EditBox, "Random Pitch.", "0 180")]
	float m_pitch;
	
	[Attribute("0", UIWidgets.EditBox, "Random Yaw.", "0 180")]
	float m_yaw;
	
	[Attribute("0", UIWidgets.EditBox, "Random Roll.", "0 180")]
	float m_roll;
	
	[Attribute("0", UIWidgets.CheckBox, "CSV uses vector angles (instead of degrees) for rotation.")]
	bool m_vectorAngles;
	
	[Attribute("", UIWidgets.ResourceNamePicker, "CSV file.\nFormat: optionalResourceHash PosX PosY PosZ Pitch Yaw Roll Scale\nIf no Resource Hash is specified, a random entity is selected from the Prefab Pool.", "csv")]
	ResourceName DataPath;

	ref array<IEntity> m_arrayOfEntities;
	
	
	[ButtonAttribute("Place")]
	void PlaceEntities()
	{
		ParseHandle parser = FileIO.BeginParse(DataPath.GetPath());
		if (parser == 0)
		{
			Print("Could not create parser from file: " + DataPath.GetPath(), LogLevel.ERROR);
			return;
		}

		int quatStart;
		int posStart;
		int scaleStart;
		
		bool TNS = false;
		array<string> tokens = new array<string>();
		
		bool relativeY;
		map<string, bool> hm = new map<string, bool>;
		
		if (m_API.BeginEntityAction())
		{
			for (int i = 0; true; ++i)
			{
				
				quatStart = 3;
				posStart = 0;
				scaleStart = 6;
				
				ResourceName rName;
				
				int numTokens = parser.ParseLine(i, tokens);
							
				if (numTokens == 0) {
					break;
				} else if (numTokens == 8) {
					rName = tokens[0];
					quatStart = 4;
					posStart = 1;
					scaleStart = 7;
				} else if (numTokens != 7) {
					Print("Line " + i + ": Invalid data format: Expected 7-8 tokens, got " + numTokens + ":", LogLevel.ERROR);
					for (int j = 0; j < numTokens; ++j)
					{
						Print(tokens[j]);
					}
					continue;
				} else {
					rName = m_PrefabVariants.GetRandomElement();
				}
				
				EntityFlags flags;
				
				relativeY;
				if (hm.Contains(rName)) {
					relativeY = hm.Get(rName);
				} else {
					SCR_BaseContainerTools.FindEntitySource(Resource.Load(rName)).Get("Flags", flags);
					relativeY = (flags & EntityFlags.RELATIVE_Y);
					hm.Set(rName, relativeY);
				}
				
				
				vector pos; // CUSTOM
				if (numTokens == 8) {
					pos[0] = tokens[1].ToFloat();
					pos[1] = tokens[2].ToFloat();
					pos[2] = tokens[3].ToFloat();
				} else {
					pos[0] = tokens[0].ToFloat();
					pos[1] = tokens[1].ToFloat();
					pos[2] = tokens[2].ToFloat();
				}
				
							
				float scale = tokens[scaleStart].ToFloat();
				
				vector rot;
				
				float terrainY;
				bool terrain;
					
				terrain = m_API.TryGetTerrainSurfaceY(pos[0], pos[2], terrainY);
				
				if (terrain && m_terrainSnapDistance >= Math.AbsFloat(terrainY-pos[1])) {
					if (!relativeY) {
						pos[1] = terrainY + m_heightOffset;
					} else {
						pos[1] = 0 + m_heightOffset;
					}
					TNS = true;
				} else if (relativeY) {
					pos[1] = pos[1] - terrainY + m_heightOffset;
				}
				
				if (m_randomRotation) {
					rot = Vector(Math.RandomFloat(0, m_pitch), Math.RandomFloat(0, m_yaw), Math.RandomFloat(0, m_roll));
				} else if (m_vectorAngles) {
					rot = Vector(tokens[quatStart].ToFloat(), tokens[quatStart+1].ToFloat(), tokens[quatStart+2].ToFloat()).VectorToAngles();
				} else {
					rot = Vector(tokens[quatStart].ToFloat(), tokens[quatStart+1].ToFloat(), tokens[quatStart+2].ToFloat());
				}
				
				IEntity entity = m_API.CreateEntity(rName, "", m_API.GetCurrentEntityLayerId(), null, pos, rot);
				if (entity == null) {
					Print("Entity " + rName + " could not be created!", LogLevel.ERROR);
				}
				m_arrayOfEntities.Insert(entity);
				
				if (m_randomScale) {
					m_API.ModifyEntityKey(entity, "scale", (Math.RandomFloat(m_minSize,m_maxSize)).ToString());
				} else {
					m_API.ModifyEntityKey(entity, "scale", scale.ToString());
				}
			
				if (m_normalSnapping > 0 && TNS) {
					int selectedCount = m_API.GetSelectedEntitiesCount();
					vector transform[4], parentTransform[4];
					vector angles, pos2;
					IEntity parent;
					BaseWorld world = m_API.GetWorld();
					entity.GetWorldTransform(transform);
					
					if (SCR_Global.SnapAndOrientToTerrain(transform, world)) {
						parent = m_API.SourceToEntity(m_API.EntityToSource(entity).GetParent());
						if (parent)
						{
							parent.GetWorldTransform(parentTransform);
							Math3D.MatrixInvMultiply4(parentTransform, transform, transform);
						}
						angles = Math3D.MatrixToAngles(transform);
						m_API.ModifyEntityKey(entity, "angleX", (angles[1] * m_normalSnapping + rot[0] * (1 - m_normalSnapping)).ToString());
						m_API.ModifyEntityKey(entity, "angleY", (angles[0] * m_normalSnapping + rot[1] * (1 - m_normalSnapping)).ToString());
						m_API.ModifyEntityKey(entity, "angleZ", (angles[2] * m_normalSnapping + rot[2] * (1 - m_normalSnapping)).ToString());
						
						pos2 = transform[3];
						
						//--- Y height relative to ground, use 0 instead of ASL height
						if (relativeY)
							pos2[1] = 0;
						
						m_API.ModifyEntityKey(entity, "coords", string.Format("%1 %2 %3", pos2[0], pos2[1], pos2[2]));
					}
					
				}
			}
			m_API.EndEntityAction();
		}
		parser.EndParse();
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