class LCN_DisableObjectiveAction : SCR_ScriptedUserAction
{
	[Attribute("Disable objective", UIWidgets.EditBox, "Action name shown to players", category: "LCN Objective")]
	protected string m_sActionName;

	[Attribute("LCN_COMMS_01", UIWidgets.EditBox, "LCN objective key to disable", category: "LCN Objective")]
	protected string m_sTargetObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Fallback entity name if no objective key is set", category: "LCN Objective")]
	protected string m_sTargetEntityName;

	[Attribute("USSR", UIWidgets.EditBox, "Only this faction can use the action. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking the allowed faction key", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("1", UIWidgets.CheckBox, "Disable this action forever after successful use", category: "LCN Objective")]
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

		LCN_ObjectiveStateComponent objective = ResolveTargetObjective(pOwnerEntity);
		if (!objective)
			return;

		objective.DisableObjective();
		m_bUsed = true;

		if (m_bOneUse)
			SetActionEnabled_S(false);

		if (m_bDebug)
			Print(string.Format("LCN_DisableObjectiveAction: '%1' disabled by %2", objective.GetObjectiveKey(), pUserEntity));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		if (m_bOneUse && m_bUsed)
			return false;

		LCN_ObjectiveStateComponent objective = ResolveTargetObjective(GetOwner());
		return objective && objective.IsObjectiveActive();
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

		LCN_ObjectiveStateComponent objective = ResolveTargetObjective(owner);
		if (!objective)
		{
			SetCannotPerformReason("Objective missing");
			return false;
		}

		if (!objective.IsObjectiveActive())
		{
			SetCannotPerformReason("Already disabled");
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
