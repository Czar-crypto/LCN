[ComponentEditorProps(category: "LCN/Fire Mission", description: "Server-side fire mission state, correction, and GM effect spawning")]
class LCN_FireMissionConsoleComponentClass : ScriptComponentClass
{
}

class LCN_FireMissionConsoleComponent : ScriptComponent
{
	protected static ref array<LCN_FireMissionConsoleComponent> s_aConsoles;

	static const int ACTION_SET_TARGET_FROM_VIEW = 0;
	static const int ACTION_SET_TARGET_FROM_MARKER = 1;
	static const int ACTION_SPOTTING_ROUND = 2;
	static const int ACTION_FIRE_FOR_EFFECT = 3;
	static const int ACTION_CORRECT_LEFT = 4;
	static const int ACTION_CORRECT_RIGHT = 5;
	static const int ACTION_CORRECT_ADD = 6;
	static const int ACTION_CORRECT_DROP = 7;
	static const int ACTION_CLEAR_MISSION = 8;

	[Attribute("{5D48E2F7DB0C3714}PrefabsEditable/EffectsModules/Mortar/EffectModule_Zoned_MortarBarrage_Small.et", UIWidgets.ResourcePickerThumbnail, "GM effect module used for a spotting round", "et", category: "LCN Fire Mission")]
	protected ResourceName m_sSpottingEffectModulePrefab;

	[Attribute("{E7B7D4467E9A82DD}PrefabsEditable/EffectsModules/Mortar/EffectModule_Zoned_MortarBarrage_Medium.et", UIWidgets.ResourcePickerThumbnail, "GM effect module used for fire for effect", "et", category: "LCN Fire Mission")]
	protected ResourceName m_sFireForEffectModulePrefab;

	[Attribute("8", UIWidgets.EditBox, "Delay in seconds before a spotting round is spawned", params: "0 1800 1", category: "LCN Fire Mission")]
	protected float m_fSpottingDelay;

	[Attribute("15", UIWidgets.EditBox, "Delay in seconds before fire for effect is spawned", params: "0 1800 1", category: "LCN Fire Mission")]
	protected float m_fFireDelay;

	[Attribute("240", UIWidgets.EditBox, "Cooldown in seconds after fire for effect", params: "0 3600 1", category: "LCN Fire Mission")]
	protected float m_fFireCooldown;

	[Attribute("50", UIWidgets.EditBox, "Default left/right correction in metres", params: "1 1000 1", category: "LCN Fire Mission")]
	protected float m_fDefaultLateralCorrection;

	[Attribute("100", UIWidgets.EditBox, "Default add/drop correction in metres", params: "1 2000 1", category: "LCN Fire Mission")]
	protected float m_fDefaultRangeCorrection;

	[Attribute("1", UIWidgets.CheckBox, "Snap requested target positions down to the ground before spawning effects", category: "LCN Fire Mission")]
	protected bool m_bSnapTargetToGround;

	[Attribute("LCN_FM_MARKER_01", UIWidgets.EditBox, "Optional entity name used by 'Set target from marker'", category: "LCN Links")]
	protected string m_sDefaultTargetEntityName;

	[Attribute("US", UIWidgets.EditBox, "Only this faction can request/correct/fire. Leave empty to allow any faction", category: "LCN Faction")]
	protected FactionKey m_sAllowedFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Accept inherited factions when checking allowed faction", category: "LCN Faction")]
	protected bool m_bAcceptInheritedFaction;

