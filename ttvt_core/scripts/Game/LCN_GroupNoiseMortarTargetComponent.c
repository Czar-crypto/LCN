[ComponentEditorProps(category: "LCN/Combat", description: "Tracks sustained weapon noise from one AI/player group and creates a temporary mortar target zone")]
class LCN_GroupNoiseMortarTargetComponentClass : ScriptComponentClass
{
}

class LCN_GroupNoiseMortarTargetComponent : ScriptComponent
{
	protected static ref array<LCN_GroupNoiseMortarTargetComponent> s_aComponents;

	[Attribute("1", UIWidgets.CheckBox, "Enable this group noise tracker", category: "LCN Group Noise")]
	protected bool m_bEnabled;

	[Attribute("GroupNoiseMortar", UIWidgets.EditBox, "Debug name used in script log", category: "LCN Group Noise")]
	protected string m_sDebugName;

	[Attribute("150", UIWidgets.EditBox, "How long the group must keep making noise in roughly one place", params: "10 900 1", category: "LCN Group Noise")]
	protected float m_fRequiredNoisyTime;

	[Attribute("90", UIWidgets.EditBox, "Noise score required before the target zone can be created", params: "1 10000 1", category: "LCN Group Noise")]
	protected float m_fRequiredNoiseScore;

	[Attribute("22", UIWidgets.EditBox, "Score that must remain present for the noisy timer to keep counting", params: "0 10000 1", category: "LCN Group Noise")]
	protected float m_fMinimumScoreToCountTime;

	[Attribute("1", UIWidgets.EditBox, "Score multiplier for one weapon shot", params: "0 100 0.1", category: "LCN Group Noise")]
	protected float m_fWeaponFireScore;

	[Attribute("0.32", UIWidgets.EditBox, "Noise score decay per second", params: "0 100 0.01", category: "LCN Group Noise")]
	protected float m_fNoiseDecayPerSecond;

	[Attribute("35", UIWidgets.EditBox, "A shot must be this close to a current group member to count", params: "1 200 1", category: "LCN Group Noise")]
	protected float m_fMemberNoiseMatchRadius;

	[Attribute("90", UIWidgets.EditBox, "If new noise appears farther than this from the current noise cluster, the cluster resets", params: "10 500 1", category: "LCN Group Noise")]
	protected float m_fClusterResetDistance;

	[Attribute("120", UIWidgets.EditBox, "If the group center moves farther than this from the cluster start, the cluster resets", params: "10 1000 1", category: "LCN Group Noise")]
	protected float m_fMaxGroupMoveDistance;

	[Attribute("360", UIWidgets.EditBox, "Cooldown after a target zone is created", params: "0 3600 1", category: "LCN Group Noise")]
	protected float m_fRetriggerCooldown;

	[Attribute("250", UIWidgets.EditBox, "Square target zone size in meters", params: "50 1000 1", category: "LCN Mortar Target")]
	protected float m_fTargetZoneSize;

	[Attribute("7", UIWidgets.EditBox, "How many mortar impacts this triggered target fires", params: "1 50 1", category: "LCN Mortar Target")]
	protected int m_iTargetRounds;

	[Attribute("3", UIWidgets.EditBox, "Minimum delay before first impact after the target is created", params: "0 300 0.1", category: "LCN Mortar Target")]
	protected float m_fTargetFirstRoundDelayMin;

	[Attribute("8", UIWidgets.EditBox, "Maximum delay before first impact after the target is created", params: "0 300 0.1", category: "LCN Mortar Target")]
	protected float m_fTargetFirstRoundDelayMax;

	[Attribute("15", UIWidgets.EditBox, "Minimum delay between impacts", params: "1 600 0.1", category: "LCN Mortar Target")]
	protected float m_fTargetRoundDelayMin;

	[Attribute("20", UIWidgets.EditBox, "Maximum delay between impacts", params: "1 600 0.1", category: "LCN Mortar Target")]
	protected float m_fTargetRoundDelayMax;

	[Attribute("{AD5A5C56F9A14721}Prefabs/Triggers/CombatActivityMortar/GroupNoise/E_LCN_GroupNoiseMortarTarget_250.et", UIWidgets.ResourcePickerThumbnail, "Temporary target zone prefab spawned when the group is detected", "et", category: "LCN Mortar Target")]
	protected ResourceName m_sTargetZonePrefab;

