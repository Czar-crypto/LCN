[EntityEditorProps(category: "LCN/Alarm", description: "Visible faction trigger zone that activates a base alarm objective", color: "255 32 32 255", color2: "255 32 32 48", visible: true, style: "sphere", dynamicBox: true)]
class LCN_BaseAlarmZoneEntityClass : GenericEntityClass
{
}

class LCN_BaseAlarmZoneEntity : GenericEntity
{
	[Attribute("LCN_BASE_ALARM_01", UIWidgets.EditBox, "LCN objective key activated by this zone", category: "LCN Alarm")]
	protected string m_sAlarmObjectiveKey;

	[Attribute("Gen_1", UIWidgets.EditBox, "Optional objective key required for automatic alarm activation", category: "LCN Alarm")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Second optional objective key required for automatic alarm activation", category: "LCN Alarm")]
	protected string m_sRequiredObjectiveKey2;

	[Attribute("", UIWidgets.EditBox, "Third optional objective key required for automatic alarm activation", category: "LCN Alarm")]
	protected string m_sRequiredObjectiveKey3;

	[Attribute("1", UIWidgets.CheckBox, "Allow the zone to activate alarm again after alarm duration expires", category: "LCN Alarm")]
	protected bool m_bAllowRetrigger;

	[Attribute("USSR", UIWidgets.EditBox, "Faction key that can trigger the alarm. Leave empty to allow any faction", category: "Filter")]
	protected FactionKey m_sTriggerFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Treat inherited trigger factions as valid too", category: "Filter")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("0", UIWidgets.CheckBox, "Require the blocking faction to be absent while the triggering faction is present", category: "Filter")]
	protected bool m_bRequireBlockingFactionCleared;

	[Attribute("0", UIWidgets.CheckBox, "Require that only the triggering faction is present in the zone", category: "Filter")]
	protected bool m_bRequireOnlyTriggerFactionPresent;

	[Attribute("US", UIWidgets.EditBox, "Faction key that can block the alarm while present", category: "Filter")]
	protected FactionKey m_sBlockingFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Treat inherited blocking factions as valid too", category: "Filter")]
	protected bool m_bAcceptInheritedBlockingFaction;

	[Attribute("1", UIWidgets.CheckBox, "Ignore destroyed characters in the zone scan", category: "Filter")]
	protected bool m_bIgnoreDeadCharacters;

	[Attribute("35", UIWidgets.EditBox, "Zone radius in meters", params: "1 1000 1", category: "Trigger")]
	protected float m_fTriggerRadius;

	[Attribute("5", UIWidgets.EditBox, "Seconds the condition must stay true before alarm starts", params: "0 1800 1", category: "Trigger")]
	protected float m_fRequiredPresenceTime;

	[Attribute("0.5", UIWidgets.EditBox, "How often the zone is scanned", params: "0.05 10 0.05", category: "Trigger")]
	protected float m_fCheckPeriod;

	[Attribute("1", UIWidgets.CheckBox, "Print zone state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected ref array<IEntity> m_aNearbyCharacters = {};

	protected float m_fCheckDelay;
	protected float m_fConditionHeldTime;
	protected bool m_bHasTriggered;

	//------------------------------------------------------------------------------------------------
	void LCN_BaseAlarmZoneEntity(IEntitySource src, IEntity parent)
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
		if (!IsRequiredObjectiveActive(owner))
		{
			m_fConditionHeldTime = 0;
			return;
		}

		if (m_bAllowRetrigger && !IsAlarmActive(owner))
			m_bHasTriggered = false;

		m_aNearbyCharacters.Clear();
		owner.GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), m_fTriggerRadius, QueryEntitiesCallback, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		bool conditionMet = EvaluateTriggerCondition();
		if (conditionMet)
		{
			m_fConditionHeldTime += m_fCheckPeriod;
			if (!m_bHasTriggered && m_fConditionHeldTime >= m_fRequiredPresenceTime)
				ActivateAlarm(owner);
		}
		else
		{
			m_fConditionHeldTime = 0;
			if (m_bAllowRetrigger)
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

		if (m_bIgnoreDeadCharacters && !IsEntityAlive(character))
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
			if (m_bDebug)
				Print(string.Format("LCN_BaseAlarmZoneEntity: only-trigger check trigger=%1 blocking=%2 other=%3 result=%4", triggerCount, blockingCount, otherCount, onlyTriggerFactionPresent));
			return onlyTriggerFactionPresent;
		}

		if (m_bRequireBlockingFactionCleared)
		{
			bool blockingCleared = !blockingPresent && triggeringPresent;
			if (m_bDebug)
				Print(string.Format("LCN_BaseAlarmZoneEntity: cleared-blocking check trigger=%1 blocking=%2 other=%3 result=%4", triggerCount, blockingCount, otherCount, blockingCleared));
			return blockingCleared;
		}

		if (m_bDebug)
			Print(string.Format("LCN_BaseAlarmZoneEntity: simple trigger check trigger=%1 blocking=%2 other=%3 result=%4", triggerCount, blockingCount, otherCount, triggeringPresent));

		return triggeringPresent;
	}

	//------------------------------------------------------------------------------------------------
	protected void ActivateAlarm(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		int count = LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sAlarmObjectiveKey, true, world);
		m_bHasTriggered = count > 0;

		if (m_bDebug)
			Print(string.Format("LCN_BaseAlarmZoneEntity: alarm '%1' activated, count=%2", m_sAlarmObjectiveKey, count));
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsAlarmActive(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sAlarmObjectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredObjectiveActive(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!IsRequiredObjectiveKeyActive(m_sRequiredObjectiveKey, world))
			return false;

		if (!IsRequiredObjectiveKeyActive(m_sRequiredObjectiveKey2, world))
			return false;

		return IsRequiredObjectiveKeyActive(m_sRequiredObjectiveKey3, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredObjectiveKeyActive(string objectiveKey, BaseWorld world)
	{
		if (objectiveKey.IsEmpty())
			return true;

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(objectiveKey, world);
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
	protected bool IsEntityAlive(IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return false;

		return true;
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
