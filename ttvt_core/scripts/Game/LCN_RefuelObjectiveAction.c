class LCN_RefuelObjectiveAction : SCR_ScriptedUserAction
{
	[Attribute("Refuel generator", UIWidgets.EditBox, "Action name shown to players", category: "LCN Fuel")]
	protected string m_sActionName;

	[Attribute("Gen_1", UIWidgets.EditBox, "LCN fueled objective key to refuel", category: "LCN Fuel")]
	protected string m_sTargetObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Optional target entity name. If set, this is used before Target Objective Key", category: "LCN Fuel")]
	protected string m_sTargetEntityName;

	[Attribute("300", UIWidgets.EditBox, "Fuel seconds added by this action", params: "1 7200 1", category: "LCN Fuel")]
	protected float m_fRefuelSeconds;

	[Attribute("0", UIWidgets.CheckBox, "Start the fueled objective immediately after refueling", category: "LCN Fuel")]
	protected bool m_bStartAfterRefuel;

	[Attribute("US", UIWidgets.EditBox, "Only this faction can use the action. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking the allowed faction key", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("1", UIWidgets.CheckBox, "Disable this fuel source after successful refuel", category: "LCN Fuel")]
	protected bool m_bOneUse;

	[Attribute("1", UIWidgets.CheckBox, "Print refuel action state to the script log", category: "LCN Debug")]
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

		float addedFuel = target.AddFuel(m_fRefuelSeconds, m_bStartAfterRefuel);
		if (addedFuel <= 0)
			return;

		m_bUsed = true;
		if (m_bOneUse)
			SetActionEnabled_S(false);

		if (m_bDebug)
			Print(string.Format("LCN_RefuelObjectiveAction: '%1' refueled by %2, added=%3s, oneUse=%4", target.GetObjectiveKey(), pUserEntity, addedFuel, m_bOneUse));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		if (m_bOneUse && m_bUsed)
			return false;

		if (!IsEntityAlive(GetOwner()))
			return false;

		LCN_FueledObjectiveComponent target = ResolveTarget(GetOwner());
		return target && target.CanAcceptFuel();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_bOneUse && m_bUsed)
		{
			SetCannotPerformReason("Fuel source empty");
			return false;
		}

		if (!IsUserFactionAllowed(user))
		{
			SetCannotPerformReason("Wrong faction");
			return false;
		}

		if (!IsEntityAlive(GetOwner()))
		{
			SetCannotPerformReason("Fuel source destroyed");
			return false;
		}

		if (m_fRefuelSeconds <= 0)
		{
			SetCannotPerformReason("No fuel configured");
			return false;
		}

		LCN_FueledObjectiveComponent target = ResolveTarget(GetOwner());
		if (!target)
		{
			SetCannotPerformReason("Generator missing");
			return false;
		}

		if (!target.CanAcceptFuel())
		{
			SetCannotPerformReason("Generator fuel full or destroyed");
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

	//------------------------------------------------------------------------------------------------
	protected bool IsEntityAlive(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (damageManager)
		{
			if (damageManager.IsDestroyed())
				return false;

			if (damageManager.GetState() == EDamageState.DESTROYED)
				return false;
		}

		return true;
	}
}
