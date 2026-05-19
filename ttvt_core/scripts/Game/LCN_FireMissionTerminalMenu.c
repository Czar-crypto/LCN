class LCN_FireMissionTerminalWidgetComponent : ScriptedWidgetComponent
{
	protected static const ResourceName TERMINAL_LAYOUT = "{00C9E596F1F790A4}UI/layouts/LCN_FireMissionTerminal.layout";
	protected static const int INPUT_TICK_MS = 16;
	protected static const int STATUS_TICK_MS = 250;
	protected static LCN_FireMissionConsoleComponent s_PendingConsole;
	protected static string s_sPendingConsoleName;
	protected static LCN_OpenFireMissionTerminalAction s_PendingActionSettings;
	protected static Widget s_ActiveRoot;

	[Attribute("MenuContext", UIWidgets.EditBox, "Input context kept active while the terminal is open. Leave empty to disable context forcing", category: "LCN Terminal Input")]
	protected string m_sInputContext;

	[Attribute("1", UIWidgets.CheckBox, "Force mouse as current UI input device while the terminal is open", category: "LCN Terminal Input")]
	protected bool m_bForceMouseInput;

	[Attribute("1", UIWidgets.CheckBox, "Ask InputManager to show the cursor while the terminal is open", category: "LCN Terminal Input")]
	protected bool m_bShowCursor;

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

	protected Widget m_Root;
	protected LCN_FireMissionConsoleComponent m_Console;
	protected LCN_OpenFireMissionTerminalAction m_ActionSettings;
	protected SCR_EditBoxComponent m_CoordX;
	protected SCR_EditBoxComponent m_CoordZ;
	protected RichTextWidget m_TitleText;
	protected RichTextWidget m_StatusText;
	protected EInputDeviceType m_ePreviousInputDevice = EInputDeviceType.INVALID;
	protected bool m_bTerminalInputApplied;

	protected SCR_InputButtonComponent m_SetTargetButton;
	protected SCR_InputButtonComponent m_LoadMarkerButton;
	protected SCR_InputButtonComponent m_SpottingButton;
	protected SCR_InputButtonComponent m_FireButton;
	protected SCR_InputButtonComponent m_LeftButton;
	protected SCR_InputButtonComponent m_RightButton;
	protected SCR_InputButtonComponent m_AddButton;
	protected SCR_InputButtonComponent m_DropButton;
	protected SCR_InputButtonComponent m_ClearButton;
	protected SCR_InputButtonComponent m_CloseButton;

	//------------------------------------------------------------------------------------------------
	static void OpenTerminal(LCN_FireMissionConsoleComponent console, string consoleEntityName = "", LCN_OpenFireMissionTerminalAction actionSettings = null)
	{
		CloseActiveTerminal();

		s_PendingConsole = console;
		s_sPendingConsoleName = consoleEntityName;
		s_PendingActionSettings = actionSettings;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print("LCN_FireMissionTerminalWidgetComponent: workspace missing");
			return;
		}

		Widget root = workspace.CreateWidgets(TERMINAL_LAYOUT);
		if (!root)
		{
			Print(string.Format("LCN_FireMissionTerminalWidgetComponent: failed to create layout '%1'", TERMINAL_LAYOUT));
			return;
		}

		s_ActiveRoot = root;
		workspace.AddModal(root, null);

		if (!root.FindHandler(LCN_FireMissionTerminalWidgetComponent))
			Print("LCN_FireMissionTerminalWidgetComponent: layout created but terminal component is missing");
	}

	//------------------------------------------------------------------------------------------------
	static void CloseActiveTerminal()
	{
		if (s_ActiveRoot)
		{
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (workspace)
				workspace.RemoveModal(s_ActiveRoot);

			s_ActiveRoot.RemoveFromHierarchy();
			s_ActiveRoot = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_Root = w;
		s_ActiveRoot = w;
		m_Console = s_PendingConsole;
		m_ActionSettings = s_PendingActionSettings;

		if (!m_Console)
			m_Console = LCN_FireMissionConsoleComponent.FindConsole(s_sPendingConsoleName, GetGame().GetWorld());

		BindWidgets();
		BindButtons();
		ApplyTerminalLabels();
		PrefillTargetFields();
		ApplyTerminalInput();
		RegisterMenuInput();
		UpdateStatus();

		GetGame().GetCallqueue().CallLater(MaintainTerminalInput, INPUT_TICK_MS, true);
		GetGame().GetCallqueue().CallLater(UpdateStatus, STATUS_TICK_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		Cleanup();

		if (s_ActiveRoot == w)
			s_ActiveRoot = null;

		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	protected void BindWidgets()
	{
		m_CoordX = SCR_EditBoxComponent.GetEditBoxComponent("CoordX", m_Root);
		m_CoordZ = SCR_EditBoxComponent.GetEditBoxComponent("CoordZ", m_Root);
		m_TitleText = RichTextWidget.Cast(m_Root.FindAnyWidget("TitleText"));
		m_StatusText = RichTextWidget.Cast(m_Root.FindAnyWidget("StatusText"));
	}

	//------------------------------------------------------------------------------------------------
	protected void BindButtons()
	{
		m_SetTargetButton = SCR_InputButtonComponent.GetInputButtonComponent("SetTarget", m_Root);
		if (m_SetTargetButton)
			m_SetTargetButton.m_OnClicked.Insert(SetTargetFromCoordinates);

		m_LoadMarkerButton = SCR_InputButtonComponent.GetInputButtonComponent("LoadMarker", m_Root);
		if (m_LoadMarkerButton)
			m_LoadMarkerButton.m_OnClicked.Insert(LoadMarkerTarget);

		m_SpottingButton = SCR_InputButtonComponent.GetInputButtonComponent("Spotting", m_Root);
		if (m_SpottingButton)
			m_SpottingButton.m_OnClicked.Insert(RequestSpottingRound);

		m_FireButton = SCR_InputButtonComponent.GetInputButtonComponent("FireForEffect", m_Root);
		if (m_FireButton)
			m_FireButton.m_OnClicked.Insert(RequestFireForEffect);

		m_LeftButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectLeft", m_Root);
		if (m_LeftButton)
			m_LeftButton.m_OnClicked.Insert(CorrectLeft);

		m_RightButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectRight", m_Root);
		if (m_RightButton)
			m_RightButton.m_OnClicked.Insert(CorrectRight);

		m_AddButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectAdd", m_Root);
		if (m_AddButton)
			m_AddButton.m_OnClicked.Insert(CorrectAdd);

		m_DropButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectDrop", m_Root);
		if (m_DropButton)
			m_DropButton.m_OnClicked.Insert(CorrectDrop);

		m_ClearButton = SCR_InputButtonComponent.GetInputButtonComponent("ClearMission", m_Root);
		if (m_ClearButton)
			m_ClearButton.m_OnClicked.Insert(ClearMission);

		m_CloseButton = SCR_InputButtonComponent.GetInputButtonComponent("CloseTerminal", m_Root);
		if (m_CloseButton)
			m_CloseButton.m_OnClicked.Insert(CloseTerminal);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyTerminalLabels()
	{
		string titleLabel = ResolveLabel("title", m_sTitleLabel);
		if (m_TitleText && !titleLabel.IsEmpty())
			m_TitleText.SetText(titleLabel);

		string coordXLabel = ResolveLabel("coord_x", m_sCoordXLabel);
		if (m_CoordX && !coordXLabel.IsEmpty())
			m_CoordX.SetLabel(coordXLabel);

		string coordZLabel = ResolveLabel("coord_z", m_sCoordZLabel);
		if (m_CoordZ && !coordZLabel.IsEmpty())
			m_CoordZ.SetLabel(coordZLabel);

		SetButtonLabel(m_SetTargetButton, ResolveLabel("set_target", m_sSetTargetLabel));
		SetButtonLabel(m_LoadMarkerButton, ResolveLabel("load_marker", m_sLoadMarkerLabel));
		SetButtonLabel(m_SpottingButton, ResolveLabel("spotting", m_sSpottingLabel));
		SetButtonLabel(m_FireButton, ResolveLabel("fire", m_sFireLabel));
		SetButtonLabel(m_LeftButton, ResolveLabel("left", m_sLeftLabel));
		SetButtonLabel(m_RightButton, ResolveLabel("right", m_sRightLabel));
		SetButtonLabel(m_AddButton, ResolveLabel("add", m_sAddLabel));
		SetButtonLabel(m_DropButton, ResolveLabel("drop", m_sDropLabel));
		SetButtonLabel(m_ClearButton, ResolveLabel("clear", m_sClearLabel));
		SetButtonLabel(m_CloseButton, ResolveLabel("close", m_sCloseLabel));
	}

	//------------------------------------------------------------------------------------------------
	protected string ResolveLabel(string labelKey, string fallback)
	{
		if (m_ActionSettings)
		{
			string actionLabel = m_ActionSettings.GetTerminalLabel(labelKey);
			if (!actionLabel.IsEmpty())
				return actionLabel;
		}

		return fallback;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetButtonLabel(SCR_InputButtonComponent button, string label)
	{
		if (!button || label.IsEmpty())
			return;

		button.SetLabel(label);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyTerminalInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		if (!m_bTerminalInputApplied)
		{
			m_ePreviousInputDevice = inputManager.GetLastUsedInputDevice();
			m_bTerminalInputApplied = true;
		}

		if (m_bShowCursor)
			inputManager.SetLoading(true);

		MaintainTerminalInput();
	}

	//------------------------------------------------------------------------------------------------
	protected void MaintainTerminalInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		if (m_bForceMouseInput)
			inputManager.SetLastUsedInputDevice(EInputDeviceType.MOUSE);

		if (!m_sInputContext.IsEmpty())
			inputManager.ActivateContext(m_sInputContext, 2);
	}

	//------------------------------------------------------------------------------------------------
	protected void RestoreTerminalInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		if (m_bShowCursor)
			inputManager.SetLoading(false);

		if (!m_sInputContext.IsEmpty())
			inputManager.ResetContext(m_sInputContext);

		if (m_bTerminalInputApplied && m_ePreviousInputDevice != EInputDeviceType.INVALID)
			inputManager.SetLastUsedInputDevice(m_ePreviousInputDevice);

		m_bTerminalInputApplied = false;
		m_ePreviousInputDevice = EInputDeviceType.INVALID;
	}

	//------------------------------------------------------------------------------------------------
	protected void Cleanup()
	{
		RestoreTerminalInput();
		GetGame().GetCallqueue().Remove(MaintainTerminalInput);
		GetGame().GetCallqueue().Remove(UpdateStatus);
		UnbindButtons();
		UnregisterMenuInput();
	}

	//------------------------------------------------------------------------------------------------
	protected void UnbindButtons()
	{
		if (m_SetTargetButton)
			m_SetTargetButton.m_OnClicked.Remove(SetTargetFromCoordinates);

		if (m_LoadMarkerButton)
			m_LoadMarkerButton.m_OnClicked.Remove(LoadMarkerTarget);

		if (m_SpottingButton)
			m_SpottingButton.m_OnClicked.Remove(RequestSpottingRound);

		if (m_FireButton)
			m_FireButton.m_OnClicked.Remove(RequestFireForEffect);

		if (m_LeftButton)
			m_LeftButton.m_OnClicked.Remove(CorrectLeft);

		if (m_RightButton)
			m_RightButton.m_OnClicked.Remove(CorrectRight);

		if (m_AddButton)
			m_AddButton.m_OnClicked.Remove(CorrectAdd);

		if (m_DropButton)
			m_DropButton.m_OnClicked.Remove(CorrectDrop);

		if (m_ClearButton)
			m_ClearButton.m_OnClicked.Remove(ClearMission);

		if (m_CloseButton)
			m_CloseButton.m_OnClicked.Remove(CloseTerminal);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetTargetFromCoordinates()
	{
		if (!m_Console || !m_CoordX || !m_CoordZ)
			return;

		string xText = m_CoordX.GetValue().Trim();
		string zText = m_CoordZ.GetValue().Trim();

		if (xText.IsEmpty() || zText.IsEmpty())
		{
			SetStatusLine("FDC: enter X and Z coordinates");
			return;
		}

		m_Console.RequestTargetCoordinates(xText.ToFloat(), zText.ToFloat(), GetLocalPlayerId(), GetLocalControlledEntity());
		UpdateStatus();
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadMarkerTarget()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_MARKER);
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestSpottingRound()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_SPOTTING_ROUND);
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestFireForEffect()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_FIRE_FOR_EFFECT);
	}

	//------------------------------------------------------------------------------------------------
	protected void CorrectLeft()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_CORRECT_LEFT);
	}

	//------------------------------------------------------------------------------------------------
	protected void CorrectRight()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_CORRECT_RIGHT);
	}

	//------------------------------------------------------------------------------------------------
	protected void CorrectAdd()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_CORRECT_ADD);
	}

	//------------------------------------------------------------------------------------------------
	protected void CorrectDrop()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_CORRECT_DROP);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearMission()
	{
		RequestConsoleAction(LCN_FireMissionConsoleComponent.ACTION_CLEAR_MISSION);
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseTerminal()
	{
		CloseActiveTerminal();
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestConsoleAction(int actionType)
	{
		if (!m_Console)
			return;

		m_Console.RequestAction(actionType, 0, vector.Zero, vector.Zero, GetLocalPlayerId(), GetLocalControlledEntity());
		UpdateStatus();
	}

	//------------------------------------------------------------------------------------------------
	protected void PrefillTargetFields()
	{
		if (!m_Console || !m_Console.HasMission())
			return;

		vector target = m_Console.GetAdjustedTargetPosition();
		if (m_CoordX)
			m_CoordX.SetValue(target[0].ToString(0, 0));

		if (m_CoordZ)
			m_CoordZ.SetValue(target[2].ToString(0, 0));
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateStatus()
	{
		if (!m_Console)
		{
			SetStatusLine("FDC: console missing");
			UpdateButtonStates(null);
			return;
		}

		IEntity user = GetLocalControlledEntity();
		string status = m_Console.GetStatusText();
		string reason;

		if (!m_Console.CanPerformAction(LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_COORDINATES, user, reason) && !reason.IsEmpty())
			status += " | locked: " + reason;

		SetStatusLine(status);
		UpdateButtonStates(user);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatusLine(string text)
	{
		if (m_StatusText)
			m_StatusText.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateButtonStates(IEntity user)
	{
		SetButtonState(m_SetTargetButton, LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_COORDINATES, user);
		SetButtonState(m_LoadMarkerButton, LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_MARKER, user);
		SetButtonState(m_SpottingButton, LCN_FireMissionConsoleComponent.ACTION_SPOTTING_ROUND, user);
		SetButtonState(m_FireButton, LCN_FireMissionConsoleComponent.ACTION_FIRE_FOR_EFFECT, user);
		SetButtonState(m_LeftButton, LCN_FireMissionConsoleComponent.ACTION_CORRECT_LEFT, user);
		SetButtonState(m_RightButton, LCN_FireMissionConsoleComponent.ACTION_CORRECT_RIGHT, user);
		SetButtonState(m_AddButton, LCN_FireMissionConsoleComponent.ACTION_CORRECT_ADD, user);
		SetButtonState(m_DropButton, LCN_FireMissionConsoleComponent.ACTION_CORRECT_DROP, user);
		SetButtonState(m_ClearButton, LCN_FireMissionConsoleComponent.ACTION_CLEAR_MISSION, user);

		if (m_CloseButton)
			m_CloseButton.SetEnabled(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetButtonState(SCR_InputButtonComponent button, int actionType, IEntity user)
	{
		if (!button)
			return;

		string reason;
		button.SetEnabled(m_Console && m_Console.CanPerformAction(actionType, user, reason));
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterMenuInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, CloseTerminal);
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, CloseTerminal);
		#ifdef WORKBENCH
		inputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, CloseTerminal);
		inputManager.AddActionListener("MenuBackWB", EActionTrigger.DOWN, CloseTerminal);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	protected void UnregisterMenuInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, CloseTerminal);
		inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, CloseTerminal);
		#ifdef WORKBENCH
		inputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, CloseTerminal);
		inputManager.RemoveActionListener("MenuBackWB", EActionTrigger.DOWN, CloseTerminal);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	protected int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity GetLocalControlledEntity()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return null;

		return controller.GetControlledEntity();
	}
}
