[EntityEditorProps(category: "LCN/Triggers", description: "Visible trigger zone that launches a built-in GM effect module after someone stays inside for a configurable time", color: "255 96 32 255", color2: "255 96 32 48", visible: true, style: "sphere", dynamicBox: true)]
class LCN_MortarStrikeTriggerEntityClass : GenericEntityClass
{
}

class LCN_MortarStrikeTriggerEntity : GenericEntity
{
	[Attribute("PrefabsEditable/EffectsModules/Mine/BaseEffectModule_MineField_Medium.et", UIWidgets.ResourcePickerThumbnail, "Built-in GM effect module prefab to spawn when the trigger fires", "et", category: "Effect")]
	protected ResourceName m_sEffectModulePrefab;

	[Attribute("0", UIWidgets.EditBox, "Optional extra delay before the effect module is spawned", params: "0 300 0.1", category: "Effect")]
	protected float m_fInitialEffectDelay;

	[Attribute("0", UIWidgets.CheckBox, "Allow the trigger to fire again after everyone leaves the zone", category: "Effect")]
	protected bool m_bAllowRetrigger;

	[Attribute("", UIWidgets.EditBox, "Faction key that can trigger the effect. Leave empty to allow any faction", category: "Filter")]
	protected FactionKey m_sTriggerFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Treat inherited factions as valid too", category: "Filter")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("25", UIWidgets.EditBox, "Zone radius in meters", params: "1 500 1", category: "Trigger")]
	protected float m_fTriggerRadius;

	[Attribute("30", UIWidgets.EditBox, "How long a character must stay in the zone before the effect starts", params: "1 1800 1", category: "Trigger")]
	protected float m_fRequiredPresenceTime;

	[Attribute("0.25", UIWidgets.EditBox, "How often the zone is scanned", params: "0.05 5 0.05", category: "Trigger")]
	protected float m_fCheckPeriod;

	protected ref array<IEntity> m_aNearbyCharacters = {};
	protected ref array<IEntity> m_aTrackedCharacters = {};
	protected ref array<float> m_aTrackedDurations = {};

	protected float m_fCheckDelay;
	protected float m_fPendingEffectDelay;
	protected bool m_bHasTriggered;
	protected bool m_bEffectPending;

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

		if (m_bEffectPending)
			UpdatePendingEffect(owner, timeSlice);

		m_fCheckDelay -= timeSlice;
		if (m_fCheckDelay > 0)
			return;

		m_fCheckDelay = m_fCheckPeriod;
		ScanZone(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePendingEffect(IEntity owner, float timeSlice)
	{
		m_fPendingEffectDelay -= timeSlice;
		if (m_fPendingEffectDelay > 0)
			return;

		m_bEffectPending = false;
		SpawnEffectModule(owner);
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
				StartEffectSequence(owner);
		}

		m_aTrackedCharacters = nextTracked;
		m_aTrackedDurations = nextDurations;

		if (m_bAllowRetrigger && m_aTrackedCharacters.IsEmpty() && !m_bEffectPending)
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
	protected void StartEffectSequence(IEntity owner)
	{
		if (!m_sEffectModulePrefab)
		{
			Print("LCN_MortarStrikeTriggerEntity: no effect module prefab configured");
			return;
		}

		Print(string.Format("LCN_MortarStrikeTriggerEntity: effect queued at %1", owner.GetOrigin().ToString()));
		m_fPendingEffectDelay = m_fInitialEffectDelay;
		m_bEffectPending = true;
		m_bHasTriggered = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnEffectModule(IEntity owner)
	{
		Resource effectResource = Resource.Load(m_sEffectModulePrefab);
		if (!effectResource || !effectResource.IsValid())
		{
			Print(string.Format("LCN_MortarStrikeTriggerEntity: failed to load effect module '%1'", m_sEffectModulePrefab));
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		owner.GetTransform(spawnParams.Transform);

		IEntity effectEntity = GetGame().SpawnEntityPrefab(effectResource, owner.GetWorld(), spawnParams);
		if (!effectEntity)
		{
			Print("LCN_MortarStrikeTriggerEntity: effect module spawn failed");
			return;
		}

		Print(string.Format("LCN_MortarStrikeTriggerEntity: effect module spawned '%1'", m_sEffectModulePrefab));
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
