class LCN_FireMissionAction : SCR_ScriptedUserAction
{
	[Attribute("0", UIWidgets.ComboBox, "Fire mission action type", "", enums: {
		ParamEnum("Set target from current view", "0"),
		ParamEnum("Set target from marker", "1"),
		ParamEnum("Spotting round", "2"),
		ParamEnum("Fire for effect", "3"),
		ParamEnum("Correct left", "4"),
		ParamEnum("Correct right", "5"),
		ParamEnum("Correct add", "6"),
		ParamEnum("Correct drop", "7"),
		ParamEnum("Clear mission", "8")
	}, category: "LCN Fire Mission")]
	protected int m_iActionType;

	[Attribute("", UIWidgets.EditBox, "Optional action name override. Empty uses a generated name", category: "LCN Fire Mission")]
	protected string m_sActionName;

	[Attribute("", UIWidgets.EditBox, "Optional action description override. Empty shows current mission status", category: "LCN Fire Mission")]
	protected string m_sActionDescription;

	[Attribute("", UIWidgets.EditBox, "Optional fire direction console entity name. Leave empty when this action is on the console itself", category: "LCN Links")]
	protected string m_sConsoleEntityName;

	[Attribute("0", UIWidgets.EditBox, "Correction amount in metres. 0 uses the console default", params: "0 2000 1", category: "LCN Fire Mission")]
	protected float m_fCorrectionAmount;

	[Attribute("6000", UIWidgets.EditBox, "Maximum distance in metres for current-view target acquisition", params: "100 20000 100", category: "LCN Fire Mission")]
	protected float m_fMaxTraceDistance;

	[Attribute("0", UIWidgets.CheckBox, "Print client-side action debug to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		LCN_FireMissionConsoleComponent console = GetFireMissionConsole();
		if (!console)
			return;

		vector observedTarget = vector.Zero;
		vector observerPosition = vector.Zero;

		if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_VIEW)
		{
			if (!TraceCurrentViewTarget(pUserEntity, observedTarget, observerPosition))
			{
				if (m_bDebug)
					Print("LCN_FireMissionAction: no valid current-view target");

				return;
			}
		}

		console.RequestAction(m_iActionType, m_fCorrectionAmount, observedTarget, observerPosition, GetLocalPlayerId(), pUserEntity);
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
		if (!console.CanPerformAction(m_iActionType, user, reason))
		{
			SetCannotPerformReason(reason);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!m_sActionName.IsEmpty())
		{
			outName = m_sActionName;
			return true;
		}

		float amount = m_fCorrectionAmount;
		LCN_FireMissionConsoleComponent console = GetFireMissionConsole();

		if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_VIEW)
			outName = "Transmit observed grid";
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_SET_TARGET_FROM_MARKER)
			outName = "Load target marker";
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_SPOTTING_ROUND)
			outName = "Request spotting round";
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_FIRE_FOR_EFFECT)
			outName = "Fire for effect";
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_CORRECT_LEFT)
		{
			if (amount <= 0 && console)
				amount = console.GetDefaultLateralCorrection();

			outName = string.Format("Adjust left %1 m", amount.ToString(0, 0));
		}
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_CORRECT_RIGHT)
		{
			if (amount <= 0 && console)
				amount = console.GetDefaultLateralCorrection();

			outName = string.Format("Adjust right %1 m", amount.ToString(0, 0));
		}
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_CORRECT_ADD)
		{
			if (amount <= 0 && console)
				amount = console.GetDefaultRangeCorrection();

			outName = string.Format("Add %1 m", amount.ToString(0, 0));
		}
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_CORRECT_DROP)
		{
			if (amount <= 0 && console)
				amount = console.GetDefaultRangeCorrection();

			outName = string.Format("Drop %1 m", amount.ToString(0, 0));
		}
		else if (m_iActionType == LCN_FireMissionConsoleComponent.ACTION_CLEAR_MISSION)
			outName = "Clear fire mission";
		else
			outName = "Fire mission action";

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionDescriptionScript(out string outName)
	{
		if (!m_sActionDescription.IsEmpty())
		{
			outName = m_sActionDescription;
			return true;
		}

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

	//------------------------------------------------------------------------------------------------
	protected bool TraceCurrentViewTarget(IEntity user, out vector targetPosition, out vector observerPosition)
	{
		targetPosition = vector.Zero;
		observerPosition = vector.Zero;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		vector direction = vector.Zero;
		CameraManager cameraManager = GetGame().GetCameraManager();
		CameraBase camera;
		if (cameraManager)
			camera = cameraManager.CurrentCamera();
		if (camera)
		{
			vector cameraTransform[4];
			camera.GetTransform(cameraTransform);
			observerPosition = camera.GetOrigin();
			direction = cameraTransform[2];
			direction.Normalize();
		}
		else
		{
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (!workspace)
				return false;

			observerPosition = workspace.ProjScreenToWorld(workspace.DPIUnscale(workspace.GetWidth() * 0.5), workspace.DPIUnscale(workspace.GetHeight() * 0.5), direction, world, -1);
			direction.Normalize();
		}

		if (direction == vector.Zero)
			return false;

		float traceDistance = Math.Max(m_fMaxTraceDistance, 100);
		TraceParam trace = new TraceParam();
		trace.Start = observerPosition;
		trace.End = observerPosition + direction * traceDistance;
		trace.Flags = TraceFlags.WORLD | TraceFlags.OCEAN | TraceFlags.ENTS;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;

		array<IEntity> excludedEntities = {};
		if (user)
			excludedEntities.Insert(user);

		PlayerController localController = GetGame().GetPlayerController();
		if (localController)
		{
			IEntity controlledEntity = localController.GetControlledEntity();
			if (controlledEntity && controlledEntity != user)
				excludedEntities.Insert(controlledEntity);
		}

		trace.ExcludeArray = excludedEntities;

		float hit = world.TraceMove(trace, null);
		if (hit == 1)
			return false;

		targetPosition = trace.Start + (trace.End - trace.Start) * hit;
		return targetPosition != vector.Zero;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}
}
