class LCN_FireMissionTerminalWidgetComponent : ScriptedWidgetComponent
{
	protected static const ResourceName TERMINAL_LAYOUT = "{00C9E596F1F790A4}UI/layouts/LCN_FireMissionTerminal.layout";
	protected static LCN_FireMissionConsoleComponent s_PendingConsole;
	protected static string s_sPendingConsoleName;
	protected static Widget s_ActiveRoot;

	protected Widget m_Root;
	protected LCN_FireMissionConsoleComponent m_Console;
	protected SCR_EditBoxComponent m_CoordX;
	protected SCR_EditBoxComponent m_CoordZ;
	protected RichTextWidget m_StatusText;

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
	static void OpenTerminal(LCN_FireMissionConsoleComponent console, string consoleEntityName = "")
	{
		CloseActiveTerminal();

		s_PendingConsole = console;
		s_sPendingConsoleName = consoleEntityName;

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

		if (!m_Console)
			m_Console = LCN_FireMissionConsoleComponent.FindConsole(s_sPendingConsoleName, GetGame().GetWorld());

		BindWidgets();
		BindButtons();
		PrefillTargetFields();
		RegisterMenuInput();
		UpdateStatus();

		GetGame().GetCallqueue().CallLater(UpdateStatus, 250, true);
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
	protected void Cleanup()
	{
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
