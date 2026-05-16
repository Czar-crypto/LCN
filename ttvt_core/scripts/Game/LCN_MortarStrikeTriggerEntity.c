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

	[Attribute("0", UIWidgets.CheckBox, "Allow the trigger to fire again after everyone leaves the zone", category: "Barrage")]
	protected bool m_bAllowRetrigger;

	protected ref array<IEntity> m_aNearbyCharacters = {};
	protected ref array<IEntity> m_aTrackedCharacters = {};
	protected ref array<float> m_aTrackedDurations = {};

	protected float m_fCheckDelay;
	protected bool m_bHasTriggered;

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

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		owner.GetTransform(spawnParams.Transform);

		LCN_ModularMortarBarrageEntity barrageEntity = LCN_ModularMortarBarrageEntity.Cast(GetGame().SpawnEntity(LCN_ModularMortarBarrageEntity, owner.GetWorld(), spawnParams));
		if (!barrageEntity)
		{
			Print("LCN_MortarStrikeTriggerEntity: failed to spawn barrage helper entity");
			return;
		}

		Print(string.Format("LCN_MortarStrikeTriggerEntity: barrage started at %1", owner.GetOrigin().ToString()));
		barrageEntity.Configure(m_aProjectilePrefabs, m_fRoundInterval, m_fBarrageDuration, m_fImpactRadius, m_fInitialBarrageDelay);
		m_bHasTriggered = true;
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
