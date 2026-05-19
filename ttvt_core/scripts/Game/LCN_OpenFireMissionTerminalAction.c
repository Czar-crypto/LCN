class LCN_OpenFireMissionTerminalAction : SCR_ScriptedUserAction
{
	[Attribute("", UIWidgets.EditBox, "Optional fire direction console entity name. Leave empty when this action is on the console itself", category: "LCN Links")]
	protected string m_sConsoleEntityName;

	[Attribute("0", UIWidgets.CheckBox, "Print client-side terminal debug to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	[Attribute("FIRE DIRECTION TERMINAL", UIWidgets.EditBox, "Terminal title text", category: "LCN Terminal Labels")]
	protected string m_sTitleLabel;

	[Attribute("Grid X", UIWidgets.EditBox, "X coordinate field label", category: "LCN Terminal Labels")]
	protected string m_sCoordXLabel;

	[Attribute("Grid Z", UIWidgets.EditBox, "Z coordinate field label", category: "LCN Terminal Labels")]
	protected string m_sCoordZLabel;

	[Attribute("SET TARGET", UIWidgets.EditBox, "Button text: set target from typed coordinates", category: "LCN Terminal Buttons")]
	protected string m_sSetTargetLabel;

	[Attribute("LOAD MARKER", UIWidgets.EditBox, "Button text: load configured marker target", category: "LCN Terminal Buttons")]
	protected string m_sLoadMarkerLabel;

	[Attribute("SPOTTING ROUND", UIWidgets.EditBox, "Button text: request spotting round", category: "LCN Terminal Buttons")]
	protected string m_sSpottingLabel;

	[Attribute("FIRE FOR EFFECT", UIWidgets.EditBox, "Button text: request main fire mission", category: "LCN Terminal Buttons")]
	protected string m_sFireLabel;

	[Attribute("< LEFT", UIWidgets.EditBox, "Button text: move impact point left", category: "LCN Terminal Buttons")]
	protected string m_sLeftLabel;

	[Attribute("RIGHT >", UIWidgets.EditBox, "Button text: move impact point right", category: "LCN Terminal Buttons")]
	protected string m_sRightLabel;

	[Attribute("ADD +", UIWidgets.EditBox, "Button text: move impact point farther from observer", category: "LCN Terminal Buttons")]
	protected string m_sAddLabel;

	[Attribute("DROP -", UIWidgets.EditBox, "Button text: move impact point closer to observer", category: "LCN Terminal Buttons")]
	protected string m_sDropLabel;

	[Attribute("CLEAR", UIWidgets.EditBox, "Button text: clear current fire mission", category: "LCN Terminal Buttons")]
	protected string m_sClearLabel;

	[Attribute("CLOSE", UIWidgets.EditBox, "Button text: close terminal", category: "LCN Terminal Buttons")]
	protected string m_sCloseLabel;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		LCN_FireMissionConsoleComponent console = GetFireMissionConsole();
		if (!console)
			return;

		LCN_FireMissionTerminalWidgetComponent.OpenTerminal(console, m_sConsoleEntityName, this);
	}

	//------------------------------------------------------------------------------------------------
	string GetTerminalLabel(string labelKey)
	{
		if (labelKey == "title")
			return m_sTitleLabel;

		if (labelKey == "coord_x")
			return m_sCoordXLabel;

		if (labelKey == "coord_z")
			return m_sCoordZLabel;

		if (labelKey == "set_target")
			return m_sSetTargetLabel;

		if (labelKey == "load_marker")
			return m_sLoadMarkerLabel;

		if (labelKey == "spotting")
			return m_sSpottingLabel;

		if (labelKey == "fire")
			return m_sFireLabel;

		if (labelKey == "left")
			return m_sLeftLabel;

		if (labelKey == "right")
			return m_sRightLabel;

		if (labelKey == "add")
			return m_sAddLabel;

		if (labelKey == "drop")
			return m_sDropLabel;

		if (labelKey == "clear")
			return m_sClearLabel;

		if (labelKey == "close")
			return m_sCloseLabel;

		return "";
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
