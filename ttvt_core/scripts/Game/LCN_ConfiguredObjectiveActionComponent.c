[ComponentEditorProps(category: "LCN/Objectives", description: "Workbench-facing settings for a user action that sets an LCN objective key")]
class LCN_ConfiguredObjectiveActionComponentClass : ScriptComponentClass
{
}

class LCN_ConfiguredObjectiveActionComponent : ScriptComponent
{
	[Attribute("Arm alarm system", UIWidgets.EditBox, "Action name shown to players", category: "LCN Action")]
	protected string m_sActionName;

	[Attribute("Enable the linked alarm trigger zone", UIWidgets.EditBox, "Action description shown to players", category: "LCN Action")]
	protected string m_sActionDescription;

	[Attribute("5", UIWidgets.EditBox, "How many seconds the action takes", params: "0 120 0.5", category: "LCN Action")]
	protected float m_fActionDuration;

	[Attribute("LCN_ALARM_ARMED_01", UIWidgets.EditBox, "Objective key this action sets", category: "LCN Objective")]
	protected string m_sTargetObjectiveKey;

	[Attribute("1", UIWidgets.CheckBox, "Target objective active state after the action", category: "LCN Objective")]
	protected bool m_bSetActive;

	[Attribute("1", UIWidgets.CheckBox, "Copy Target Objective Key to the owner's LCN_ObjectiveStateComponent on init", category: "LCN Objective")]
	protected bool m_bApplyTargetKeyToOwnerObjective;

	[Attribute("1", UIWidgets.CheckBox, "Only show/use the action when the target objective is in the opposite state", category: "LCN Objective")]
	protected bool m_bRequireTargetStateDifferent;

	[Attribute("Gen_1", UIWidgets.EditBox, "First objective key that must be active before this action can be used", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Second objective key that must be active before this action can be used", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey2;

	[Attribute("", UIWidgets.EditBox, "Third objective key that must be active before this action can be used", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey3;

	[Attribute("US", UIWidgets.EditBox, "Only this faction can use the action. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking the allowed faction key", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("1", UIWidgets.CheckBox, "Disable this action forever after successful use", category: "LCN Action")]
	protected bool m_bOneUse;

	[Attribute("1", UIWidgets.CheckBox, "Print action state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected bool m_bUsed;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		ApplyTargetKeyToOwnerObjective(owner);
	}

	//------------------------------------------------------------------------------------------------
	string GetActionName()
	{
		return m_sActionName;
	}

	//------------------------------------------------------------------------------------------------
	string GetActionDescription()
	{
		return m_sActionDescription;
	}

	//------------------------------------------------------------------------------------------------
	float GetActionDuration()
	{
		return Math.Max(m_fActionDuration, 0);
	}

	//------------------------------------------------------------------------------------------------
	bool CanBeShown(IEntity owner)
	{
		if (m_bOneUse && m_bUsed)
			return false;

		if (!AreRequiredObjectivesActive(owner))
			return false;

		if (!HasTargetObjective(owner))
			return false;

		return !m_bRequireTargetStateDifferent || IsTargetStateDifferent(owner);
	}

	//------------------------------------------------------------------------------------------------
	bool CanBePerformed(IEntity owner, IEntity user, out string reason)
	{
		reason = "";

		if (m_bOneUse && m_bUsed)
		{
			reason = "Already used";
			return false;
		}

		if (!owner || !IsEntityAlive(owner))
		{
			reason = "Station destroyed";
			return false;
		}

		if (!IsUserFactionAllowed(user))
		{
			reason = "Wrong faction";
			return false;
		}

		if (!AreRequiredObjectivesActive(owner))
		{
			reason = "Required objective offline";
			return false;
		}

		if (!HasTargetObjective(owner))
		{
			reason = "Objective missing";
			return false;
		}

		if (m_bRequireTargetStateDifferent && !IsTargetStateDifferent(owner))
		{
			reason = "Already set";
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Perform(IEntity owner, IEntity user)
	{
		int setCount = SetTargetObjective(owner);
		if (setCount <= 0)
			return false;

		m_bUsed = true;

		if (m_bDebug)
			Print(string.Format("LCN_ConfiguredObjectiveActionComponent: key '%1' set active=%2 by %3, count=%4", m_sTargetObjectiveKey, m_bSetActive, user, setCount));

		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOneUse()
	{
		return m_bOneUse;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyTargetKeyToOwnerObjective(IEntity owner)
	{
		if (!m_bApplyTargetKeyToOwnerObjective || m_sTargetObjectiveKey.IsEmpty() || !owner)
			return;

		LCN_ObjectiveStateComponent objective = LCN_ObjectiveStateComponent.Cast(owner.FindComponent(LCN_ObjectiveStateComponent));
		if (objective)
			objective.SetObjectiveKey(m_sTargetObjectiveKey);
	}

	//------------------------------------------------------------------------------------------------
	protected int SetTargetObjective(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		int setCount = LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sTargetObjectiveKey, m_bSetActive, world);
		if (setCount > 0)
			return setCount;

		LCN_ObjectiveStateComponent objective = LCN_ObjectiveStateComponent.Cast(owner.FindComponent(LCN_ObjectiveStateComponent));
		if (!objective)
			return 0;

		if (!m_sTargetObjectiveKey.IsEmpty() && objective.GetObjectiveKey() != m_sTargetObjectiveKey)
			return 0;

		objective.SetObjectiveActive(m_bSetActive);
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasTargetObjective(IEntity owner)
	{
		if (!owner)
			return false;

		BaseWorld world = owner.GetWorld();
		if (LCN_ObjectiveStateComponent.FindObjective(m_sTargetObjectiveKey, world))
			return true;

		LCN_ObjectiveStateComponent objective = LCN_ObjectiveStateComponent.Cast(owner.FindComponent(LCN_ObjectiveStateComponent));
		return objective && (m_sTargetObjectiveKey.IsEmpty() || objective.GetObjectiveKey() == m_sTargetObjectiveKey);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsTargetStateDifferent(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		bool currentActive = LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sTargetObjectiveKey, world);
		return currentActive != m_bSetActive;
	}

	//------------------------------------------------------------------------------------------------
	protected bool AreRequiredObjectivesActive(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!IsRequiredObjectiveActive(m_sRequiredObjectiveKey, world))
			return false;

		if (!IsRequiredObjectiveActive(m_sRequiredObjectiveKey2, world))
			return false;

		return IsRequiredObjectiveActive(m_sRequiredObjectiveKey3, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredObjectiveActive(string objectiveKey, BaseWorld world)
	{
		if (objectiveKey.IsEmpty())
			return true;

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(objectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsUserFactionAllowed(IEntity user)
	{
		if (m_sAllowedFactionKey.IsEmpty())
			return true;

		if (!user)
			return false;

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(user.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return false;

		Faction faction = factionAffiliation.GetAffiliatedFaction();
		if (!faction)
			return false;

		if (faction.GetFactionKey() == m_sAllowedFactionKey)
			return true;

		SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
		return m_bAcceptInheritedFaction && scriptedFaction && scriptedFaction.IsInherited(m_sAllowedFactionKey);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsEntityAlive(IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return false;

		return true;
	}
}
