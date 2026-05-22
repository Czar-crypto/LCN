[EntityEditorProps(category: "LCN/Triggers", description: "Visible base-clear zone that starts a configured built-in support effect after the defenders are gone", color: "255 160 32 255", color2: "255 160 32 48", visible: true, style: "sphere", dynamicBox: true)]
class LCN_BaseClearSupportTriggerEntityClass : GenericEntityClass
{
}

class LCN_BaseClearSupportTriggerEntity : GenericEntity
{
	[Attribute("{5D48E2F7DB0C3714}PrefabsEditable/EffectsModules/Mortar/EffectModule_Zoned_MortarBarrage_Small.et", UIWidgets.ResourcePickerThumbnail, "Built-in GM/support effect module spawned when the base is clear", "et", category: "Support")]
	protected ResourceName m_sEffectModulePrefab;

	[Attribute("10", UIWidgets.EditBox, "Delay before the support effect starts after the base is confirmed clear", params: "0 1800 1", category: "Support")]
	protected float m_fEffectDelay;

	[Attribute("0", UIWidgets.CheckBox, "Allow the trigger to arm again after the condition becomes false", category: "Support")]
	protected bool m_bAllowRetrigger;

	[Attribute("USSR", UIWidgets.EditBox, "Faction that must enter and clear the zone. Leave empty to allow any non-defender", category: "Factions")]
	protected FactionKey m_sAttackerFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited attacker factions", category: "Factions")]
	protected bool m_bAcceptInheritedAttackerFaction;

	[Attribute("US", UIWidgets.EditBox, "Faction that must be absent before support starts", category: "Factions")]
	protected FactionKey m_sDefenderFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited defender factions", category: "Factions")]
	protected bool m_bAcceptInheritedDefenderFaction;

	[Attribute("1", UIWidgets.CheckBox, "Require at least one attacker inside the zone", category: "Condition")]
	protected bool m_bRequireAttackerPresent;

	[Attribute("1", UIWidgets.CheckBox, "Require defenders to be absent from the zone", category: "Condition")]
	protected bool m_bRequireDefenderCleared;

	[Attribute("1", UIWidgets.CheckBox, "Do not block on civilians, neutrals, or other factions", category: "Condition")]
	protected bool m_bIgnoreOtherFactions;

	[Attribute("1", UIWidgets.CheckBox, "Do not count dead characters in the zone", category: "Condition")]
	protected bool m_bIgnoreDeadCharacters;

	[Attribute("25", UIWidgets.EditBox, "Zone radius in meters", params: "1 500 1", category: "Zone")]
	protected float m_fTriggerRadius;

	[Attribute("5", UIWidgets.EditBox, "How long the clear condition must stay true before support is queued", params: "0 1800 1", category: "Zone")]
	protected float m_fRequiredClearTime;

	[Attribute("0.25", UIWidgets.EditBox, "How often the zone is scanned", params: "0.05 5 0.05", category: "Zone")]
	protected float m_fCheckPeriod;

	[Attribute("1", UIWidgets.CheckBox, "Print zone counts to the script log", category: "Debug")]
	protected bool m_bDebug;

	protected ref array<ChimeraCharacter> m_aCharactersInZone = {};

	protected float m_fCheckDelay;
	protected float m_fConditionHeldTime;
	protected float m_fPendingEffectDelay;
	protected bool m_bHasTriggered;
	protected bool m_bEffectPending;

	//------------------------------------------------------------------------------------------------
	void LCN_BaseClearSupportTriggerEntity(IEntitySource src, IEntity parent)
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

		m_fCheckDelay = Math.Max(m_fCheckPeriod, 0.05);
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
		m_aCharactersInZone.Clear();
		owner.GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), m_fTriggerRadius, QueryEntitiesCallback, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		bool conditionMet = EvaluateClearCondition();
		if (conditionMet)
		{
			m_fConditionHeldTime += m_fCheckDelay;
			if (!m_bHasTriggered && m_fConditionHeldTime >= m_fRequiredClearTime)
				QueueEffect(owner);
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
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return true;

		if (m_bIgnoreDeadCharacters && !IsCharacterAlive(character))
			return true;

		m_aCharactersInZone.Insert(character);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool EvaluateClearCondition()
	{
		int attackerCount = 0;
		int defenderCount = 0;
		int otherCount = 0;

		foreach (ChimeraCharacter character : m_aCharactersInZone)
		{
			bool isDefender = IsCharacterInFaction(character, m_sDefenderFactionKey, m_bAcceptInheritedDefenderFaction);
			bool isAttacker = IsCharacterInFaction(character, m_sAttackerFactionKey, m_bAcceptInheritedAttackerFaction);

			if (isDefender)
				defenderCount++;
			else if (m_sAttackerFactionKey.IsEmpty() || isAttacker)
				attackerCount++;
			else
				otherCount++;
		}

		bool result = true;

		if (m_bRequireAttackerPresent && attackerCount == 0)
			result = false;

		if (m_bRequireDefenderCleared && defenderCount > 0)
			result = false;

		if (!m_bIgnoreOtherFactions && otherCount > 0)
			result = false;

		if (m_bDebug)
		{
			Print(string.Format("LCN_BaseClearSupportTriggerEntity: clear check attackers=%1 defenders=%2 other=%3 held=%4 result=%5",
				attackerCount,
				defenderCount,
				otherCount,
				m_fConditionHeldTime,
				result));
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCharacterAlive(ChimeraCharacter character)
	{
		CharacterControllerComponent controller = character.GetCharacterController();
		if (controller && controller.IsDead())
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(character);
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return false;

		return true;
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
		return acceptInherited && scriptedFaction && scriptedFaction.IsInherited(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	protected void QueueEffect(IEntity owner)
	{
		if (!m_sEffectModulePrefab)
		{
			Print("LCN_BaseClearSupportTriggerEntity: no effect module prefab configured");
			return;
		}

		m_fPendingEffectDelay = Math.Max(m_fEffectDelay, 0.0);
		m_bEffectPending = true;
		m_bHasTriggered = true;

		Print(string.Format("LCN_BaseClearSupportTriggerEntity: support effect queued at %1 after %2 seconds", owner.GetOrigin().ToString(), m_fPendingEffectDelay));
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnEffectModule(IEntity owner)
	{
		Resource effectResource = Resource.Load(m_sEffectModulePrefab);
		if (!effectResource || !effectResource.IsValid())
		{
			Print(string.Format("LCN_BaseClearSupportTriggerEntity: failed to load effect module '%1'", m_sEffectModulePrefab));
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		owner.GetTransform(spawnParams.Transform);

		IEntity effectEntity = GetGame().SpawnEntityPrefab(effectResource, owner.GetWorld(), spawnParams);
		if (!effectEntity)
		{
			Print("LCN_BaseClearSupportTriggerEntity: effect module spawn failed");
			return;
		}

		Print(string.Format("LCN_BaseClearSupportTriggerEntity: effect module spawned '%1'", m_sEffectModulePrefab));
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
