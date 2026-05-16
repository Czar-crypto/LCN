[EntityEditorProps(category: "LCN/Triggers", description: "Visible mortar trigger zone with configurable delayed barrage", color: "255 96 32 255", color2: "255 96 32 48", visible: true, style: "sphere", dynamicBox: true)]
class LCN_MortarStrikeTriggerEntityClass : GenericEntityClass
{
}

class LCN_MortarStrikeTriggerEntity : GenericEntity
{
	[Attribute("", UIWidgets.ResourceAssignArray, "Projectile prefabs used by the modular mortar barrage effect", "et", category: "Barrage")]
	ref array<ResourceName> m_aProjectilePrefabs = {};

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

		Faction faction = character.GetFaction();
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
		if (m_aProjectilePrefabs.IsEmpty())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		owner.GetTransform(spawnParams.Transform);

		LCN_ModularMortarBarrageEntity barrageEntity = LCN_ModularMortarBarrageEntity.Cast(GetGame().SpawnEntity(LCN_ModularMortarBarrageEntity, owner.GetWorld(), spawnParams));
		if (!barrageEntity)
			return;

		barrageEntity.Configure(m_aProjectilePrefabs, m_fRoundInterval, m_fBarrageDuration, m_fImpactRadius, m_fInitialBarrageDelay);
		m_bHasTriggered = true;
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override void _WB_GetBoundBox(inout vector mins, inout vector maxs, IEntitySource src)
	{
		float radius = Math.Max(m_fTriggerRadius, 1.0);
		mins = Vector(-radius, -1, -radius);
		maxs = Vector(radius, 1, radius);
	}
#endif
}
