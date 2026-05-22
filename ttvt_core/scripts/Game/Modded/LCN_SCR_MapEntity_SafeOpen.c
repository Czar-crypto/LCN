//------------------------------------------------------------------------------------------------
modded class SCR_MapEntity
{
	// Echo can open the map before the player camera manager is ready.
	// Keep vanilla OpenMap flow, but avoid null calls during that early init pass.
	override void OpenMap(MapConfiguration config)
	{
		if (!config)
			return;

		if (m_bIsOpen)
		{
			Print("SCR_MapEntity: Attempted opening a map while it is already open", LogLevel.WARNING);
			CloseMap();
		}

		if (config.MapEntityMode != m_eLastMapMode)
			m_bDoReload = true;

		m_eLastMapMode = config.MapEntityMode;
		m_ActiveMapCfg = config;
		m_Workspace = GetGame().GetWorkspace();
		m_wMapRoot = config.RootWidgetRef;

		SetMapWidget(config.RootWidgetRef.FindAnyWidget(SCR_MapConstants.MAP_WIDGET_NAME));

		PlayerController plc = GetGame().GetPlayerController();

		if (config.MapEntityMode == EMapEntityMode.FULLSCREEN)
		{
			ChimeraCharacter character;
			if (plc)
				character = ChimeraCharacter.Cast(plc.GetControlledEntity());

			if (character)
			{
				SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
				if (controller)
					controller.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
			}

			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (gameMode)
				gameMode.GetOnPlayerDeleted().Insert(OnPlayerDeleted);
		}

		InitLayers(config);
		SetFrame(Vector(0, 0, 0), Vector(0, 0, 0));

		m_bIsOpen = true;

		s_OnMapInit.Invoke(config);

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (plc && cameraManager && cameraManager.CurrentCamera() == plc.GetPlayerCamera())
			plc.SetCharacterCameraRenderActive(false);
	}
}
