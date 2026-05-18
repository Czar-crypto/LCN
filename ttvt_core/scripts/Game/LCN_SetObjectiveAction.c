class LCN_SetObjectiveAction : SCR_ScriptedUserAction
{
	[Attribute("Activate objective", UIWidgets.EditBox, "Action name shown to players", category: "LCN Objective")]
	protected string m_sActionName;

	[Attribute("LCN_BASE_ALARM_01", UIWidgets.EditBox, "LCN objective key to set", category: "LCN Objective")]
	protected string m_sTargetObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Fallback entity name if no objective key is set", category: "LCN Objective")]
	protected string m_sTargetEntityName;

	[Attribute("1", UIWidgets.CheckBox, "Target active state after this action is performed", category: "LCN Objective")]
	protected bool m_bSetActive;

	[Attribute("", UIWidgets.EditBox, "Optional LCN objective key that must be active before this action can be used", category: "LCN Objective")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("US", UIWidgets.EditBox, "Only this faction can use the action. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking the allowed faction key", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("0", UIWidgets.CheckBox, "Disable this action forever after successful use", category: "LCN Objective")]
	protected bool m_bOneUse;

	[Attribute("1", UIWidgets.CheckBox, "Print action state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected bool m_bUsed;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!IsMaster())
			return;

		if (!CanBePerformedScript(pUserEntity))
			return;

		BaseWorld world = null;
		if (pOwnerEntity)
			world = pOwnerEntity.GetWorld();

		int setCount = 0;
		LCN_ObjectiveStateComponent objective = null;
		if (!m_sTargetObjectiveKey.IsEmpty())
		{
			setCount = LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sTargetObjectiveKey, m_bSetActive, world);
			if (setCount <= 0)
				return;
		}
		else
		{
			objective = ResolveTargetObjective(pOwnerEntity);
			if (!objective)
				return;

			objective.SetObjectiveActive(m_bSetActive);
			setCount = 1;
		}

		m_bUsed = true;

		if (m_bOneUse)
			SetActionEnabled_S(false);

		if (m_bDebug)
		{
			if (!m_sTargetObjectiveKey.IsEmpty())
				Print(string.Format("LCN_SetObjectiveAction: key '%1' set active=%2 by %3, count=%4", m_sTargetObjectiveKey, m_bSetActive, pUserEntity, setCount));
			else if (objective)
				Print(string.Format("LCN_SetObjectiveAction: '%1' set active=%2 by %3", objective.GetObjectiveKey(), m_bSetActive, pUserEntity));
		}
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		if (m_bOneUse && m_bUsed)
			return false;

		IEntity owner = GetOwner();
		return IsRequiredObjectiveActive(owner) && HasTargetObjective(owner) && IsTargetStateDifferent(owner);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_bOneUse && m_bUsed)
		{
			SetCannotPerformReason("Already used");
			return false;
		}

		IEntity owner = GetOwner();
		if (!owner || !IsEntityAlive(owner))
		{
			SetCannotPerformReason("Station destroyed");
			return false;
		}

		if (!IsUserFactionAllowed(user))
		{
			SetCannotPerformReason("Wrong faction");
			return false;
		}

		if (!IsRequiredObjectiveActive(owner))
		{
			SetCannotPerformReason("Required objective offline");
			return false;
		}

		if (!HasTargetObjective(owner))
		{
			SetCannotPerformReason("Objective missing");
			return false;
		}

		if (!IsTargetStateDifferent(owner))
		{
			SetCannotPerformReason("Already set");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = m_sActionName;
		return !outName.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	protected LCN_ObjectiveStateComponent ResolveTargetObjective(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!m_sTargetObjectiveKey.IsEmpty())
			return LCN_ObjectiveStateComponent.FindObjective(m_sTargetObjectiveKey, world);

		IEntity target = owner;
		if (!m_sTargetEntityName.IsEmpty() && world)
			target = world.FindEntityByName(m_sTargetEntityName);

		if (!target)
			return null;

		return LCN_ObjectiveStateComponent.Cast(target.FindComponent(LCN_ObjectiveStateComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasTargetObjective(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!m_sTargetObjectiveKey.IsEmpty())
			return LCN_ObjectiveStateComponent.FindObjective(m_sTargetObjectiveKey, world) != null;

		return ResolveTargetObjective(owner) != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsTargetStateDifferent(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		bool currentActive;
		if (!m_sTargetObjectiveKey.IsEmpty())
		{
			currentActive = LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sTargetObjectiveKey, world);
			return currentActive != m_bSetActive;
		}

		LCN_ObjectiveStateComponent objective = ResolveTargetObjective(owner);
		if (!objective)
			return false;

		currentActive = objective.IsObjectiveActive();
		return currentActive != m_bSetActive;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredObjectiveActive(IEntity owner)
	{
		if (m_sRequiredObjectiveKey.IsEmpty())
			return true;

		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sRequiredObjectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMaster()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			return gameMode.IsMaster();

		return Replication.IsServer();
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
