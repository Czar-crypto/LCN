class LCN_OpenFireMissionTerminalAction : SCR_ScriptedUserAction
{
	[Attribute("", UIWidgets.EditBox, "Optional fire direction console entity name. Leave empty when this action is on the console itself", category: "LCN Links")]
	protected string m_sConsoleEntityName;

	[Attribute("0", UIWidgets.CheckBox, "Print client-side terminal debug to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		LCN_FireMissionConsoleComponent console = GetFireMissionConsole();
		if (!console)
			return;

		LCN_FireMissionTerminalWidgetComponent.OpenTerminal(console, m_sConsoleEntityName);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		return GetFireMissionConsole() != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		LCN_FireMissionConsoleComponent console = GetFireMissionConsole();
		if (!console)
		{
			SetCannotPerformReason("Fire direction center missing");
			return false;
		}

		string reason;
		if (!console.CanPerformAction(LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_COORDINATES, user, reason))
		{
			SetCannotPerformReason(reason);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionDescriptionScript(out string outName)
	{
		LCN_FireMissionConsoleComponent console = GetFireMissionConsole();
		if (!console)
			return false;

		outName = console.GetStatusText();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected LCN_FireMissionConsoleComponent GetFireMissionConsole()
	{
		IEntity owner = GetOwner();
		if (owner)
		{
			LCN_FireMissionConsoleComponent ownerConsole = LCN_FireMissionConsoleComponent.Cast(owner.FindComponent(LCN_FireMissionConsoleComponent));
			if (ownerConsole)
				return ownerConsole;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		LCN_FireMissionConsoleComponent registeredConsole = LCN_FireMissionConsoleComponent.FindConsole(m_sConsoleEntityName, world);
		if (registeredConsole)
			return registeredConsole;

		if (m_sConsoleEntityName.IsEmpty())
			return null;

		IEntity consoleEntity = world.FindEntityByName(m_sConsoleEntityName);
		if (!consoleEntity)
			return null;

		return LCN_FireMissionConsoleComponent.Cast(consoleEntity.FindComponent(LCN_FireMissionConsoleComponent));
	}
}
