class LCN_FireMissionTerminalMenu : MenuBase
{
	protected static LCN_FireMissionConsoleComponent s_PendingConsole;
	protected static string s_sPendingConsoleName;

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
	static void SetPendingConsole(LCN_FireMissionConsoleComponent console, string consoleEntityName = "")
	{
		s_PendingConsole = console;
		s_sPendingConsoleName = consoleEntityName;
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		Widget root = GetRootWidget();
		m_Console = s_PendingConsole;

		if (!m_Console)
			m_Console = LCN_FireMissionConsoleComponent.FindConsole(s_sPendingConsoleName, GetGame().GetWorld());

		m_CoordX = SCR_EditBoxComponent.GetEditBoxComponent("CoordX", root);
		m_CoordZ = SCR_EditBoxComponent.GetEditBoxComponent("CoordZ", root);
		m_StatusText = RichTextWidget.Cast(root.FindAnyWidget("StatusText"));

		m_SetTargetButton = SCR_InputButtonComponent.GetInputButtonComponent("SetTarget", root);
		if (m_SetTargetButton)
			m_SetTargetButton.m_OnClicked.Insert(SetTargetFromCoordinates);

		m_LoadMarkerButton = SCR_InputButtonComponent.GetInputButtonComponent("LoadMarker", root);
		if (m_LoadMarkerButton)
			m_LoadMarkerButton.m_OnClicked.Insert(LoadMarkerTarget);

		m_SpottingButton = SCR_InputButtonComponent.GetInputButtonComponent("Spotting", root);
		if (m_SpottingButton)
			m_SpottingButton.m_OnClicked.Insert(RequestSpottingRound);

		m_FireButton = SCR_InputButtonComponent.GetInputButtonComponent("FireForEffect", root);
		if (m_FireButton)
			m_FireButton.m_OnClicked.Insert(RequestFireForEffect);

		m_LeftButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectLeft", root);
		if (m_LeftButton)
			m_LeftButton.m_OnClicked.Insert(CorrectLeft);

		m_RightButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectRight", root);
		if (m_RightButton)
			m_RightButton.m_OnClicked.Insert(CorrectRight);

		m_AddButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectAdd", root);
		if (m_AddButton)
			m_AddButton.m_OnClicked.Insert(CorrectAdd);

		m_DropButton = SCR_InputButtonComponent.GetInputButtonComponent("CorrectDrop", root);
		if (m_DropButton)
			m_DropButton.m_OnClicked.Insert(CorrectDrop);

		m_ClearButton = SCR_InputButtonComponent.GetInputButtonComponent("ClearMission", root);
		if (m_ClearButton)
			m_ClearButton.m_OnClicked.Insert(ClearMission);

		m_CloseButton = SCR_InputButtonComponent.GetInputButtonComponent("CloseTerminal", root);
		if (m_CloseButton)
			m_CloseButton.m_OnClicked.Insert(CloseTerminal);

		PrefillTargetFields();
		RegisterMenuInput();
		UpdateStatus();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
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

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, Close);
			inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Close);
			#ifdef WORKBENCH
			inputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, Close);
			inputManager.RemoveActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
			#endif
		}

		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		UpdateStatus();
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
		Close();
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

		inputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, Close);
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
		inputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, Close);
		inputManager.AddActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
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