	[Attribute("{E15B8A4A6D904A2E}Prefabs/Weapons/Projectiles/Mortar/Ammo_Shell_82mm_HE_O832DU.et", UIWidgets.ResourcePickerThumbnail, "Projectile prefab used by the spawned target zone when no effect module is configured", "et", category: "LCN Mortar Target")]
	protected ResourceName m_sImpactProjectilePrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Optional effect module used by the spawned target zone instead of projectile prefab", "et", category: "LCN Mortar Target")]
	protected ResourceName m_sImpactEffectModulePrefab;

	[Attribute("1", UIWidgets.CheckBox, "Snap the created target zone to terrain", category: "LCN Mortar Target")]
	protected bool m_bSnapTargetToGround;

	[Attribute("1", UIWidgets.CheckBox, "First impacts are scattered, later impacts become more accurate", category: "LCN Mortar Accuracy")]
	protected bool m_bTargetProgressiveAccuracy;

	[Attribute("0.22", UIWidgets.EditBox, "Final spread multiplier after ranging impacts", params: "0.05 1 0.01", category: "LCN Mortar Accuracy")]
	protected float m_fTargetFinalAccuracyScale;

	[Attribute("5", UIWidgets.EditBox, "How many impacts it takes to reach final accuracy", params: "1 50 1", category: "LCN Mortar Accuracy")]
	protected int m_iTargetAccuracyRounds;

	[Attribute("55", UIWidgets.EditBox, "Early impacts try to avoid landing closer than this to the target center", params: "0 500 1", category: "LCN Mortar Accuracy")]
	protected float m_fTargetEarlyCenterMissDistance;

	[Attribute("60", UIWidgets.EditBox, "Early impacts try to keep at least this distance from previous impacts", params: "0 500 1", category: "LCN Mortar Accuracy")]
	protected float m_fTargetEarlyImpactMinSpacing;

	[Attribute("12", UIWidgets.EditBox, "Final minimum distance from previous impacts", params: "0 500 1", category: "LCN Mortar Accuracy")]
	protected float m_fTargetFinalImpactMinSpacing;

	[Attribute("24", UIWidgets.EditBox, "Random attempts used to find a well-spaced impact position", params: "1 100 1", category: "LCN Mortar Accuracy")]
	protected int m_iTargetImpactPositionAttempts;

