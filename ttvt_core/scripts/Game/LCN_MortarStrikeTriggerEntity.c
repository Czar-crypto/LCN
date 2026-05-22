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

	[Attribute("0", UIWidgets.CheckBox, "Require the blocking faction to be absent while the triggering faction is present", category: "Filter")]
	protected bool m_bRequireBlockingFactionCleared;

	[Attribute("0", UIWidgets.CheckBox, "Require that only the triggering faction is present in the zone", category: "Filter")]
	protected bool m_bRequireOnlyTriggerFactionPresent;

	[Attribute("", UIWidgets.EditBox, "Faction key that prevents the trigger while present in the zone", category: "Filter")]
	protected FactionKey m_sBlockingFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Treat inherited blocking factions as valid too", category: "Filter")]
	protected bool m_bAcceptInheritedBlockingFaction;

	[Attribute("25", UIWidgets.EditBox, "Zone radius in meters", params: "1 500 1", category: "Trigger")]
	protected float m_fTriggerRadius;

	[Attribute("30", UIWidgets.EditBox, "How long a character must stay in the zone before the effect starts", params: "1 1800 1", category: "Trigger")]
	protected float m_fRequiredPresenceTime;

	[Attribute("0.25", UIWidgets.EditBox, "How often the zone is scanned", params: "0.05 5 0.05", category: "Trigger")]
	protected float m_fCheckPeriod;

	protected ref array<IEntity> m_aNearbyCharacters = {};

	protected float m_fCheckDelay;
	protected float m_fPendingEffectDelay;
	protected float m_fConditionHeldTime;
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

		bool conditionMet = EvaluateTriggerCondition();
		if (conditionMet)
		{
			m_fConditionHeldTime += m_fCheckPeriod;
			if (!m_bHasTriggered && m_fConditionHeldTime >= m_fRequiredPresenceTime)
				StartEffectSequence(owner);
		}
		else
		{
			m_fConditionHeldTime = 0;
			if (m_bAllowRetrigger && !m_bEffectPending)
				m_bHasTriggered = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool QueryEntitiesCallback(IEntity entity)
	{
		if (!entity)
			return true;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return true;

		m_aNearbyCharacters.Insert(entity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool EvaluateTriggerCondition()
	{
		bool blockingPresent = false;
		bool triggeringPresent = false;
		int triggerCount = 0;
		int blockingCount = 0;
		int otherCount = 0;

		foreach (IEntity entity : m_aNearbyCharacters)
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(entity);
			if (!character)
				continue;

			bool isBlockingFaction = IsCharacterInFaction(character, m_sBlockingFactionKey, m_bAcceptInheritedBlockingFaction);
			bool isTriggeringFaction = IsCharacterInFaction(character, m_sTriggerFactionKey, m_bAcceptInheritedFaction);

			if (isBlockingFaction)
			{
				blockingPresent = true;
				blockingCount++;
			}

			if (m_sTriggerFactionKey.IsEmpty())
			{
				if (m_bRequireBlockingFactionCleared)
				{
					if (!isBlockingFaction)
					{
						triggeringPresent = true;
						triggerCount++;
					}
				}
				else
				{
					triggeringPresent = true;
					triggerCount++;
				}
			}
			else if (isTriggeringFaction)
			{
				triggeringPresent = true;
				triggerCount++;
			}
			else if (!isBlockingFaction)
			{
				otherCount++;
			}
		}

		if (m_bRequireOnlyTriggerFactionPresent)
		{
			bool onlyTriggerFactionPresent = triggerCount > 0 && blockingCount == 0 && otherCount == 0;
			Print(string.Format("LCN_MortarStrikeTriggerEntity: only-trigger check trigger=%1 blocking=%2 other=%3 result=%4", triggerCount, blockingCount, otherCount, onlyTriggerFactionPresent));
			return onlyTriggerFactionPresent;
		}

		if (m_bRequireBlockingFactionCleared)
		{
			bool blockingCleared = !blockingPresent && triggeringPresent;
			Print(string.Format("LCN_MortarStrikeTriggerEntity: cleared-blocking check trigger=%1 blocking=%2 other=%3 result=%4", triggerCount, blockingCount, otherCount, blockingCleared));
			return blockingCleared;
		}

		Print(string.Format("LCN_MortarStrikeTriggerEntity: simple trigger check trigger=%1 blocking=%2 other=%3 result=%4", triggerCount, blockingCount, otherCount, triggeringPresent));
		return triggeringPresent;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCharacterInFaction(ChimeraCharacter character, FactionKey factionKey, bool acceptInherited)
	{
		if (factionKey.IsEmpty())
			return false;

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return false;

		Faction faction = factionAffiliation.GetAffiliatedFaction();
		if (!faction)
			return false;

		if (faction.GetFactionKey() == factionKey)
			return true;

		SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
		if (acceptInherited && scriptedFaction && scriptedFaction.IsInherited(factionKey))
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
