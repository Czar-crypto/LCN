class LCN_InteractableSupportAction : SCR_ScriptedUserAction
{
	[Attribute("Call support", UIWidgets.EditBox, "Action name shown to players", category: "LCN Support")]
	protected string m_sActionName;

	[Attribute("{5D48E2F7DB0C3714}PrefabsEditable/EffectsModules/Mortar/EffectModule_Zoned_MortarBarrage_Small.et", UIWidgets.ResourcePickerThumbnail, "Built-in GM/support effect module spawned when the action is completed", "et", category: "LCN Support")]
	protected ResourceName m_sEffectModulePrefab;

	[Attribute("10", UIWidgets.EditBox, "Delay in seconds before the effect module is spawned", params: "0 1800 1", category: "LCN Support")]
	protected float m_fEffectDelay;

	[Attribute("1", UIWidgets.CheckBox, "Disable this action forever after successful use", category: "LCN Support")]
	protected bool m_bOneUse;

	[Attribute("0", UIWidgets.EditBox, "Cooldown in seconds after use. Ignored when One Use is enabled", params: "0 3600 1", category: "LCN Support")]
	protected float m_fCooldown;

	[Attribute("", UIWidgets.EditBox, "Only this faction can use the action. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking the allowed faction key", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("", UIWidgets.EditBox, "Optional entity name used as the effect spawn position. Leave empty to spawn at the console", category: "LCN Links")]
	protected string m_sEffectTargetEntityName;

	[Attribute("0", UIWidgets.CheckBox, "If enabled, the action will fail when Effect Target Entity Name is empty or not found", category: "LCN Links")]
	protected bool m_bRequireEffectTargetEntity;

	[Attribute("", UIWidgets.EditBox, "Optional entity name that must be alive for this action to work, for example a generator", category: "LCN Links")]
	protected string m_sRequiredAliveEntityName;

	[Attribute("", UIWidgets.EditBox, "Optional entity name that blocks this action while alive, for example a jammer", category: "LCN Links")]
	protected string m_sBlockingAliveEntityName;

	[Attribute("1", UIWidgets.CheckBox, "Print action state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected bool m_bPending;
	protected bool m_bUsed;
	protected float m_fNextUseTime;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!IsMaster())
			return;

		if (!CanBePerformedScript(pUserEntity))
			return;

		m_bPending = true;
		m_bUsed = true;

		int reenableDelay = GetReenableDelayMs();
		if (m_bOneUse || reenableDelay > 0)
			SetActionEnabled_S(false);

		if (!m_bOneUse && m_fCooldown > 0)
			m_fNextUseTime = GetWorldTime() + Math.Round(m_fCooldown * 1000);

		if (!m_bOneUse && reenableDelay > 0)
			GetGame().GetCallqueue().CallLater(ReenableAction, reenableDelay, false);

		if (m_fEffectDelay <= 0)
		{
			SpawnEffectModule(pOwnerEntity);
			return;
		}

		GetGame().GetCallqueue().CallLater(SpawnEffectModule, Math.Round(m_fEffectDelay * 1000), false, pOwnerEntity);

		if (m_bDebug)
			Print(string.Format("LCN_InteractableSupportAction: '%1' queued by %2, effect delay=%3, cooldown=%4", m_sActionName, pUserEntity, m_fEffectDelay, m_fCooldown));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		IEntity owner = GetOwner();
		if (!owner || !IsEntityAlive(owner))
			return false;

		if (m_bOneUse && m_bUsed)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_bOneUse && m_bUsed)
		{
			SetCannotPerformReason("Already used");
			return false;
		}

		if (m_bPending)
		{
			SetCannotPerformReason("Support already queued");
			return false;
		}

		IEntity owner = GetOwner();
		if (!owner || !IsEntityAlive(owner))
		{
			SetCannotPerformReason("Station destroyed");
			return false;
		}

		if (!IsUserFactionAllowed(user))
		{
			SetCannotPerformReason("Wrong faction");
			return false;
		}

		if (!IsEffectTargetReady())
		{
			SetCannotPerformReason("Effect target missing");
			return false;
		}

		if (!IsRequiredEntityReady())
		{
			SetCannotPerformReason("Required system offline");
			return false;
		}

		if (IsBlockingEntityActive())
		{
			SetCannotPerformReason("Blocked by active system");
			return false;
		}

		if (!m_bOneUse && m_fCooldown > 0 && GetWorldTime() < m_fNextUseTime)
		{
			SetCannotPerformReason("On cooldown");
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
	protected bool IsMaster()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			return gameMode.IsMaster();

		return Replication.IsServer();
	}

	//------------------------------------------------------------------------------------------------
	protected float GetWorldTime()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetWorldTime();
	}

	//------------------------------------------------------------------------------------------------
	protected int GetReenableDelayMs()
	{
		if (m_bOneUse)
			return 0;

		float delay = Math.Max(m_fEffectDelay, m_fCooldown);
		if (delay <= 0)
			return 0;

		return Math.Round(delay * 1000);
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
	protected bool IsEffectTargetReady()
	{
		if (!m_bRequireEffectTargetEntity)
			return true;

		if (m_sEffectTargetEntityName.IsEmpty())
			return false;

		return GetGame().GetWorld().FindEntityByName(m_sEffectTargetEntityName) != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredEntityReady()
	{
		if (m_sRequiredAliveEntityName.IsEmpty())
			return true;

		IEntity entity = GetGame().GetWorld().FindEntityByName(m_sRequiredAliveEntityName);
		return entity && IsEntityAlive(entity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsBlockingEntityActive()
	{
		if (m_sBlockingAliveEntityName.IsEmpty())
			return false;

		IEntity entity = GetGame().GetWorld().FindEntityByName(m_sBlockingAliveEntityName);
		return entity && IsEntityAlive(entity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsEntityAlive(IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity GetEffectAnchor(IEntity owner)
	{
		if (!m_sEffectTargetEntityName.IsEmpty())
		{
			IEntity target = GetGame().GetWorld().FindEntityByName(m_sEffectTargetEntityName);
			if (target)
				return target;

			if (m_bRequireEffectTargetEntity)
			{
				Print(string.Format("LCN_InteractableSupportAction: required effect target entity '%1' was not found", m_sEffectTargetEntityName));
				return null;
			}

			Print(string.Format("LCN_InteractableSupportAction: effect target entity '%1' was not found, using action owner", m_sEffectTargetEntityName));
		}

		return owner;
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnEffectModule(IEntity owner)
	{
		m_bPending = false;

		if (!owner)
			return;

		if (!m_sEffectModulePrefab)
		{
			Print("LCN_InteractableSupportAction: no effect module prefab configured");
			ReenableAction();
			return;
		}

		Resource effectResource = Resource.Load(m_sEffectModulePrefab);
		if (!effectResource || !effectResource.IsValid())
		{
			Print(string.Format("LCN_InteractableSupportAction: failed to load effect module '%1'", m_sEffectModulePrefab));
			ReenableAction();
			return;
		}

		IEntity anchor = GetEffectAnchor(owner);
		if (!anchor)
		{
			ReenableAction();
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		anchor.GetTransform(spawnParams.Transform);

		IEntity effectEntity = GetGame().SpawnEntityPrefab(effectResource, anchor.GetWorld(), spawnParams);
		if (!effectEntity)
		{
			Print("LCN_InteractableSupportAction: effect module spawn failed");
			ReenableAction();
			return;
		}

		if (m_bDebug)
			Print(string.Format("LCN_InteractableSupportAction: effect module spawned '%1' at %2", m_sEffectModulePrefab, anchor.GetOrigin().ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void ReenableAction()
	{
		if (m_bOneUse)
			return;

		m_bPending = false;
		SetActionEnabled_S(true);
	}
}
