class LCN_StartFueledObjectiveAction : SCR_ScriptedUserAction
{
	[Attribute("Start generator", UIWidgets.EditBox, "Action name shown to players", category: "LCN Fuel")]
	protected string m_sActionName;

	[Attribute("Gen_1", UIWidgets.EditBox, "LCN fueled objective key to start", category: "LCN Fuel")]
	protected string m_sTargetObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Optional target entity name. If set, this is used before Target Objective Key", category: "LCN Fuel")]
	protected string m_sTargetEntityName;

	[Attribute("US", UIWidgets.EditBox, "Only this faction can use the action. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking the allowed faction key", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("0", UIWidgets.CheckBox, "Disable this action forever after successful use", category: "LCN Fuel")]
	protected bool m_bOneUse;

	[Attribute("1", UIWidgets.CheckBox, "Print start action state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected bool m_bUsed;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!IsMaster())
			return;

		if (!CanBePerformedScript(pUserEntity))
			return;

		LCN_FueledObjectiveComponent target = ResolveTarget(pOwnerEntity);
		if (!target)
			return;

		if (!target.StartObjective())
			return;

		m_bUsed = true;
		if (m_bOneUse)
			SetActionEnabled_S(false);

		if (m_bDebug)
			Print(string.Format("LCN_StartFueledObjectiveAction: '%1' started by %2, fuel=%3/%4", target.GetObjectiveKey(), pUserEntity, target.GetFuelSeconds(), target.GetMaxFuelSeconds()));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		if (m_bOneUse && m_bUsed)
			return false;

		LCN_FueledObjectiveComponent target = ResolveTarget(GetOwner());
		return target && target.CanStartObjective();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_bOneUse && m_bUsed)
		{
			SetCannotPerformReason("Already used");
			return false;
		}

		if (!IsUserFactionAllowed(user))
		{
			SetCannotPerformReason("Wrong faction");
			return false;
		}

		LCN_FueledObjectiveComponent target = ResolveTarget(GetOwner());
		if (!target)
		{
			SetCannotPerformReason("Generator missing");
			return false;
		}

		if (!target.CanStartObjective())
		{
			SetCannotPerformReason("No fuel or already running");
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
	protected LCN_FueledObjectiveComponent ResolveTarget(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!m_sTargetEntityName.IsEmpty() && world)
		{
			IEntity targetEntity = world.FindEntityByName(m_sTargetEntityName);
			if (targetEntity)
				return LCN_FueledObjectiveComponent.Cast(targetEntity.FindComponent(LCN_FueledObjectiveComponent));
		}

		if (!m_sTargetObjectiveKey.IsEmpty())
			return LCN_FueledObjectiveComponent.FindFueledObjective(m_sTargetObjectiveKey, world);

		if (!owner)
			return null;

		return LCN_FueledObjectiveComponent.Cast(owner.FindComponent(LCN_FueledObjectiveComponent));
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
}
