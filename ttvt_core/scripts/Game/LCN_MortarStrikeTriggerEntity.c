[BaseContainerProps()]
class LCN_TriggeredFallingShellData
{
	IEntity m_ShellEntity;
	vector m_vVelocity;
}

[EntityEditorProps(category: "LCN/Triggers", description: "Visible mortar trigger zone with configurable delayed barrage", color: "255 96 32 255", color2: "255 96 32 48", visible: true, style: "sphere", dynamicBox: true)]
class LCN_MortarStrikeTriggerEntityClass : GenericEntityClass
{
}

class LCN_MortarStrikeTriggerEntity : GenericEntity
{
	[Attribute("", UIWidgets.ResourceAssignArray, "Projectile prefabs used by the modular mortar barrage effect", "et", category: "Barrage")]
	ref array<ResourceName> m_aProjectilePrefabs = {};

	[Attribute("{E15B8A4A6D904A2E}Prefabs/Weapons/Projectiles/Mortar/Ammo_Shell_82mm_HE_O832DU.et", UIWidgets.ResourcePickerThumbnail, "Fallback projectile prefab used when the array above is empty", "et", category: "Barrage")]
	protected ResourceName m_sFallbackProjectilePrefab;

	[Attribute("", UIWidgets.EditBox, "Faction key that can trigger the barrage. Leave empty to allow any faction", category: "Filter")]
	protected FactionKey m_sTriggerFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Treat inherited factions as valid too", category: "Filter")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("25", UIWidgets.EditBox, "Zone radius in meters", params: "1 500 1", category: "Trigger")]
	protected float m_fTriggerRadius;

	[Attribute("30", UIWidgets.EditBox, "How long a character must stay in the zone before the barrage starts", params: "1 1800 1", category: "Trigger")]
	protected float m_fRequiredPresenceTime;

	[Attribute("0.25", UIWidgets.EditBox, "How often the zone is scanned", params: "0.05 5 0.05", category: "Trigger")]
	protected float m_fCheckPeriod;

	[Attribute("300", UIWidgets.EditBox, "Barrage duration in seconds", params: "1 1800 1", category: "Barrage")]
	protected float m_fBarrageDuration;

	[Attribute("3.5", UIWidgets.EditBox, "Delay between impacts in seconds", params: "0.1 60 0.1", category: "Barrage")]
	protected float m_fRoundInterval;

	[Attribute("35", UIWidgets.EditBox, "Random impact radius around the trigger center", params: "1 500 1", category: "Barrage")]
	protected float m_fImpactRadius;

	[Attribute("0", UIWidgets.EditBox, "Optional extra delay before the first shell lands", params: "0 300 0.1", category: "Barrage")]
	protected float m_fInitialBarrageDelay;

	[Attribute("120", UIWidgets.EditBox, "Height in meters where shells are spawned above the impact point", params: "5 1000 1", category: "Barrage")]
	protected float m_fShellSpawnHeight;

	[Attribute("55", UIWidgets.EditBox, "Initial downward shell speed in m/s", params: "1 500 1", category: "Barrage")]
	protected float m_fInitialShellSpeed;

	[Attribute("0", UIWidgets.CheckBox, "Allow the trigger to fire again after everyone leaves the zone", category: "Barrage")]
	protected bool m_bAllowRetrigger;

	protected ref array<IEntity> m_aNearbyCharacters = {};
	protected ref array<IEntity> m_aTrackedCharacters = {};
	protected ref array<float> m_aTrackedDurations = {};
	protected ref array<ref Resource> m_aLoadedProjectilePrefabs = {};
	protected ref array<ref LCN_TriggeredFallingShellData> m_aActiveShells = {};

	protected float m_fCheckDelay;
	protected float m_fBarrageTimeUntilNextShot;
	protected float m_fBarrageTimeRemaining;
	protected int m_iCurrentProjectileIndex;
	protected bool m_bHasTriggered;
	protected bool m_bBarrageActive;