	[Attribute("LCN_COMMS_01", UIWidgets.EditBox, "First objective key that must be active", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Second objective key that must be active", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey2;

	[Attribute("", UIWidgets.EditBox, "Third objective key that must be active", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey3;

	[Attribute("", UIWidgets.EditBox, "Objective key that blocks this console while active", category: "LCN Requirements")]
	protected string m_sBlockingObjectiveKey;

	[Attribute("1", UIWidgets.CheckBox, "Print fire mission events to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected IEntity m_Owner;

	[RplProp()]
	protected bool m_bHasMission;

	[RplProp()]
	protected bool m_bSpottingPending;

	[RplProp()]
	protected bool m_bFirePending;

	[RplProp()]
	protected float m_fBaseTargetX;

	[RplProp()]
	protected float m_fBaseTargetY;

	[RplProp()]
	protected float m_fBaseTargetZ;

	[RplProp()]
	protected float m_fAdjustedTargetX;

	[RplProp()]
	protected float m_fAdjustedTargetY;

	[RplProp()]
	protected float m_fAdjustedTargetZ;

	[RplProp()]
	protected float m_fObserverX;

	[RplProp()]
	protected float m_fObserverY;

	[RplProp()]
	protected float m_fObserverZ;

	[RplProp()]
	protected float m_fCorrectionLateral;

	[RplProp()]
	protected float m_fCorrectionRange;

	[RplProp()]
	protected float m_fNextFireTime;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Owner = owner;

		array<LCN_FireMissionConsoleComponent> consoles = GetConsoleRegistry();
		if (consoles.Find(this) == -1)
			consoles.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	static LCN_FireMissionConsoleComponent FindConsole(string consoleEntityName = "", BaseWorld world = null)
	{
		if (!consoleEntityName.IsEmpty() && world)
		{
			IEntity namedEntity = world.FindEntityByName(consoleEntityName);
			if (namedEntity)
				return LCN_FireMissionConsoleComponent.Cast(namedEntity.FindComponent(LCN_FireMissionConsoleComponent));

			return null;
		}

		array<LCN_FireMissionConsoleComponent> consoles = GetConsoleRegistry();
		LCN_FireMissionConsoleComponent firstInWorld;

		foreach (LCN_FireMissionConsoleComponent console : consoles)
		{
			if (!console)
				continue;

			IEntity owner = console.GetOwnerEntity();
			if (!owner)
				continue;

			if (world && owner.GetWorld() != world)
				continue;

			if (!firstInWorld)
				firstInWorld = console;
		}

		return firstInWorld;
	}

	//------------------------------------------------------------------------------------------------
	IEntity GetOwnerEntity()
	{
		return m_Owner;
	}

	//------------------------------------------------------------------------------------------------
	float GetDefaultLateralCorrection()
	{
		return Math.Max(m_fDefaultLateralCorrection, 1);
	}

	//------------------------------------------------------------------------------------------------
	float GetDefaultRangeCorrection()
	{
		return Math.Max(m_fDefaultRangeCorrection, 1);
	}

	//------------------------------------------------------------------------------------------------
	bool CanPerformAction(int actionType, IEntity user, out string reason)
	{
		reason = "";

		if (!m_Owner || !IsEntityAlive(m_Owner))
		{
			reason = "Fire direction center destroyed";
			return false;
		}

		if (!IsUserFactionAllowed(user))
		{
			reason = "Wrong faction";
			return false;
		}

		if (!AreRequiredObjectivesActive())
		{
			reason = "Required system offline";
			return false;
		}

		if (IsBlockingObjectiveActive())
		{
			reason = "Blocked by active objective";
			return false;
		}

		if (actionType == ACTION_SET_TARGET_FROM_VIEW)
			return true;

		if (actionType == ACTION_SET_TARGET_FROM_MARKER)
		{
			if (m_sDefaultTargetEntityName.IsEmpty())
			{
				reason = "Target marker name is empty";
				return false;
			}

			if (!m_Owner.GetWorld().FindEntityByName(m_sDefaultTargetEntityName))
			{
				reason = "Target marker missing";
				return false;
			}

			return true;
		}

		if (actionType == ACTION_CLEAR_MISSION)
			return m_bHasMission || m_bSpottingPending || m_bFirePending;

		if (!m_bHasMission)
		{
			reason = "No fire mission target";
			return false;
		}

		if (actionType == ACTION_SPOTTING_ROUND && m_bSpottingPending)
		{
			reason = "Spotting round pending";
			return false;
		}

		if (actionType == ACTION_FIRE_FOR_EFFECT)
		{
			if (m_bFirePending)
			{
				reason = "Fire mission pending";
				return false;
			}

			if (m_fFireCooldown > 0 && GetWorldTime() < m_fNextFireTime)
			{
				reason = "Fire mission on cooldown";
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void RequestAction(int actionType, float amount, vector observedTarget, vector observerPosition, int playerId, IEntity directUser = null)
	{
		if (IsMaster())
		{
			IEntity user = directUser;
			if (!user)
				user = GetPlayerControlledEntity(playerId);

			ExecuteAction(actionType, amount, observedTarget, observerPosition, playerId, user);
			return;
		}

		Rpc(RpcAsk_RequestAction, actionType, amount, observedTarget, observerPosition, playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAction(int actionType, float amount, vector observedTarget, vector observerPosition, int playerId)
	{
		ExecuteAction(actionType, amount, observedTarget, observerPosition, playerId, GetPlayerControlledEntity(playerId));
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteAction(int actionType, float amount, vector observedTarget, vector observerPosition, int playerId, IEntity user)
	{
		string reason;
		if (!CanPerformAction(actionType, user, reason))
		{
			if (m_bDebug)
				Print(string.Format("LCN_FireMissionConsoleComponent: action %1 rejected for player=%2 reason='%3'", actionType, playerId, reason));

			return;
		}

		if (actionType == ACTION_SET_TARGET_FROM_VIEW)
		{
			SetMissionTarget(observedTarget, observerPosition, user, "view");
			return;
		}

		if (actionType == ACTION_SET_TARGET_FROM_MARKER)
		{
			SetMissionTargetFromMarker(user);
			return;
		}

		if (actionType == ACTION_SPOTTING_ROUND)
		{
			QueueSpottingRound(user);
			return;
		}

		if (actionType == ACTION_FIRE_FOR_EFFECT)
		{
			QueueFireForEffect(user);
			return;
		}

		if (actionType == ACTION_CORRECT_LEFT)
		{
			ApplyCorrection(-GetCorrectionAmount(amount, GetDefaultLateralCorrection()), 0, user);
			return;
		}

		if (actionType == ACTION_CORRECT_RIGHT)
		{
			ApplyCorrection(GetCorrectionAmount(amount, GetDefaultLateralCorrection()), 0, user);
			return;
		}

		if (actionType == ACTION_CORRECT_ADD)
		{
			ApplyCorrection(0, GetCorrectionAmount(amount, GetDefaultRangeCorrection()), user);
			return;
		}

		if (actionType == ACTION_CORRECT_DROP)
		{
			ApplyCorrection(0, -GetCorrectionAmount(amount, GetDefaultRangeCorrection()), user);
			return;
		}

		if (actionType == ACTION_CLEAR_MISSION)
			ClearMission(user);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCorrectionAmount(float amount, float fallback)
	{
		if (amount > 0)
			return amount;

		return fallback;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetMissionTargetFromMarker(IEntity user)
	{
		IEntity marker = m_Owner.GetWorld().FindEntityByName(m_sDefaultTargetEntityName);
		if (!marker)
			return;

		vector observerPosition = m_Owner.GetOrigin();
		SetMissionTarget(marker.GetOrigin(), observerPosition, user, "marker");
	}

	//------------------------------------------------------------------------------------------------
	protected void SetMissionTarget(vector observedTarget, vector observerPosition, IEntity user, string source)
	{
		if (observedTarget == vector.Zero)
		{
			if (m_bDebug)
				Print("LCN_FireMissionConsoleComponent: ignored empty observed target");

			return;
		}

		vector target = observedTarget;
		if (m_bSnapTargetToGround)
			target = SnapPositionToGround(target);

		m_bHasMission = true;
		m_bSpottingPending = false;
		m_fCorrectionLateral = 0;
		m_fCorrectionRange = 0;

		SetBaseTarget(target);

		if (observerPosition == vector.Zero)
			observerPosition = m_Owner.GetOrigin();

		SetObserverPosition(observerPosition);
		SetAdjustedTarget(target);
		BumpState();

		if (m_bDebug)
			Print(string.Format("LCN_FireMissionConsoleComponent: target set from %1 by %2 base=%3 observer=%4", source, user, target.ToString(), observerPosition.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyCorrection(float lateralDelta, float rangeDelta, IEntity user)
	{
		m_fCorrectionLateral += lateralDelta;
		m_fCorrectionRange += rangeDelta;
		UpdateAdjustedTarget();
		BumpState();

		if (m_bDebug)
			Print(string.Format("LCN_FireMissionConsoleComponent: corrected by %1 lateral=%2 range=%3 adjusted=%4", user, m_fCorrectionLateral, m_fCorrectionRange, GetAdjustedTarget().ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateAdjustedTarget()
	{
		vector baseTarget = GetBaseTarget();
		vector observer = GetObserverPosition();

		vector rangeDir = baseTarget - observer;
		rangeDir[1] = 0;

		if (rangeDir.Length() < 0.1)
			rangeDir = Vector(0, 0, 1);
		else
			rangeDir.Normalize();

		vector rightDir = Vector(rangeDir[2], 0, -rangeDir[0]);
		vector adjustedTarget = baseTarget + rightDir * m_fCorrectionLateral + rangeDir * m_fCorrectionRange;

		if (m_bSnapTargetToGround)
			adjustedTarget = SnapPositionToGround(adjustedTarget);

		SetAdjustedTarget(adjustedTarget);
	}

	//------------------------------------------------------------------------------------------------
	protected void QueueSpottingRound(IEntity user)
	{
		m_bSpottingPending = true;
		vector target = GetAdjustedTarget();
		BumpState();

		GetGame().GetCallqueue().CallLater(SpawnSpottingRoundAt, Math.Round(Math.Max(m_fSpottingDelay, 0) * 1000), false, target[0], target[1], target[2]);

		if (m_bDebug)
			Print(string.Format("LCN_FireMissionConsoleComponent: spotting round queued by %1 delay=%2 target=%3", user, m_fSpottingDelay, target.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void QueueFireForEffect(IEntity user)
	{
		m_bFirePending = true;
		vector target = GetAdjustedTarget();

		if (m_fFireCooldown > 0)
			m_fNextFireTime = GetWorldTime() + Math.Round(m_fFireCooldown * 1000);

		BumpState();
		GetGame().GetCallqueue().CallLater(SpawnFireForEffectAt, Math.Round(Math.Max(m_fFireDelay, 0) * 1000), false, target[0], target[1], target[2]);

		if (m_bDebug)
			Print(string.Format("LCN_FireMissionConsoleComponent: fire for effect queued by %1 delay=%2 cooldown=%3 target=%4", user, m_fFireDelay, m_fFireCooldown, target.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnSpottingRoundAt(float x, float y, float z)
	{
		m_bSpottingPending = false;
		SpawnEffectModuleAt(m_sSpottingEffectModulePrefab, Vector(x, y, z), "spotting");
		BumpState();
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnFireForEffectAt(float x, float y, float z)
	{
		m_bFirePending = false;
		SpawnEffectModuleAt(m_sFireForEffectModulePrefab, Vector(x, y, z), "fire-for-effect");
		BumpState();
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnEffectModuleAt(ResourceName effectPrefab, vector position, string label)
	{
		if (!effectPrefab)
		{
			Print(string.Format("LCN_FireMissionConsoleComponent: %1 effect prefab is empty", label));
			return;
		}

		Resource effectResource = Resource.Load(effectPrefab);
		if (!effectResource || !effectResource.IsValid())
		{
			Print(string.Format("LCN_FireMissionConsoleComponent: failed to load %1 effect '%2'", label, effectPrefab));
			return;
		}

		if (m_bSnapTargetToGround)
			position = SnapPositionToGround(position);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = position;

		IEntity effectEntity = GetGame().SpawnEntityPrefab(effectResource, m_Owner.GetWorld(), spawnParams);
		if (!effectEntity)
		{
			Print(string.Format("LCN_FireMissionConsoleComponent: %1 effect spawn failed", label));
			return;
		}

		if (m_bDebug)
			Print(string.Format("LCN_FireMissionConsoleComponent: %1 effect spawned '%2' at %3", label, effectPrefab, position.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearMission(IEntity user)
	{
		m_bHasMission = false;
		m_bSpottingPending = false;
		m_bFirePending = false;
		m_fCorrectionLateral = 0;
		m_fCorrectionRange = 0;
		SetBaseTarget(vector.Zero);
		SetAdjustedTarget(vector.Zero);
		SetObserverPosition(vector.Zero);
		BumpState();

		if (m_bDebug)
			Print(string.Format("LCN_FireMissionConsoleComponent: mission cleared by %1", user));
	}

	//------------------------------------------------------------------------------------------------
	string GetStatusText()
	{
		if (!m_bHasMission)
			return "FDC: no target";

		vector target = GetAdjustedTarget();
		string status = string.Format("FDC grid X%1 Z%2", target[0].ToString(0, 0), target[2].ToString(0, 0));

		if (m_fCorrectionLateral < 0)
		{
			float leftCorrection = -m_fCorrectionLateral;
			status += string.Format(" | Left %1 m", leftCorrection.ToString(0, 0));
		}
		else if (m_fCorrectionLateral > 0)
			status += string.Format(" | Right %1 m", m_fCorrectionLateral.ToString(0, 0));

		if (m_fCorrectionRange < 0)
		{
			float dropCorrection = -m_fCorrectionRange;
			status += string.Format(" | Drop %1 m", dropCorrection.ToString(0, 0));
		}
		else if (m_fCorrectionRange > 0)
			status += string.Format(" | Add %1 m", m_fCorrectionRange.ToString(0, 0));

		if (m_bSpottingPending)
			status += " | spotting pending";

		if (m_bFirePending)
			status += " | fire pending";

		return status;
	}

	//------------------------------------------------------------------------------------------------
	protected vector SnapPositionToGround(vector position)
	{
		BaseWorld world = null;
		if (m_Owner)
			world = m_Owner.GetWorld();

		if (!world)
			return position;

		TraceParam trace = new TraceParam();
		trace.Start = Vector(position[0], position[1] + 250, position[2]);
		trace.End = Vector(position[0], position[1] - 500, position[2]);
		trace.Flags = TraceFlags.WORLD | TraceFlags.OCEAN;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;

		float traceDistance = world.TraceMove(trace, null);
		if (traceDistance == 1)
			return position;

		return trace.Start + (trace.End - trace.Start) * traceDistance;
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
	protected IEntity GetPlayerControlledEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || playerId <= 0)
			return null;

		return playerManager.GetPlayerControlledEntity(playerId);
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
	protected bool AreRequiredObjectivesActive()
	{
		BaseWorld world = null;
		if (m_Owner)
			world = m_Owner.GetWorld();

		return IsRequiredObjectiveActive(m_sRequiredObjectiveKey, world)
			&& IsRequiredObjectiveActive(m_sRequiredObjectiveKey2, world)
			&& IsRequiredObjectiveActive(m_sRequiredObjectiveKey3, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredObjectiveActive(string objectiveKey, BaseWorld world)
	{
		if (objectiveKey.IsEmpty())
			return true;

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(objectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsBlockingObjectiveActive()
	{
		if (m_sBlockingObjectiveKey.IsEmpty())
			return false;

		BaseWorld world = null;
		if (m_Owner)
			world = m_Owner.GetWorld();

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sBlockingObjectiveKey, world);
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
	protected void SetBaseTarget(vector target)
	{
		m_fBaseTargetX = target[0];
		m_fBaseTargetY = target[1];
		m_fBaseTargetZ = target[2];
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetBaseTarget()
	{
		return Vector(m_fBaseTargetX, m_fBaseTargetY, m_fBaseTargetZ);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetAdjustedTarget(vector target)
	{
		m_fAdjustedTargetX = target[0];
		m_fAdjustedTargetY = target[1];
		m_fAdjustedTargetZ = target[2];
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetAdjustedTarget()
	{
		return Vector(m_fAdjustedTargetX, m_fAdjustedTargetY, m_fAdjustedTargetZ);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetObserverPosition(vector observerPosition)
	{
		m_fObserverX = observerPosition[0];
		m_fObserverY = observerPosition[1];
		m_fObserverZ = observerPosition[2];
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetObserverPosition()
	{
		return Vector(m_fObserverX, m_fObserverY, m_fObserverZ);
	}

	//------------------------------------------------------------------------------------------------
	protected void BumpState()
	{
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected static array<LCN_FireMissionConsoleComponent> GetConsoleRegistry()
	{
		if (!s_aConsoles)
			s_aConsoles = new array<LCN_FireMissionConsoleComponent>();

		return s_aConsoles;
	}
}