	[Attribute("", UIWidgets.EditBox, "Objective key that must be active for this automatic mortar to work, e.g. LCN_AMMO_DEPOT_01", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Second objective key that must be active", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey2;

	[Attribute("", UIWidgets.EditBox, "Objective key that blocks this automatic mortar while active", category: "LCN Requirements")]
	protected string m_sBlockingObjectiveKey;

	[Attribute("0", UIWidgets.CheckBox, "Print tracker state to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected IEntity m_Owner;
	protected SCR_AIGroup m_Group;
	protected ref Resource m_TargetZoneResource;

	protected vector m_vClusterCenter;
	protected vector m_vClusterAnchor;
	protected vector m_vGroupAnchor;
	protected float m_fNoiseScore;
	protected float m_fNoisyTime;
	protected float m_fCooldownRemaining;
	protected bool m_bClusterActive;
	protected bool m_bWarnedNoTargetPrefab;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (RplSession.Mode() == RplMode.Client)
			return;

		m_Owner = owner;
		m_Group = SCR_AIGroup.Cast(owner);

		RegisterComponent(this);
		LoadTargetZoneResource();
		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	void ~LCN_GroupNoiseMortarTargetComponent()
	{
		if (s_aComponents)
			s_aComponents.RemoveItem(this);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!m_bEnabled)
			return;

		UpdateCooldown(timeSlice);
		UpdateCluster(owner, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	static void ReportCombatNoise(vector position, float amount, BaseWorld world = null, string source = "combat")
	{
		if (amount <= 0)
			return;

		array<LCN_GroupNoiseMortarTargetComponent> components = GetComponentRegistry();
		foreach (LCN_GroupNoiseMortarTargetComponent component : components)
		{
			if (!component)
				continue;

			component.ProcessCombatNoise(position, amount, world, source);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ProcessCombatNoise(vector position, float amount, BaseWorld world, string source)
	{
		if (!m_bEnabled)
			return;

		if (!m_Owner)
			return;

		if (world && m_Owner.GetWorld() != world)
			return;

		if (m_fCooldownRemaining > 0)
			return;

		if (!IsNoiseNearGroupMember(position))
			return;

		if (!m_bClusterActive)
			StartCluster(position);
		else if (DistanceSqXZ(position, m_vClusterCenter) > Square(Math.Max(m_fClusterResetDistance, 1.0)))
			StartCluster(position);
		else
			BlendClusterCenter(position);

		float scoreGain = amount;
		if (source == "weapon-fire")
			scoreGain = amount * Math.Max(m_fWeaponFireScore, 0.0);

		m_fNoiseScore = Math.Clamp(m_fNoiseScore + scoreGain, 0.0, Math.Max(m_fRequiredNoiseScore * 1.75, 1.0));

		if (m_bDebug)
			Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: +%2 from %3, score=%4/%5, time=%6/%7", m_sDebugName, scoreGain, source, m_fNoiseScore, m_fRequiredNoiseScore, m_fNoisyTime, m_fRequiredNoisyTime));
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCluster(IEntity owner, float timeSlice)
	{
		if (!m_bClusterActive)
			return;

		m_fNoiseScore = Math.Max(m_fNoiseScore - Math.Max(m_fNoiseDecayPerSecond, 0.0) * timeSlice, 0.0);
		if (m_fNoiseScore <= 0)
		{
			ResetCluster();
			return;
		}

		vector groupCenter = GetGroupCenter(owner);
		if (DistanceSqXZ(groupCenter, m_vGroupAnchor) > Square(Math.Max(m_fMaxGroupMoveDistance, 1.0)))
		{
			if (m_bDebug)
				Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: reset because group moved away from noisy point", m_sDebugName));

			ResetCluster();
			return;
		}

		if (DistanceSqXZ(m_vClusterCenter, m_vClusterAnchor) > Square(Math.Max(m_fClusterResetDistance, 1.0)))
		{
			if (m_bDebug)
				Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: reset because noise cluster drifted", m_sDebugName));

			ResetCluster();
			return;
		}

		if (m_fNoiseScore >= m_fMinimumScoreToCountTime)
			m_fNoisyTime += timeSlice;
		else
			m_fNoisyTime = Math.Max(m_fNoisyTime - timeSlice, 0.0);

		if (m_fNoisyTime < m_fRequiredNoisyTime)
			return;

		if (m_fNoiseScore < m_fRequiredNoiseScore)
			return;

		CreateTargetZone(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void StartCluster(vector position)
	{
		m_bClusterActive = true;
		m_vClusterCenter = position;
		m_vClusterAnchor = position;
		m_vGroupAnchor = GetGroupCenter(m_Owner);
		m_fNoiseScore = 0;
		m_fNoisyTime = 0;

		if (m_bDebug)
			Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: new noise cluster at %2", m_sDebugName, position.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void BlendClusterCenter(vector position)
	{
		float blend = 0.12;
		m_vClusterCenter[0] = m_vClusterCenter[0] + (position[0] - m_vClusterCenter[0]) * blend;
		m_vClusterCenter[1] = position[1];
		m_vClusterCenter[2] = m_vClusterCenter[2] + (position[2] - m_vClusterCenter[2]) * blend;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetCluster()
	{
		m_bClusterActive = false;
		m_fNoiseScore = 0;
		m_fNoisyTime = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsNoiseNearGroupMember(vector position)
	{
		float maxDistanceSq = Square(Math.Max(m_fMemberNoiseMatchRadius, 1.0));

		if (!m_Group)
			return DistanceSqXZ(position, m_Owner.GetOrigin()) <= maxDistanceSq;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);

		if (agents.IsEmpty())
			return DistanceSqXZ(position, m_Owner.GetOrigin()) <= maxDistanceSq;

		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity controlledEntity = agent.GetControlledEntity();
			if (!controlledEntity)
				continue;

			if (DistanceSqXZ(position, controlledEntity.GetOrigin()) <= maxDistanceSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetGroupCenter(IEntity owner)
	{
		if (m_Group && m_Group.GetAgentsCount() > 0)
			return m_Group.GetCenterOfMass();

		if (owner)
			return owner.GetOrigin();

		return vector.Zero;
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateTargetZone(IEntity owner)
	{
		if (!CanFire(owner))
		{
			if (m_bDebug)
				Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: target skipped because requirement is offline", m_sDebugName));

			ResetCluster();
			m_fCooldownRemaining = Math.Max(m_fRetriggerCooldown * 0.25, 10.0);
			return;
		}

		LoadTargetZoneResource();
		if (!m_TargetZoneResource || !m_TargetZoneResource.IsValid())
		{
			if (!m_bWarnedNoTargetPrefab)
			{
				m_bWarnedNoTargetPrefab = true;
				Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: no valid target zone prefab configured", m_sDebugName));
			}

			ResetCluster();
			return;
		}

		vector targetPosition = m_vClusterCenter;
		if (m_bSnapTargetToGround)
			targetPosition = SnapPositionToGround(owner.GetWorld(), targetPosition);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = targetPosition;

		IEntity targetEntity = GetGame().SpawnEntityPrefab(m_TargetZoneResource, owner.GetWorld(), spawnParams);
		LCN_CombatActivityMortarZoneEntity targetZone = LCN_CombatActivityMortarZoneEntity.Cast(targetEntity);
		if (!targetZone)
		{
			Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: spawned target prefab is not LCN_CombatActivityMortarZoneEntity", m_sDebugName));
			ResetCluster();
			return;
		}

		targetZone.LCN_ConfigureExternalSquareBarrage(m_sDebugName + "_Target", Math.Max(m_fTargetZoneSize, 50.0), m_sImpactProjectilePrefab, m_sImpactEffectModulePrefab, m_sRequiredObjectiveKey, m_sRequiredObjectiveKey2, m_sBlockingObjectiveKey, m_bDebug);
		targetZone.LCN_ConfigureExternalBarrageSchedule(m_fTargetFirstRoundDelayMin, m_fTargetFirstRoundDelayMax, m_fTargetRoundDelayMin, m_fTargetRoundDelayMax, m_iTargetRounds, 1.0);
		targetZone.LCN_ConfigureExternalProgressiveAccuracy(m_bTargetProgressiveAccuracy, m_fTargetFinalAccuracyScale, m_iTargetAccuracyRounds, m_fTargetEarlyCenterMissDistance, m_fTargetEarlyImpactMinSpacing, m_fTargetFinalImpactMinSpacing, m_iTargetImpactPositionAttempts);
		if (!targetZone.LCN_TriggerExternalBarrage())
		{
			if (m_bDebug)
				Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: target zone spawned but barrage did not start", m_sDebugName));
		}
		else if (m_bDebug)
		{
			Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: target zone created at %2 size=%3", m_sDebugName, targetPosition.ToString(), m_fTargetZoneSize));
		}

		m_fCooldownRemaining = Math.Max(m_fRetriggerCooldown, 0.0);
		ResetCluster();
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadTargetZoneResource()
	{
		if (m_TargetZoneResource && m_TargetZoneResource.IsValid())
			return;

		if (!m_sTargetZonePrefab)
			return;

		m_TargetZoneResource = Resource.Load(m_sTargetZonePrefab);
		if ((!m_TargetZoneResource || !m_TargetZoneResource.IsValid()) && !m_bWarnedNoTargetPrefab)
		{
			m_bWarnedNoTargetPrefab = true;
			Print(string.Format("LCN_GroupNoiseMortarTargetComponent[%1]: failed to load target zone prefab '%2'", m_sDebugName, m_sTargetZonePrefab));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCooldown(float timeSlice)
	{
		if (m_fCooldownRemaining <= 0)
			return;

		m_fCooldownRemaining = Math.Max(m_fCooldownRemaining - timeSlice, 0.0);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanFire(IEntity owner)
	{
		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!IsRequiredObjectiveKeyActive(m_sRequiredObjectiveKey, world))
			return false;

		if (!IsRequiredObjectiveKeyActive(m_sRequiredObjectiveKey2, world))
			return false;

		if (!m_sBlockingObjectiveKey.IsEmpty() && LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sBlockingObjectiveKey, world))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRequiredObjectiveKeyActive(string objectiveKey, BaseWorld world)
	{
		if (objectiveKey.IsEmpty())
			return true;

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(objectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected vector SnapPositionToGround(BaseWorld world, vector position)
	{
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
	protected float DistanceSqXZ(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}

	//------------------------------------------------------------------------------------------------
	protected float Square(float value)
	{
		return value * value;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RegisterComponent(LCN_GroupNoiseMortarTargetComponent component)
	{
		array<LCN_GroupNoiseMortarTargetComponent> components = GetComponentRegistry();
		if (components.Find(component) == -1)
			components.Insert(component);
	}

	//------------------------------------------------------------------------------------------------
	protected static array<LCN_GroupNoiseMortarTargetComponent> GetComponentRegistry()
	{
		if (!s_aComponents)
			s_aComponents = {};

		return s_aComponents;
	}
}