	//------------------------------------------------------------------------------------------------
	void LCN_MortarStrikeTriggerEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	protected void EnsureProjectilePrefabs()
	{
		if (!m_aProjectilePrefabs)
			m_aProjectilePrefabs = {};

		if (!m_aProjectilePrefabs.IsEmpty())
			return;

		if (m_sFallbackProjectilePrefab)
			m_aProjectilePrefabs.Insert(m_sFallbackProjectilePrefab);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		if (m_bBarrageActive)
			UpdateBarrage(owner, timeSlice);

		m_fCheckDelay -= timeSlice;
		if (m_fCheckDelay > 0)
			return;

		m_fCheckDelay = m_fCheckPeriod;
		ScanZone(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScanZone(IEntity owner)
	{
		m_aNearbyCharacters.Clear();
		owner.GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), m_fTriggerRadius, QueryEntitiesCallback, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		array<IEntity> nextTracked = {};
		array<float> nextDurations = {};

		foreach (IEntity character : m_aNearbyCharacters)
		{
			float duration = m_fCheckPeriod;
			int existingIndex = m_aTrackedCharacters.Find(character);
			if (existingIndex != -1)
				duration = m_aTrackedDurations[existingIndex] + m_fCheckPeriod;

			nextTracked.Insert(character);
			nextDurations.Insert(duration);

			if (!m_bHasTriggered && duration >= m_fRequiredPresenceTime)
				StartBarrage(owner);
		}

		m_aTrackedCharacters = nextTracked;
		m_aTrackedDurations = nextDurations;

		if (m_bAllowRetrigger && m_aTrackedCharacters.IsEmpty())
			m_bHasTriggered = false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool QueryEntitiesCallback(IEntity entity)
	{
		if (!entity)
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		if (!CanFactionTrigger(character))
			return false;

		m_aNearbyCharacters.Insert(entity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanFactionTrigger(ChimeraCharacter character)
	{
		if (m_sTriggerFactionKey.IsEmpty())
			return true;

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return false;

		Faction faction = factionAffiliation.GetAffiliatedFaction();
		if (!faction)
			return false;

		if (faction.GetFactionKey() == m_sTriggerFactionKey)
			return true;

		SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
		if (m_bAcceptInheritedFaction && scriptedFaction && scriptedFaction.IsInherited(m_sTriggerFactionKey))
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void StartBarrage(IEntity owner)
	{
		EnsureProjectilePrefabs();

		if (m_aProjectilePrefabs.IsEmpty())
		{
			Print("LCN_MortarStrikeTriggerEntity: no projectile prefabs configured");
			return;
		}

		m_aLoadedProjectilePrefabs.Clear();
		foreach (ResourceName projectilePrefab : m_aProjectilePrefabs)
		{
			if (!projectilePrefab)
				continue;

			Resource loadedResource = Resource.Load(projectilePrefab);
			if (loadedResource && loadedResource.IsValid())
				m_aLoadedProjectilePrefabs.Insert(loadedResource);
		}

		if (m_aLoadedProjectilePrefabs.IsEmpty())
		{
			Print("LCN_MortarStrikeTriggerEntity: projectile prefabs failed to load");
			return;
		}

		Print(string.Format("LCN_MortarStrikeTriggerEntity: barrage started at %1", owner.GetOrigin().ToString()));
		m_fBarrageTimeUntilNextShot = m_fInitialBarrageDelay;
		m_fBarrageTimeRemaining = m_fBarrageDuration;
		m_iCurrentProjectileIndex = 0;
		m_bBarrageActive = true;
		m_bHasTriggered = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateBarrage(IEntity owner, float timeSlice)
	{
		UpdateFallingShells(owner, timeSlice);

		m_fBarrageTimeRemaining -= timeSlice;
		m_fBarrageTimeUntilNextShot -= timeSlice;

		if (m_fBarrageTimeRemaining <= 0)
		{
			if (m_aActiveShells.IsEmpty())
				m_bBarrageActive = false;

			return;
		}

		if (m_fBarrageTimeUntilNextShot > 0)
			return;

		m_fBarrageTimeUntilNextShot += m_fRoundInterval;
		SpawnShell(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFallingShells(IEntity owner, float timeSlice)
	{
		vector gravity = PhysicsWorld.GetGravity(GetGame().GetWorldEntity());

		for (int i = m_aActiveShells.Count() - 1; i >= 0; i--)
		{
			LCN_TriggeredFallingShellData shellData = m_aActiveShells[i];
			if (!shellData || !shellData.m_ShellEntity)
			{
				m_aActiveShells.Remove(i);
				continue;
			}

			vector startPos = shellData.m_ShellEntity.GetOrigin();
			shellData.m_vVelocity = shellData.m_vVelocity + gravity * timeSlice;
			vector endPos = startPos + shellData.m_vVelocity * timeSlice;

			autoptr TraceParam trace = new TraceParam();
			trace.Start = startPos;
			trace.End = endPos;
			trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			trace.Exclude = shellData.m_ShellEntity;

			float traced = owner.GetWorld().TraceMove(trace, null);
			if (traced < 1)
			{
				vector hitPos = startPos + (endPos - startPos) * traced;
				shellData.m_ShellEntity.SetOrigin(hitPos);
				TriggerShell(shellData.m_ShellEntity);
				m_aActiveShells.Remove(i);
				continue;
			}

			shellData.m_ShellEntity.SetOrigin(endPos);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnShell(IEntity owner)
	{
		if (m_aLoadedProjectilePrefabs.IsEmpty())
			return;

		Resource prefab = m_aLoadedProjectilePrefabs[m_iCurrentProjectileIndex];
		m_iCurrentProjectileIndex++;
		if (m_iCurrentProjectileIndex >= m_aLoadedProjectilePrefabs.Count())
			m_iCurrentProjectileIndex = 0;

		if (!prefab)
			return;

		vector spawnPos = owner.GetOrigin();
		spawnPos[0] = spawnPos[0] + Math.RandomFloat(-m_fImpactRadius, m_fImpactRadius);
		spawnPos[2] = spawnPos[2] + Math.RandomFloat(-m_fImpactRadius, m_fImpactRadius);
		spawnPos[1] = spawnPos[1] + m_fShellSpawnHeight;

		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = spawnPos;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		for (int i = 0; i < 4; i++)
			spawnParams.Transform[i] = transform[i];

		IEntity shellEntity = GetGame().SpawnEntityPrefab(prefab, owner.GetWorld(), spawnParams);
		if (!shellEntity)
		{
			Print("LCN_MortarStrikeTriggerEntity: shell spawn failed");
			return;
		}

		ref LCN_TriggeredFallingShellData shellData = new LCN_TriggeredFallingShellData();
		shellData.m_ShellEntity = shellEntity;
		shellData.m_vVelocity = Vector(0, -m_fInitialShellSpeed, 0);
		m_aActiveShells.Insert(shellData);
	}

	//------------------------------------------------------------------------------------------------
	protected void TriggerShell(IEntity shellEntity)
	{
		BaseTriggerComponent triggerComponent = BaseTriggerComponent.Cast(shellEntity.FindComponent(BaseTriggerComponent));
		if (triggerComponent)
			triggerComponent.OnUserTrigger(shellEntity);
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override void _WB_GetBoundBox(inout vector min, inout vector max, IEntitySource src)
	{
		float radius = Math.Max(m_fTriggerRadius, 1.0);
		min = Vector(-radius, -1, -radius);
		max = Vector(radius, 1, radius);
	}
#endif
}
