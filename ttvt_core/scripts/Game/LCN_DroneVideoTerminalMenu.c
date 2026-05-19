//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "LCN/Drone Video", description: "Registers an entity as a camera source for LCN drone video monitors")]
class LCN_DroneVideoSourceComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
class LCN_DroneVideoSourceComponent : ScriptComponent
{
	protected static ref array<LCN_DroneVideoSourceComponent> s_aSources;

	[Attribute("", UIWidgets.EditBox, "Optional source key. Monitors with the same key can use this source", category: "LCN Drone Feed")]
	protected string m_sSourceKey;

	[Attribute("", UIWidgets.EditBox, "Status label shown on monitor terminals", category: "LCN Drone Feed")]
	protected string m_sDisplayName;

	[Attribute("1", UIWidgets.CheckBox, "Source can be used by drone video monitors", category: "LCN Drone Feed")]
	protected bool m_bEnabled;

	[Attribute("0 0.12 0.25", UIWidgets.EditBox, "Camera offset in source local space: right up forward", category: "LCN Drone Feed")]
	protected vector m_vCameraOffset;

	[Attribute("55", UIWidgets.EditBox, "Vertical FOV used when this source drives a monitor camera", params: "5 140 1", category: "LCN Drone Feed")]
	protected float m_fVerticalFOV;

	[Attribute("0", UIWidgets.EditBox, "Default camera pan/yaw in degrees relative to the source entity", params: "-180 180 1", category: "LCN Drone Feed")]
	protected float m_fPanDegrees;

	[Attribute("0", UIWidgets.EditBox, "Default camera tilt/pitch in degrees relative to the source entity", params: "-90 90 1", category: "LCN Drone Feed")]
	protected float m_fTiltDegrees;

	[Attribute("0", UIWidgets.EditBox, "Default camera roll in degrees relative to the source entity", params: "-180 180 1", category: "LCN Drone Feed")]
	protected float m_fRollDegrees;

	[Attribute("0", UIWidgets.CheckBox, "Print source registration to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected IEntity m_Owner;
	protected bool m_bHasBroadcast;
	protected vector m_aBroadcastMat[4];
	protected float m_fBroadcastVerticalFOV;
	protected float m_fBroadcastWorldTime;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Owner = owner;

		array<LCN_DroneVideoSourceComponent> sources = GetSourceRegistry();
		if (sources.Find(this) == -1)
			sources.Insert(this);

		if (m_bDebug)
			Print(string.Format("LCN_DroneVideoSourceComponent: registered %1", GetStatusLabel()));
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (s_aSources)
			s_aSources.RemoveItem(this);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	static LCN_DroneVideoSourceComponent FindNearestSource(string sourceKey, vector origin, float scanRadius, BaseWorld world = null, IEntity ignoredOwner = null, IEntity ignoredUser = null)
	{
		array<LCN_DroneVideoSourceComponent> sources = GetSourceRegistry();
		LCN_DroneVideoSourceComponent bestSource;
		float bestDistanceSq = 999999999.0;
		float radiusSq = Math.Max(scanRadius, 1.0);
		radiusSq = radiusSq * radiusSq;

		foreach (LCN_DroneVideoSourceComponent source : sources)
		{
			if (!source || !source.CanUseSource(world))
				continue;

			IEntity owner = source.GetOwnerEntity();
			if (!owner || owner == ignoredOwner || owner == ignoredUser)
				continue;

			if (!sourceKey.IsEmpty() && !source.MatchesSourceKey(sourceKey))
				continue;

			vector delta = owner.GetOrigin() - origin;
			float distanceSq = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
			if (distanceSq > radiusSq || distanceSq >= bestDistanceSq)
				continue;

			bestSource = source;
			bestDistanceSq = distanceSq;
		}

		return bestSource;
	}

	//------------------------------------------------------------------------------------------------
	static LCN_DroneVideoSourceComponent FindSource(string sourceKey, BaseWorld world = null)
	{
		array<LCN_DroneVideoSourceComponent> sources = GetSourceRegistry();
		foreach (LCN_DroneVideoSourceComponent source : sources)
		{
			if (!source || !source.CanUseSource(world))
				continue;

			if (!sourceKey.IsEmpty() && !source.MatchesSourceKey(sourceKey))
				continue;

			return source;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	IEntity GetOwnerEntity()
	{
		return m_Owner;
	}

	//------------------------------------------------------------------------------------------------
	bool CanUseSource(BaseWorld world = null)
	{
		if (!m_bEnabled || !m_Owner)
			return false;

		if (world && m_Owner.GetWorld() != world)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool MatchesSourceKey(string sourceKey)
	{
		if (sourceKey.IsEmpty() || m_sSourceKey.IsEmpty())
			return false;

		string wanted = sourceKey.Trim();
		wanted.ToLower();

		string ownKey = m_sSourceKey.Trim();
		ownKey.ToLower();

		return wanted == ownKey;
	}

	//------------------------------------------------------------------------------------------------
	void RequestBroadcastCamera(vector cameraMat[4], float verticalFOV)
	{
		m_aBroadcastMat[0] = cameraMat[0];
		m_aBroadcastMat[1] = cameraMat[1];
		m_aBroadcastMat[2] = cameraMat[2];
		m_aBroadcastMat[3] = cameraMat[3];
		m_fBroadcastVerticalFOV = Math.Clamp(verticalFOV, 5.0, 140.0);
		m_fBroadcastWorldTime = GetWorldTime();
		m_bHasBroadcast = true;
	}

	//------------------------------------------------------------------------------------------------
	bool HasFreshBroadcast(BaseWorld world = null)
	{
		if (!m_bHasBroadcast)
			return false;

		float now = GetWorldTime(world);
		return now - m_fBroadcastWorldTime <= 3.0;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateCamera(BaseWorld world, int cameraIndex, float fallbackVerticalFOV, float panOffsetDegrees = 0, float tiltOffsetDegrees = 0)
	{
		if (!world || !m_Owner)
			return;

		if (HasFreshBroadcast(world))
		{
			world.SetCameraType(cameraIndex, CameraType.PERSPECTIVE);
			world.SetCameraEx(cameraIndex, m_aBroadcastMat);
			world.SetCameraVerticalFOV(cameraIndex, Math.Clamp(m_fBroadcastVerticalFOV, 5.0, 140.0));
			return;
		}

		vector baseMat[4];
		m_Owner.GetTransform(baseMat);

		vector localAngles = Vector(m_fPanDegrees + panOffsetDegrees, m_fTiltDegrees + tiltOffsetDegrees, m_fRollDegrees);
		vector localDirection = localAngles.AnglesToVector();
		vector worldDirection = baseMat[0] * localDirection[0] + baseMat[1] * localDirection[1] + baseMat[2] * localDirection[2];
		worldDirection.Normalize();

		vector mat[4];
		Math3D.DirectionAndUpMatrix(worldDirection, baseMat[1], mat);
		mat[3] = baseMat[3] + baseMat[0] * m_vCameraOffset[0] + baseMat[1] * m_vCameraOffset[1] + baseMat[2] * m_vCameraOffset[2];

		world.SetCameraType(cameraIndex, CameraType.PERSPECTIVE);
		world.SetCameraEx(cameraIndex, mat);
		world.SetCameraVerticalFOV(cameraIndex, GetVerticalFOV(fallbackVerticalFOV));
	}

	//------------------------------------------------------------------------------------------------
	float GetVerticalFOV(float fallbackVerticalFOV)
	{
		if (m_fVerticalFOV > 0)
			return Math.Clamp(m_fVerticalFOV, 5.0, 140.0);

		return Math.Clamp(fallbackVerticalFOV, 5.0, 140.0);
	}

	//------------------------------------------------------------------------------------------------
	string GetStatusLabel()
	{
		string label = m_sDisplayName.Trim();
		if (!label.IsEmpty())
			return label;

		label = m_sSourceKey.Trim();
		if (!label.IsEmpty())
			return label;

		return LCN_DroneVideoTerminalHelpers.GetEntityLabel(m_Owner);
	}

	//------------------------------------------------------------------------------------------------
	protected static array<LCN_DroneVideoSourceComponent> GetSourceRegistry()
	{
		if (!s_aSources)
			s_aSources = new array<LCN_DroneVideoSourceComponent>();

		return s_aSources;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetWorldTime(BaseWorld world = null)
	{
		if (!world)
			world = GetGame().GetWorld();

		if (!world)
			return 0;

		return world.GetWorldTime();
	}
}

//------------------------------------------------------------------------------------------------
class LCN_DroneVideoTerminalHelpers
{
	//------------------------------------------------------------------------------------------------
	static string GetEntityLabel(IEntity entity)
	{
		if (!entity)
			return "none";

		string label = string.Format("%1", entity);
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (prefabData)
			label += string.Format(" %1", prefabData.GetPrefabName());

		return label;
	}

	//------------------------------------------------------------------------------------------------
	static bool EntityMatchesKeywords(IEntity entity, string keywords)
	{
		if (!entity || keywords.IsEmpty())
			return false;

		string haystack = GetEntityLabel(entity);
		haystack.ToLower();

		keywords.Replace(";", ",");
		array<string> tokens = new array<string>();
		keywords.Split(",", tokens, true);

		foreach (string token : tokens)
		{
			token = token.Trim();
			token.ToLower();
			if (token.IsEmpty())
				continue;

			if (haystack.IndexOf(token) >= 0)
				return true;
		}

		return false;
	}
}

//------------------------------------------------------------------------------------------------
class LCN_DroneVideoBroadcastAction : SCR_ScriptedUserAction
{
	protected static bool s_bBroadcasting;
	protected static string s_sSourceKey;
	protected static int s_iBroadcastIntervalMs = 150;
	protected static bool s_bDebug;

	[Attribute("DJI", UIWidgets.EditBox, "Drone video source key that receives this controller camera", category: "LCN Drone Feed Broadcast")]
	protected string m_sSourceKey;

	[Attribute("150", UIWidgets.EditBox, "Broadcast update interval in milliseconds", params: "50 1000 10", category: "LCN Drone Feed Broadcast")]
	protected int m_iBroadcastIntervalMs;

	[Attribute("0", UIWidgets.CheckBox, "Print broadcast state to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (s_bBroadcasting)
		{
			StopBroadcast();
			return;
		}

		StartBroadcastForSource(m_sSourceKey, m_iBroadcastIntervalMs, m_bDebug);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (s_bBroadcasting)
			outName = "Stop drone video broadcast";
		else
			outName = "Broadcast drone camera";

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionDescriptionScript(out string outName)
	{
		outName = "Transmit current camera to the video station";
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
	static void StartBroadcastForSource(string sourceKey, int intervalMs = 150, bool debugEnabled = false)
	{
		s_sSourceKey = sourceKey;
		s_iBroadcastIntervalMs = Math.Max(intervalMs, 50);
		s_bDebug = debugEnabled;
		s_bBroadcasting = true;

		GetGame().GetCallqueue().Remove(BroadcastTick);
		BroadcastTick();

		if (s_bDebug)
			Print(string.Format("LCN_DroneVideoBroadcastAction: started source='%1' interval=%2", s_sSourceKey, s_iBroadcastIntervalMs));
	}

	//------------------------------------------------------------------------------------------------
	protected static void StopBroadcast()
	{
		s_bBroadcasting = false;
		GetGame().GetCallqueue().Remove(BroadcastTick);

		if (s_bDebug)
			Print("LCN_DroneVideoBroadcastAction: stopped");
	}

	//------------------------------------------------------------------------------------------------
	protected static void BroadcastTick()
	{
		if (!s_bBroadcasting)
			return;

		BaseWorld world = GetGame().GetWorld();
		LCN_DroneVideoSourceComponent source = LCN_DroneVideoSourceComponent.FindSource(s_sSourceKey, world);
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();

		if (source && camera)
		{
			vector mat[4];
			camera.GetTransform(mat);
			source.RequestBroadcastCamera(mat, camera.GetVerticalFOV());
		}
		else if (s_bDebug)
		{
			Print(string.Format("LCN_DroneVideoBroadcastAction: skipped source=%1 camera=%2", source, camera), LogLevel.WARNING);
		}

		GetGame().GetCallqueue().CallLater(BroadcastTick, s_iBroadcastIntervalMs, false);
	}
}

//------------------------------------------------------------------------------------------------
class LCN_DroneDirectConnectAction : SCR_ScriptedUserAction
{
	[Attribute("DJI", UIWidgets.EditBox, "Video relay source key to start", category: "LCN Drone Direct Connect")]
	protected string m_sSourceKey;

	[Attribute("", UIWidgets.EditBox, "Optional exact drone entity names, separated by comma or semicolon", category: "LCN Drone Direct Connect")]
	protected string m_sDroneEntityNames;

	[Attribute("dji81dropper,djisupplydrone,djibase,dji", UIWidgets.EditBox, "Keywords used to find the nearest drone", category: "LCN Drone Direct Connect")]
	protected string m_sDroneKeywords;

	[Attribute("battery,goggles,commandstation,controller,antenna,tripod,radio,station,monitor,item,headgear,placeholder,grenade,shell,mortar,ammo,projectile", UIWidgets.EditBox, "Keywords excluded from direct drone search", category: "LCN Drone Direct Connect")]
	protected string m_sExcludedKeywords;

	[Attribute("5000", UIWidgets.EditBox, "Direct drone search radius in meters", params: "1 20000 1", category: "LCN Drone Direct Connect")]
	protected float m_fScanRadius;

	[Attribute("1", UIWidgets.CheckBox, "Start/refresh video broadcast", category: "LCN Drone Direct Connect")]
	protected bool m_bStartBroadcast;

	[Attribute("150", UIWidgets.EditBox, "Broadcast update interval in milliseconds", params: "50 1000 10", category: "LCN Drone Direct Connect")]
	protected int m_iBroadcastIntervalMs;

	[Attribute("0", UIWidgets.CheckBox, "Print direct connect state to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_bStartBroadcast)
			LCN_DroneVideoBroadcastAction.StartBroadcastForSource(m_sSourceKey, m_iBroadcastIntervalMs, m_bDebug);

		if (m_bDebug)
			Print("LCN_DroneDirectConnectAction: station relay started. Open the monitor or use the native controller to pilot.");
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Start DJI station relay";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionDescriptionScript(out string outName)
	{
		outName = "Start station video relay";
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
}

//------------------------------------------------------------------------------------------------
class LCN_OpenDroneVideoTerminalAction : SCR_ScriptedUserAction
{
	[Attribute("", UIWidgets.EditBox, "Optional registered drone video source key", category: "LCN Drone Feed")]
	protected string m_sSourceKey;

	[Attribute("0", UIWidgets.CheckBox, "Prefer entities with LCN_DroneVideoSourceComponent before keyword searching", category: "LCN Drone Feed")]
	protected bool m_bUseRegisteredVideoSources;

	[Attribute("", UIWidgets.EditBox, "Optional exact drone/camera entity names, separated by comma or semicolon", category: "LCN Drone Feed")]
	protected string m_sSourceEntityNames;

	[Attribute("djibase,dji81dropper,djisupplydrone,fpvbase,quadcopter,uav,dji", UIWidgets.EditBox, "Fallback keywords used to find the nearest drone entity by name or prefab path", category: "LCN Drone Feed")]
	protected string m_sSourceKeywords;

	[Attribute("battery,goggles,commandstation,controller,antenna,tripod,radio,station,item,headgear,placeholder,grenade,shell,mortar,ammo,projectile", UIWidgets.EditBox, "Keywords excluded from automatic drone source search", category: "LCN Drone Feed")]
	protected string m_sExcludedKeywords;

	[Attribute("1", UIWidgets.CheckBox, "Allow keyword search if no registered source or exact entity name is found", category: "LCN Drone Feed")]
	protected bool m_bUseKeywordFallback;

	[Attribute("5000", UIWidgets.EditBox, "Fallback drone scan radius around the terminal/user, in metres", params: "10 20000 10", category: "LCN Drone Feed")]
	protected float m_fScanRadius;

	[Attribute("27", UIWidgets.EditBox, "World camera index used by this monitor render target", params: "1 128 1", category: "LCN Drone Feed")]
	protected int m_iCameraIndex;

	[Attribute("55", UIWidgets.EditBox, "Monitor camera vertical FOV", params: "5 140 1", category: "LCN Drone Feed")]
	protected float m_fVerticalFOV;

	[Attribute("0 0.12 0.25", UIWidgets.EditBox, "Camera offset in source local space: right up forward", category: "LCN Drone Feed")]
	protected vector m_vCameraOffset;

	[Attribute("1", UIWidgets.CheckBox, "Use current player camera if no drone source can be found", category: "LCN Drone Feed")]
	protected bool m_bFallbackToPlayerCamera;

	[Attribute("DRONE VIDEO POST", UIWidgets.EditBox, "Terminal title text", category: "LCN Drone Feed Labels")]
	protected string m_sTitleLabel;

	[Attribute("LIVE FEED", UIWidgets.EditBox, "Feed label text", category: "LCN Drone Feed Labels")]
	protected string m_sFeedLabel;

	[Attribute("RESCAN", UIWidgets.EditBox, "Button text: find drone source again", category: "LCN Drone Feed Labels")]
	protected string m_sRescanLabel;

	[Attribute("LOCAL VIEW", UIWidgets.EditBox, "Button text: force current player camera view", category: "LCN Drone Feed Labels")]
	protected string m_sPlayerViewLabel;

	[Attribute("DRONE VIEW", UIWidgets.EditBox, "Button text: return to drone source view", category: "LCN Drone Feed Labels")]
	protected string m_sDroneViewLabel;

	[Attribute("CLOSE", UIWidgets.EditBox, "Button text: close terminal", category: "LCN Drone Feed Labels")]
	protected string m_sCloseLabel;

	[Attribute("1", UIWidgets.CheckBox, "Print monitor debug to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		LCN_DroneVideoTerminalWidgetComponent.OpenTerminal(this, pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Open drone video post";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionDescriptionScript(out string outName)
	{
		outName = "Open live drone monitor";
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

	string GetSourceKey() { return m_sSourceKey; }
	bool UseRegisteredVideoSources() { return m_bUseRegisteredVideoSources; }
	string GetSourceEntityNames() { return m_sSourceEntityNames; }
	string GetSourceKeywords() { return m_sSourceKeywords; }
	string GetExcludedKeywords() { return m_sExcludedKeywords; }
	bool UseKeywordFallback() { return m_bUseKeywordFallback; }
	float GetScanRadius() { return Math.Max(m_fScanRadius, 1.0); }
	int GetCameraIndex() { return m_iCameraIndex; }
	float GetVerticalFOV() { return Math.Clamp(m_fVerticalFOV, 5.0, 140.0); }
	vector GetCameraOffset() { return m_vCameraOffset; }
	bool CanFallbackToPlayerCamera() { return m_bFallbackToPlayerCamera; }
	bool IsDebugEnabled() { return m_bDebug; }

	//------------------------------------------------------------------------------------------------
	string GetTerminalLabel(string labelKey)
	{
		if (labelKey == "title")
			return m_sTitleLabel;

		if (labelKey == "feed")
			return m_sFeedLabel;

		if (labelKey == "rescan")
			return m_sRescanLabel;

		if (labelKey == "player_view")
			return m_sPlayerViewLabel;

		if (labelKey == "drone_view")
			return m_sDroneViewLabel;

		if (labelKey == "close")
			return m_sCloseLabel;

		return "";
	}
}

//------------------------------------------------------------------------------------------------
class LCN_DroneVideoTerminalWidgetComponent : ScriptedWidgetComponent
{
	protected static const ResourceName TERMINAL_LAYOUT = "{2A8F63CCD996E800}UI/layouts/LCN_DroneVideoTerminal.layout";
	protected static const int INPUT_TICK_MS = 16;
	protected static const int FEED_TICK_MS = 16;
	protected static LCN_OpenDroneVideoTerminalAction s_PendingActionSettings;
	protected static IEntity s_PendingOwner;
	protected static IEntity s_PendingUser;
	protected static Widget s_ActiveRoot;

	[Attribute("InGameMenuContext", UIWidgets.EditBox, "Primary input context kept active while the monitor is open", category: "LCN Drone Feed Input")]
	protected string m_sPrimaryInputContext;

	[Attribute("InventoryContext", UIWidgets.EditBox, "Fallback input context also kept active while the monitor is open", category: "LCN Drone Feed Input")]
	protected string m_sFallbackInputContext;

	[Attribute("250", UIWidgets.EditBox, "How long monitor input contexts stay active after each refresh, in milliseconds", params: "16 2000 1", category: "LCN Drone Feed Input")]
	protected int m_iInputContextDurationMs;

	[Attribute("1", UIWidgets.CheckBox, "Force mouse as current UI input device while the monitor is open", category: "LCN Drone Feed Input")]
	protected bool m_bForceMouseInput;

	protected Widget m_Root;
	protected RenderTargetWidget m_FeedWidget;
	protected RichTextWidget m_TitleText;
	protected RichTextWidget m_FeedLabelText;
	protected RichTextWidget m_StatusText;
	protected SCR_InputButtonComponent m_RescanButton;
	protected SCR_InputButtonComponent m_PlayerViewButton;
	protected SCR_InputButtonComponent m_DroneViewButton;
	protected SCR_InputButtonComponent m_CloseButton;
	protected LCN_OpenDroneVideoTerminalAction m_ActionSettings;
	protected IEntity m_Owner;
	protected IEntity m_User;
	protected LCN_DroneVideoSourceComponent m_SourceComponent;
	protected IEntity m_Source;
	protected IEntity m_BestCandidate;
	protected vector m_vSearchCenter;
	protected float m_fBestCandidateDistanceSq;
	protected bool m_bUsePlayerCamera;
	protected bool m_bInputApplied;
	protected EInputDeviceType m_ePreviousInputDevice = EInputDeviceType.INVALID;

	//------------------------------------------------------------------------------------------------
	static void OpenTerminal(LCN_OpenDroneVideoTerminalAction actionSettings, IEntity owner, IEntity user)
	{
		CloseActiveTerminal();

		s_PendingActionSettings = actionSettings;
		s_PendingOwner = owner;
		s_PendingUser = user;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print("LCN_DroneVideoTerminalWidgetComponent: workspace missing", LogLevel.WARNING);
			return;
		}

		Widget root = workspace.CreateWidgets(TERMINAL_LAYOUT);
		if (!root)
		{
			Print(string.Format("LCN_DroneVideoTerminalWidgetComponent: failed to create layout '%1'", TERMINAL_LAYOUT), LogLevel.WARNING);
			return;
		}

		s_ActiveRoot = root;
		workspace.AddModal(root, null);
		workspace.SetFocusedWidget(root);

		if (!root.FindHandler(LCN_DroneVideoTerminalWidgetComponent))
			Print("LCN_DroneVideoTerminalWidgetComponent: layout created but terminal component is missing", LogLevel.WARNING);
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
		m_ActionSettings = s_PendingActionSettings;
		m_Owner = s_PendingOwner;
		m_User = s_PendingUser;

		BindWidgets();
		BindButtons();
		ApplyLabels();
		ApplyMonitorInput();
		RegisterMenuInput();
		ResolveSource(true);
		UpdateFeed();

		GetGame().GetCallqueue().CallLater(MaintainMonitorInput, INPUT_TICK_MS, true);
		GetGame().GetCallqueue().CallLater(UpdateFeed, FEED_TICK_MS, true);
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
		m_FeedWidget = RenderTargetWidget.Cast(m_Root.FindAnyWidget("DroneFeed"));
		if (m_FeedWidget)
		{
			m_FeedWidget.SetWorld(GetGame().GetWorld(), GetCameraIndex());
			m_FeedWidget.SetResolutionScale(1.0, 1.0);
		}

		m_TitleText = RichTextWidget.Cast(m_Root.FindAnyWidget("TitleText"));
		m_FeedLabelText = RichTextWidget.Cast(m_Root.FindAnyWidget("FeedLabelText"));
		m_StatusText = RichTextWidget.Cast(m_Root.FindAnyWidget("StatusText"));
	}

	//------------------------------------------------------------------------------------------------
	protected void BindButtons()
	{
		m_RescanButton = SCR_InputButtonComponent.GetInputButtonComponent("RescanFeed", m_Root);
		if (m_RescanButton)
			m_RescanButton.m_OnClicked.Insert(RescanSource);

		m_PlayerViewButton = SCR_InputButtonComponent.GetInputButtonComponent("PlayerView", m_Root);
		if (m_PlayerViewButton)
			m_PlayerViewButton.m_OnClicked.Insert(UsePlayerCamera);

		m_DroneViewButton = SCR_InputButtonComponent.GetInputButtonComponent("DroneView", m_Root);
		if (m_DroneViewButton)
			m_DroneViewButton.m_OnClicked.Insert(UseDroneCamera);

		m_CloseButton = SCR_InputButtonComponent.GetInputButtonComponent("CloseTerminal", m_Root);
		if (m_CloseButton)
			m_CloseButton.m_OnClicked.Insert(CloseTerminal);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyLabels()
	{
		SetRichText(m_TitleText, ResolveLabel("title", "DRONE VIDEO POST"));
		SetRichText(m_FeedLabelText, ResolveLabel("feed", "LIVE FEED"));
		SetButtonLabel(m_RescanButton, ResolveLabel("rescan", "RESCAN"));
		SetButtonLabel(m_PlayerViewButton, ResolveLabel("player_view", "LOCAL VIEW"));
		SetButtonLabel(m_DroneViewButton, ResolveLabel("drone_view", "DRONE VIEW"));
		SetButtonLabel(m_CloseButton, ResolveLabel("close", "CLOSE"));
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
	protected void SetRichText(RichTextWidget widget, string text)
	{
		if (widget && !text.IsEmpty())
			widget.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetButtonLabel(SCR_InputButtonComponent button, string label)
	{
		if (button && !label.IsEmpty())
			button.SetLabel(label);
	}

	//------------------------------------------------------------------------------------------------
	protected void ResolveSource(bool printDebug = false)
	{
		m_SourceComponent = null;
		m_Source = null;

		IEntity searchAnchor = m_Owner;
		if (!searchAnchor)
			searchAnchor = m_User;

		BaseWorld world = GetGame().GetWorld();
		if (!world || !searchAnchor || !m_ActionSettings)
		{
			SetStatus("source: missing world/action");
			return;
		}

		m_Source = FindConfiguredSourceEntity(world);
		if (m_Source)
		{
			SetStatus("source: " + LCN_DroneVideoTerminalHelpers.GetEntityLabel(m_Source));
			if (printDebug && m_ActionSettings.IsDebugEnabled())
				Print(string.Format("LCN_DroneVideoTerminalWidgetComponent: configured source %1", LCN_DroneVideoTerminalHelpers.GetEntityLabel(m_Source)));

			return;
		}

		if (m_ActionSettings.UseRegisteredVideoSources())
		{
			m_SourceComponent = LCN_DroneVideoSourceComponent.FindNearestSource(m_ActionSettings.GetSourceKey(), searchAnchor.GetOrigin(), m_ActionSettings.GetScanRadius(), world, m_Owner, m_User);
			if (m_SourceComponent)
			{
				SetStatus("source: " + m_SourceComponent.GetStatusLabel());
				if (printDebug && m_ActionSettings.IsDebugEnabled())
					Print(string.Format("LCN_DroneVideoTerminalWidgetComponent: registered source %1", m_SourceComponent.GetStatusLabel()));

				return;
			}
		}

		if (m_ActionSettings.UseKeywordFallback())
			m_Source = FindNearestKeywordSource(searchAnchor);

		if (m_Source)
		{
			SetStatus("source: " + LCN_DroneVideoTerminalHelpers.GetEntityLabel(m_Source));
			if (printDebug && m_ActionSettings.IsDebugEnabled())
				Print(string.Format("LCN_DroneVideoTerminalWidgetComponent: keyword source %1", LCN_DroneVideoTerminalHelpers.GetEntityLabel(m_Source)));

			return;
		}

		if (m_ActionSettings.CanFallbackToPlayerCamera())
		{
			m_bUsePlayerCamera = true;
			SetStatus("source: controller camera fallback");
		}
		else
		{
			SetStatus("source: none");
		}

		if (printDebug && m_ActionSettings.IsDebugEnabled())
			Print("LCN_DroneVideoTerminalWidgetComponent: no drone source found", LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity FindConfiguredSourceEntity(BaseWorld world)
	{
		string names = m_ActionSettings.GetSourceEntityNames();
		if (names.IsEmpty())
			return null;

		names.Replace(";", ",");
		array<string> tokens = new array<string>();
		names.Split(",", tokens, true);

		foreach (string token : tokens)
		{
			token = token.Trim();
			if (token.IsEmpty())
				continue;

			IEntity entity = world.FindEntityByName(token);
			if (entity)
				return entity;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity FindNearestKeywordSource(IEntity searchAnchor)
	{
		BaseWorld world = searchAnchor.GetWorld();
		if (!world)
			return null;

		m_BestCandidate = null;
		m_vSearchCenter = searchAnchor.GetOrigin();
		m_fBestCandidateDistanceSq = 999999999.0;

		world.QueryEntitiesBySphere(m_vSearchCenter, m_ActionSettings.GetScanRadius(), QuerySourceCandidate, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);
		return m_BestCandidate;
	}

	//------------------------------------------------------------------------------------------------
	protected bool QuerySourceCandidate(IEntity entity)
	{
		if (!entity)
			return true;

		if (entity == m_Owner || entity == m_User)
			return true;

		if (LCN_DroneVideoTerminalHelpers.EntityMatchesKeywords(entity, m_ActionSettings.GetExcludedKeywords()))
			return true;

		if (!LCN_DroneVideoTerminalHelpers.EntityMatchesKeywords(entity, m_ActionSettings.GetSourceKeywords()))
			return true;

		vector delta = entity.GetOrigin() - m_vSearchCenter;
		float distanceSq = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
		if (distanceSq < m_fBestCandidateDistanceSq)
		{
			m_BestCandidate = entity;
			m_fBestCandidateDistanceSq = distanceSq;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFeed()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world || !m_ActionSettings)
			return;

		if (m_FeedWidget)
			m_FeedWidget.SetWorld(world, GetCameraIndex());

		if (m_bUsePlayerCamera)
		{
			UpdatePlayerCamera(world);
			return;
		}

		if (m_SourceComponent)
		{
			m_SourceComponent.UpdateCamera(world, GetCameraIndex(), m_ActionSettings.GetVerticalFOV());
			return;
		}

		if (m_Source)
		{
			UpdateEntityCamera(world, m_Source);
			return;
		}

		ResolveSource(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateEntityCamera(BaseWorld world, IEntity source)
	{
		vector baseMat[4];
		source.GetTransform(baseMat);

		vector localAngles = Vector(0, 0, 0);
		vector localDirection = localAngles.AnglesToVector();
		vector worldDirection = baseMat[0] * localDirection[0] + baseMat[1] * localDirection[1] + baseMat[2] * localDirection[2];
		worldDirection.Normalize();

		vector mat[4];
		Math3D.DirectionAndUpMatrix(worldDirection, baseMat[1], mat);

		vector offset = m_ActionSettings.GetCameraOffset();
		mat[3] = baseMat[3] + baseMat[0] * offset[0] + baseMat[1] * offset[1] + baseMat[2] * offset[2];

		world.SetCameraType(GetCameraIndex(), CameraType.PERSPECTIVE);
		world.SetCameraEx(GetCameraIndex(), mat);
		world.SetCameraVerticalFOV(GetCameraIndex(), m_ActionSettings.GetVerticalFOV());
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayerCamera(BaseWorld world)
	{
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (!camera)
			return;

		vector mat[4];
		camera.GetTransform(mat);

		world.SetCameraType(GetCameraIndex(), CameraType.PERSPECTIVE);
		world.SetCameraEx(GetCameraIndex(), mat);
		world.SetCameraVerticalFOV(GetCameraIndex(), camera.GetVerticalFOV());
	}

	//------------------------------------------------------------------------------------------------
	protected int GetCameraIndex()
	{
		if (!m_ActionSettings)
			return 27;

		return m_ActionSettings.GetCameraIndex();
	}

	//------------------------------------------------------------------------------------------------
	protected void RescanSource()
	{
		m_bUsePlayerCamera = false;
		ResolveSource(true);
		UpdateFeed();
	}

	//------------------------------------------------------------------------------------------------
	protected void UsePlayerCamera()
	{
		m_bUsePlayerCamera = true;
		m_SourceComponent = null;
		m_Source = null;
		SetStatus("source: controller camera");
		UpdateFeed();
	}

	//------------------------------------------------------------------------------------------------
	protected void UseDroneCamera()
	{
		m_bUsePlayerCamera = false;
		ResolveSource(true);
		UpdateFeed();
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseTerminal()
	{
		CloseActiveTerminal();
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string text)
	{
		if (m_StatusText)
			m_StatusText.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyMonitorInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		if (!m_bInputApplied)
		{
			m_ePreviousInputDevice = inputManager.GetLastUsedInputDevice();
			m_bInputApplied = true;
		}

		MaintainMonitorInput();
	}

	//------------------------------------------------------------------------------------------------
	protected void MaintainMonitorInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		if (m_bForceMouseInput)
			inputManager.SetLastUsedInputDevice(EInputDeviceType.MOUSE);

		ActivateMonitorContext(inputManager, m_sPrimaryInputContext);
		ActivateMonitorContext(inputManager, m_sFallbackInputContext);
	}

	//------------------------------------------------------------------------------------------------
	protected void ActivateMonitorContext(InputManager inputManager, string contextName)
	{
		if (!inputManager || contextName.IsEmpty())
			return;

		inputManager.ActivateContext(contextName, m_iInputContextDurationMs);
	}

	//------------------------------------------------------------------------------------------------
	protected void RestoreMonitorInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		ResetMonitorContext(inputManager, m_sPrimaryInputContext);
		ResetMonitorContext(inputManager, m_sFallbackInputContext);

		if (m_bInputApplied && m_ePreviousInputDevice != EInputDeviceType.INVALID)
			inputManager.SetLastUsedInputDevice(m_ePreviousInputDevice);

		m_bInputApplied = false;
		m_ePreviousInputDevice = EInputDeviceType.INVALID;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetMonitorContext(InputManager inputManager, string contextName)
	{
		if (!inputManager || contextName.IsEmpty())
			return;

		inputManager.ResetContext(contextName);
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
	protected void Cleanup()
	{
		RestoreMonitorInput();
		GetGame().GetCallqueue().Remove(MaintainMonitorInput);
		GetGame().GetCallqueue().Remove(UpdateFeed);
		UnbindButtons();
		UnregisterMenuInput();
	}

	//------------------------------------------------------------------------------------------------
	protected void UnbindButtons()
	{
		if (m_RescanButton)
			m_RescanButton.m_OnClicked.Remove(RescanSource);

		if (m_PlayerViewButton)
			m_PlayerViewButton.m_OnClicked.Remove(UsePlayerCamera);

		if (m_DroneViewButton)
			m_DroneViewButton.m_OnClicked.Remove(UseDroneCamera);

		if (m_CloseButton)
			m_CloseButton.m_OnClicked.Remove(CloseTerminal);
	}
}
