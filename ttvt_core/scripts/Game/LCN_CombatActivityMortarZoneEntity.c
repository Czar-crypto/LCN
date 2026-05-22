[EntityEditorProps(category: "LCN/Triggers", description: "Silent combat activity zone that starts scattered mortar impacts after sustained nearby weapon fire", color: "255 128 32 255", color2: "255 128 32 48", visible: true, style: "box", dynamicBox: true)]
class LCN_CombatActivityMortarZoneEntityClass : GenericEntityClass
{
}

class LCN_CombatActivityMortarZoneEntity : GenericEntity
{
	protected static ref array<LCN_CombatActivityMortarZoneEntity> s_aZones;
	protected static ref array<IEntity> s_aRecentProjectiles;
	protected static ref array<float> s_aRecentProjectileTimes;

	static const float PROJECTILE_DUPLICATE_WINDOW_MS = 250;
	static const int PROJECTILE_DEDUP_LIMIT = 96;
	static const int SHAPE_RADIUS = 0;
	static const int SHAPE_BOX = 1;

	[Attribute("AutoMortar_01", UIWidgets.EditBox, "Debug name for this combat activity zone", category: "LCN Activity")]
	protected string m_sZoneName;

	[Attribute("140", UIWidgets.EditBox, "Radius that receives combat noise impulses", params: "10 1000 1", category: "LCN Activity")]
	protected float m_fActivityRadius;

	[Attribute("0", UIWidgets.ComboBox, "Activity area shape", "", enums: {
		ParamEnum("Radius", "0"),
		ParamEnum("Box", "1")
	}, category: "LCN Activity")]
	protected int m_iActivityShape;

	[Attribute("250", UIWidgets.EditBox, "Activity box size on local X axis", params: "10 2000 1", category: "LCN Activity")]
	protected float m_fActivitySizeX;

	[Attribute("250", UIWidgets.EditBox, "Activity box size on local Z axis", params: "10 2000 1", category: "LCN Activity")]
	protected float m_fActivitySizeZ;

	[Attribute("115", UIWidgets.EditBox, "Activity score required before the zone starts a mortar sequence", params: "1 10000 1", category: "LCN Activity")]
	protected float m_fRequiredActivityScore;

	[Attribute("1", UIWidgets.EditBox, "Score added by one detected weapon shot", params: "0 100 0.1", category: "LCN Activity")]
	protected float m_fWeaponFireActivity;

	[Attribute("0.18", UIWidgets.EditBox, "Activity score decay per second when the fight goes quiet", params: "0 100 0.01", category: "LCN Activity")]
	protected float m_fActivityDecayPerSecond;

	[Attribute("1", UIWidgets.CheckBox, "Allow this zone to fire again after cooldown", category: "LCN Activity")]
	protected bool m_bAllowRetrigger;

	[Attribute("240", UIWidgets.EditBox, "Cooldown after a mortar sequence ends", params: "0 3600 1", category: "LCN Activity")]
	protected float m_fRetriggerCooldown;

	[Attribute("", UIWidgets.EditBox, "Objective key that must be active for this automatic mortar to work, e.g. ammo depot", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Second objective key that must be active", category: "LCN Requirements")]
	protected string m_sRequiredObjectiveKey2;

	[Attribute("", UIWidgets.EditBox, "Objective key that blocks this automatic mortar while active", category: "LCN Requirements")]
	protected string m_sBlockingObjectiveKey;

	[Attribute("{E15B8A4A6D904A2E}Prefabs/Weapons/Projectiles/Mortar/Ammo_Shell_82mm_HE_O832DU.et", desc: "Projectile prefab triggered for each single mortar impact when no effect module is configured", category: "LCN Barrage")]
	protected ResourceName m_sImpactProjectilePrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Optional GM effect module spawned for each impact. If set, it is used instead of the projectile prefab", "et", category: "LCN Barrage")]
	protected ResourceName m_sImpactEffectModulePrefab;

	[Attribute("1", UIWidgets.CheckBox, "Snap impact positions to terrain before spawning the effect", category: "LCN Barrage")]
	protected bool m_bSnapImpactToGround;

	[Attribute("70", UIWidgets.EditBox, "Random impact radius around the zone center", params: "1 1000 1", category: "LCN Barrage")]
	protected float m_fImpactRadius;

	[Attribute("0", UIWidgets.ComboBox, "Impact area shape", "", enums: {
		ParamEnum("Radius", "0"),
		ParamEnum("Box", "1")
	}, category: "LCN Barrage")]
	protected int m_iImpactShape;

	[Attribute("1000", UIWidgets.EditBox, "Impact box size on local X axis", params: "10 5000 1", category: "LCN Barrage")]
	protected float m_fImpactSizeX;

	[Attribute("1000", UIWidgets.EditBox, "Impact box size on local Z axis", params: "10 5000 1", category: "LCN Barrage")]
	protected float m_fImpactSizeZ;

	[Attribute("", UIWidgets.EditBox, "Optional entity name that marks the center of the impact box. Leave empty to use local offset from this zone", category: "LCN Barrage")]
	protected string m_sImpactCenterEntityName;

	[Attribute("0", UIWidgets.EditBox, "Local X offset from activity zone center to impact area center", params: "-5000 5000 1", category: "LCN Barrage")]
	protected float m_fImpactCenterOffsetX;

	[Attribute("0", UIWidgets.EditBox, "Local Z offset from activity zone center to impact area center", params: "-5000 5000 1", category: "LCN Barrage")]
	protected float m_fImpactCenterOffsetZ;

	[Attribute("8", UIWidgets.EditBox, "Minimum delay before the first impact", params: "0 300 0.1", category: "LCN Barrage")]
	protected float m_fFirstRoundDelayMin;

	[Attribute("18", UIWidgets.EditBox, "Maximum delay before the first impact", params: "0 300 0.1", category: "LCN Barrage")]
	protected float m_fFirstRoundDelayMax;

	[Attribute("14", UIWidgets.EditBox, "Minimum delay between impacts", params: "1 600 0.1", category: "LCN Barrage")]
	protected float m_fRoundDelayMin;

	[Attribute("32", UIWidgets.EditBox, "Maximum delay between impacts", params: "1 600 0.1", category: "LCN Barrage")]
	protected float m_fRoundDelayMax;

	[Attribute("3", UIWidgets.EditBox, "Minimum impacts before the random stop chance is allowed", params: "1 100 1", category: "LCN Barrage")]
	protected int m_iMinRoundsBeforeStop;

	[Attribute("10", UIWidgets.EditBox, "Hard limit for impacts in one sequence", params: "1 100 1", category: "LCN Barrage")]
	protected int m_iMaxRounds;

	[Attribute("0.72", UIWidgets.EditBox, "Chance that the mortar keeps firing after each impact once minimum impacts are done", params: "0 1 0.01", category: "LCN Barrage")]
	protected float m_fContinueChance;

	[Attribute("1", UIWidgets.CheckBox, "Shrink impact spread as the barrage continues", category: "LCN Barrage Accuracy")]
	protected bool m_bProgressiveAccuracy;

	[Attribute("0.28", UIWidgets.EditBox, "Final spread multiplier after ranging rounds", params: "0.05 1 0.01", category: "LCN Barrage Accuracy")]
	protected float m_fFinalAccuracyScale;

	[Attribute("5", UIWidgets.EditBox, "How many impacts it takes to reach final accuracy", params: "1 50 1", category: "LCN Barrage Accuracy")]
	protected int m_iAccuracyRounds;

	[Attribute("45", UIWidgets.EditBox, "Early impacts try to avoid landing closer than this to the target center", params: "0 500 1", category: "LCN Barrage Accuracy")]
	protected float m_fEarlyCenterMissDistance;

	[Attribute("55", UIWidgets.EditBox, "Early impacts try to keep at least this distance from previous impacts", params: "0 500 1", category: "LCN Barrage Accuracy")]
	protected float m_fEarlyImpactMinSpacing;

	[Attribute("10", UIWidgets.EditBox, "Final minimum distance from previous impacts", params: "0 500 1", category: "LCN Barrage Accuracy")]
	protected float m_fFinalImpactMinSpacing;

	[Attribute("20", UIWidgets.EditBox, "Random attempts used to find a well-spaced impact position", params: "1 100 1", category: "LCN Barrage Accuracy")]
	protected int m_iImpactPositionAttempts;

	[Attribute("0", UIWidgets.CheckBox, "Print activity and barrage state to script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected ref Resource m_ProjectileResource;
	protected ref Resource m_EffectResource;
	protected ref array<vector> m_aRecentImpactPositions = {};

	protected float m_fActivityScore;
	protected float m_fRoundDelay;
	protected float m_fCooldownRemaining;
	protected int m_iRoundsFired;
	protected bool m_bBarrageActive;
	protected bool m_bHasTriggered;
	protected bool m_bResourcesLoaded;
	protected bool m_bWarnedNoImpactPrefab;

	//------------------------------------------------------------------------------------------------
	void LCN_CombatActivityMortarZoneEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.FRAME);
		RegisterZone(this);
	}

	//------------------------------------------------------------------------------------------------
	void ~LCN_CombatActivityMortarZoneEntity()
	{
		if (s_aZones)
			s_aZones.RemoveItem(this);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		if (!m_bResourcesLoaded)
			LoadImpactResources();

		UpdateCooldown(timeSlice);

		if (m_bBarrageActive)
		{
			UpdateBarrage(owner, timeSlice);
			return;
		}

		UpdateActivity(owner, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	static void ReportWeaponFire(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		IEntity sourceEntity = effectEntity;
		if (!sourceEntity)
			sourceEntity = projectileEntity;

		if (!sourceEntity && muzzle)
			sourceEntity = muzzle.GetOwner();

		if (!sourceEntity)
			return;

		BaseWorld world = sourceEntity.GetWorld();
		if (ShouldIgnoreDuplicateProjectile(projectileEntity, world))
			return;

		ReportCombatActivity(sourceEntity.GetOrigin(), 1.0, world, "weapon-fire");
	}

	//------------------------------------------------------------------------------------------------
	static void ReportExplosion(vector position, BaseWorld world = null, float amount = 8.0)
	{
		ReportCombatActivity(position, amount, world, "explosion");
	}

	//------------------------------------------------------------------------------------------------
	static void ReportCombatActivity(vector position, float amount, BaseWorld world = null, string source = "combat")
	{
		if (amount <= 0)
			return;

		LCN_GroupNoiseMortarTargetComponent.ReportCombatNoise(position, amount, world, source);

		array<LCN_CombatActivityMortarZoneEntity> zones = GetZoneRegistry();
		foreach (LCN_CombatActivityMortarZoneEntity zone : zones)
		{
			if (!zone)
				continue;

			if (world && zone.GetWorld() != world)
				continue;

			if (!zone.IsPositionInside(position))
				continue;

			zone.AddActivity(amount, source);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddActivity(float amount, string source)
	{
		if (m_bBarrageActive)
			return;

		if (!m_bAllowRetrigger && m_bHasTriggered)
			return;

		if (m_fCooldownRemaining > 0)
			return;

		float scoreGain = amount;
		if (source == "weapon-fire")
			scoreGain = amount * Math.Max(m_fWeaponFireActivity, 0.0);

		m_fActivityScore = Math.Clamp(m_fActivityScore + scoreGain, 0.0, Math.Max(m_fRequiredActivityScore * 1.5, 1.0));

		if (m_bDebug)
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: +%2 activity from %3, score=%4/%5", m_sZoneName, scoreGain, source, m_fActivityScore, m_fRequiredActivityScore));
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateActivity(IEntity owner, float timeSlice)
	{
		if (m_fActivityScore > 0)
			m_fActivityScore = Math.Max(m_fActivityScore - Math.Max(m_fActivityDecayPerSecond, 0.0) * timeSlice, 0.0);

		if (m_fActivityScore < m_fRequiredActivityScore)
			return;

		if (!CanFire(owner))
		{
			if (m_bDebug)
				Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: activity threshold reached, but required objective is offline", m_sZoneName));

			m_fActivityScore = Math.Max(m_fRequiredActivityScore * 0.5, 0.0);
			return;
		}

		StartBarrage(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void StartBarrage(IEntity owner)
	{
		m_bBarrageActive = true;
		m_bHasTriggered = true;
		m_iRoundsFired = 0;
		m_fActivityScore = 0;
		m_aRecentImpactPositions.Clear();
		m_fRoundDelay = RandomRangeSafe(m_fFirstRoundDelayMin, m_fFirstRoundDelayMax);

		if (m_bDebug)
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: mortar sequence started near %2, first impact in %3s", m_sZoneName, owner.GetOrigin().ToString(), m_fRoundDelay));
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateBarrage(IEntity owner, float timeSlice)
	{
		if (!CanFire(owner))
		{
			StopBarrage(true);
			return;
		}

		m_fRoundDelay -= timeSlice;
		if (m_fRoundDelay > 0)
			return;

		SpawnImpact(owner);
		m_iRoundsFired++;

		if (ShouldStopBarrage())
		{
			StopBarrage(false);
			return;
		}

		m_fRoundDelay = RandomRangeSafe(m_fRoundDelayMin, m_fRoundDelayMax);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ShouldStopBarrage()
	{
		if (m_iRoundsFired >= Math.Max(m_iMaxRounds, 1))
			return true;

		if (m_iRoundsFired < Math.Max(m_iMinRoundsBeforeStop, 1))
			return false;

		return Math.RandomFloat(0.0, 1.0) > Math.Clamp(m_fContinueChance, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopBarrage(bool objectiveWentOffline)
	{
		m_bBarrageActive = false;
		m_fRoundDelay = 0;
		m_fCooldownRemaining = Math.Max(m_fRetriggerCooldown, 0.0);

		if (m_bDebug)
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: mortar sequence stopped, rounds=%2, objectiveOffline=%3, cooldown=%4", m_sZoneName, m_iRoundsFired, objectiveWentOffline, m_fCooldownRemaining));
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnImpact(IEntity owner)
	{
		vector impactCenter = GetImpactCenter(owner);
		vector impactPos = impactCenter;
		int attempts = Math.Max(m_iImpactPositionAttempts, 1);
		float minSpacing = GetCurrentImpactMinSpacing();
		float minCenterDistance = GetCurrentCenterMissDistance();

		for (int i = 0; i < attempts; i++)
		{
			vector candidate = BuildRandomImpactPosition(owner, impactCenter);

			impactPos = candidate;
			if (IsImpactPositionAcceptable(candidate, impactCenter, minSpacing, minCenterDistance))
				break;
		}

		if (m_bSnapImpactToGround)
			impactPos = SnapPositionToGround(owner.GetWorld(), impactPos);

		RememberImpactPosition(impactPos);

		if (m_EffectResource && m_EffectResource.IsValid())
		{
			SpawnEffectImpact(owner, impactPos);
			return;
		}

		SpawnProjectileImpact(owner, impactPos);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnEffectImpact(IEntity owner, vector impactPos)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = impactPos;

		IEntity effectEntity = GetGame().SpawnEntityPrefab(m_EffectResource, owner.GetWorld(), spawnParams);
		if (!effectEntity)
		{
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: effect module spawn failed", m_sZoneName));
			return;
		}

		if (m_bDebug)
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: effect impact spawned at %2", m_sZoneName, impactPos.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnProjectileImpact(IEntity owner, vector impactPos)
	{
		if (!m_ProjectileResource || !m_ProjectileResource.IsValid())
		{
			if (!m_bWarnedNoImpactPrefab)
			{
				m_bWarnedNoImpactPrefab = true;
				Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: no valid projectile/effect prefab configured", m_sZoneName));
			}

			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = impactPos;

		IEntity projectile = GetGame().SpawnEntityPrefab(m_ProjectileResource, owner.GetWorld(), spawnParams);
		if (!projectile)
		{
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: projectile impact spawn failed", m_sZoneName));
			return;
		}

		BaseTriggerComponent triggerComponent = BaseTriggerComponent.Cast(projectile.FindComponent(BaseTriggerComponent));
		if (triggerComponent)
			triggerComponent.OnUserTrigger(projectile);

		if (m_bDebug)
			Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: projectile impact spawned at %2", m_sZoneName, impactPos.ToString()));
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
	protected bool IsPositionInside(vector position)
	{
		if (m_iActivityShape == SHAPE_BOX)
		{
			vector localPosition = CoordToLocal(position);
			return Math.AbsFloat(localPosition[0]) <= GetActivityHalfSizeX()
				&& Math.AbsFloat(localPosition[2]) <= GetActivityHalfSizeZ();
		}

		vector origin = GetOrigin();
		float dx = position[0] - origin[0];
		float dz = position[2] - origin[2];
		float radius = Math.Max(m_fActivityRadius, 1.0);
		return dx * dx + dz * dz <= radius * radius;
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetImpactCenter(IEntity owner)
	{
		if (!m_sImpactCenterEntityName.IsEmpty() && owner)
		{
			BaseWorld world = owner.GetWorld();
			if (world)
			{
				IEntity marker = world.FindEntityByName(m_sImpactCenterEntityName);
				if (marker)
					return marker.GetOrigin();
			}
		}

		return CoordToParent(Vector(m_fImpactCenterOffsetX, 0, m_fImpactCenterOffsetZ));
	}

	//------------------------------------------------------------------------------------------------
	protected float GetActivityHalfSizeX()
	{
		return Math.Max(m_fActivitySizeX * 0.5, 0.5);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetActivityHalfSizeZ()
	{
		return Math.Max(m_fActivitySizeZ * 0.5, 0.5);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetImpactHalfSizeX()
	{
		return Math.Max(m_fImpactSizeX * 0.5, 0.5);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetImpactHalfSizeZ()
	{
		return Math.Max(m_fImpactSizeZ * 0.5, 0.5);
	}

	//------------------------------------------------------------------------------------------------
	protected vector BuildRandomImpactPosition(IEntity owner, vector impactCenter)
	{
		float accuracyScale = GetCurrentAccuracyScale();
		vector impactPos = impactCenter;

		if (m_iImpactShape == SHAPE_BOX)
		{
			vector localImpactOffset = Vector(
				Math.RandomFloat(-GetImpactHalfSizeX() * accuracyScale, GetImpactHalfSizeX() * accuracyScale),
				0,
				Math.RandomFloat(-GetImpactHalfSizeZ() * accuracyScale, GetImpactHalfSizeZ() * accuracyScale)
			);

			return owner.CoordToParent(owner.CoordToLocal(impactCenter) + localImpactOffset);
		}

		float radius = Math.Max(m_fImpactRadius * accuracyScale, 1.0);
		impactPos[0] = impactPos[0] + Math.RandomFloat(-radius, radius);
		impactPos[2] = impactPos[2] + Math.RandomFloat(-radius, radius);
		return impactPos;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCurrentAccuracyScale()
	{
		if (!m_bProgressiveAccuracy)
			return 1.0;

		float finalScale = Math.Clamp(m_fFinalAccuracyScale, 0.05, 1.0);
		float progress = GetAccuracyProgress();
		return 1.0 + (finalScale - 1.0) * progress;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCurrentCenterMissDistance()
	{
		if (!m_bProgressiveAccuracy)
			return 0.0;

		return Math.Max(m_fEarlyCenterMissDistance, 0.0) * (1.0 - GetAccuracyProgress());
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCurrentImpactMinSpacing()
	{
		if (!m_bProgressiveAccuracy)
			return Math.Max(m_fFinalImpactMinSpacing, 0.0);

		float progress = GetAccuracyProgress();
		float earlySpacing = Math.Max(m_fEarlyImpactMinSpacing, 0.0);
		float finalSpacing = Math.Max(m_fFinalImpactMinSpacing, 0.0);
		return earlySpacing + (finalSpacing - earlySpacing) * progress;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetAccuracyProgress()
	{
		int accuracyRounds = Math.Max(m_iAccuracyRounds - 1, 1);
		return Math.Clamp((float)m_iRoundsFired / (float)accuracyRounds, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsImpactPositionAcceptable(vector candidate, vector impactCenter, float minSpacing, float minCenterDistance)
	{
		if (minCenterDistance > 0 && DistanceSqXZ(candidate, impactCenter) < minCenterDistance * minCenterDistance)
			return false;

		if (minSpacing <= 0)
			return true;

		foreach (vector previousImpact : m_aRecentImpactPositions)
		{
			if (DistanceSqXZ(candidate, previousImpact) < minSpacing * minSpacing)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void RememberImpactPosition(vector impactPos)
	{
		m_aRecentImpactPositions.Insert(impactPos);

		while (m_aRecentImpactPositions.Count() > 16)
			m_aRecentImpactPositions.Remove(0);
	}

	//------------------------------------------------------------------------------------------------
	protected float DistanceSqXZ(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadImpactResources()
	{
		m_bResourcesLoaded = true;

		if (m_sImpactEffectModulePrefab)
		{
			m_EffectResource = Resource.Load(m_sImpactEffectModulePrefab);
			if (!m_EffectResource || !m_EffectResource.IsValid())
				Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: failed to load effect prefab '%2'", m_sZoneName, m_sImpactEffectModulePrefab));
		}

		if ((!m_EffectResource || !m_EffectResource.IsValid()) && m_sImpactProjectilePrefab)
		{
			m_ProjectileResource = Resource.Load(m_sImpactProjectilePrefab);
			if (!m_ProjectileResource || !m_ProjectileResource.IsValid())
				Print(string.Format("LCN_CombatActivityMortarZoneEntity[%1]: failed to load projectile prefab '%2'", m_sZoneName, m_sImpactProjectilePrefab));
		}
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
	protected float RandomRangeSafe(float minValue, float maxValue)
	{
		float min = Math.Min(minValue, maxValue);
		float max = Math.Max(minValue, maxValue);
		return Math.RandomFloat(min, max);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ShouldIgnoreDuplicateProjectile(IEntity projectileEntity, BaseWorld world)
	{
		if (!projectileEntity)
			return false;

		if (!s_aRecentProjectiles)
			s_aRecentProjectiles = {};

		if (!s_aRecentProjectileTimes)
			s_aRecentProjectileTimes = {};

		float now = GetWorldTimeMs(world);

		for (int i = s_aRecentProjectiles.Count() - 1; i >= 0; i--)
		{
			IEntity recentProjectile = s_aRecentProjectiles[i];
			float recentTime = s_aRecentProjectileTimes[i];

			if (!recentProjectile || now - recentTime > PROJECTILE_DUPLICATE_WINDOW_MS)
			{
				s_aRecentProjectiles.Remove(i);
				s_aRecentProjectileTimes.Remove(i);
				continue;
			}

			if (recentProjectile == projectileEntity)
				return true;
		}

		s_aRecentProjectiles.Insert(projectileEntity);
		s_aRecentProjectileTimes.Insert(now);

		while (s_aRecentProjectiles.Count() > PROJECTILE_DEDUP_LIMIT)
		{
			s_aRecentProjectiles.Remove(0);
			s_aRecentProjectileTimes.Remove(0);
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static float GetWorldTimeMs(BaseWorld world)
	{
		if (!world)
			world = GetGame().GetWorld();

		if (!world)
			return 0;

		return world.GetWorldTime();
	}

	//------------------------------------------------------------------------------------------------
	protected static void RegisterZone(LCN_CombatActivityMortarZoneEntity zone)
	{
		array<LCN_CombatActivityMortarZoneEntity> zones = GetZoneRegistry();
		if (zones.Find(zone) == -1)
			zones.Insert(zone);
	}

	//------------------------------------------------------------------------------------------------
	protected static array<LCN_CombatActivityMortarZoneEntity> GetZoneRegistry()
	{
		if (!s_aZones)
			s_aZones = {};

		return s_aZones;
	}

	//------------------------------------------------------------------------------------------------
	void LCN_ConfigureExternalSquareBarrage(string zoneName, float zoneSize, ResourceName projectilePrefab, ResourceName effectModulePrefab, string requiredObjectiveKey, string requiredObjectiveKey2, string blockingObjectiveKey, bool debugEnabled)
	{
		if (!zoneName.IsEmpty())
			m_sZoneName = zoneName;

		m_iActivityShape = SHAPE_BOX;
		m_fActivitySizeX = zoneSize;
		m_fActivitySizeZ = zoneSize;
		m_fRequiredActivityScore = 999999.0;
		m_bAllowRetrigger = false;
		m_fRetriggerCooldown = 0;

		m_iImpactShape = SHAPE_BOX;
		m_fImpactSizeX = zoneSize;
		m_fImpactSizeZ = zoneSize;
		m_fImpactRadius = Math.Max(zoneSize * 0.5, 1.0);
		m_sRequiredObjectiveKey = requiredObjectiveKey;
		m_sRequiredObjectiveKey2 = requiredObjectiveKey2;
		m_sBlockingObjectiveKey = blockingObjectiveKey;
		m_bDebug = debugEnabled;

		if (projectilePrefab)
			m_sImpactProjectilePrefab = projectilePrefab;

		m_sImpactEffectModulePrefab = effectModulePrefab;
		m_bResourcesLoaded = false;
		m_bWarnedNoImpactPrefab = false;
		LoadImpactResources();
	}

	//------------------------------------------------------------------------------------------------
	void LCN_ConfigureExternalBarrageSchedule(float firstRoundDelayMin, float firstRoundDelayMax, float roundDelayMin, float roundDelayMax, int rounds, float continueChance)
	{
		m_fFirstRoundDelayMin = Math.Max(firstRoundDelayMin, 0.0);
		m_fFirstRoundDelayMax = Math.Max(firstRoundDelayMax, m_fFirstRoundDelayMin);
		m_fRoundDelayMin = Math.Max(roundDelayMin, 1.0);
		m_fRoundDelayMax = Math.Max(roundDelayMax, m_fRoundDelayMin);
		m_iMinRoundsBeforeStop = Math.Max(rounds, 1);
		m_iMaxRounds = Math.Max(rounds, 1);
		m_fContinueChance = Math.Clamp(continueChance, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	void LCN_ConfigureExternalProgressiveAccuracy(bool enabled, float finalAccuracyScale, int accuracyRounds, float earlyCenterMissDistance, float earlyImpactMinSpacing, float finalImpactMinSpacing, int impactPositionAttempts)
	{
		m_bProgressiveAccuracy = enabled;
		m_fFinalAccuracyScale = Math.Clamp(finalAccuracyScale, 0.05, 1.0);
		m_iAccuracyRounds = Math.Max(accuracyRounds, 1);
		m_fEarlyCenterMissDistance = Math.Max(earlyCenterMissDistance, 0.0);
		m_fEarlyImpactMinSpacing = Math.Max(earlyImpactMinSpacing, 0.0);
		m_fFinalImpactMinSpacing = Math.Max(finalImpactMinSpacing, 0.0);
		m_iImpactPositionAttempts = Math.Max(impactPositionAttempts, 1);
	}

	//------------------------------------------------------------------------------------------------
	bool LCN_TriggerExternalBarrage()
	{
		if (m_bBarrageActive)
			return false;

		if (!CanFire(this))
			return false;

		StartBarrage(this);
		return true;
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override void _WB_GetBoundBox(inout vector min, inout vector max, IEntitySource src)
	{
		if (m_iActivityShape == SHAPE_BOX)
		{
			min = Vector(-GetActivityHalfSizeX(), -1, -GetActivityHalfSizeZ());
			max = Vector(GetActivityHalfSizeX(), 1, GetActivityHalfSizeZ());
			return;
		}

		float radius = Math.Max(m_fActivityRadius, 1.0);
		min = Vector(-radius, -1, -radius);
		max = Vector(radius, 1, radius);
	}
#endif
}

modded class SCR_MuzzleEffectComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnFired(effectEntity, muzzle, projectileEntity);
		LCN_CombatActivityMortarZoneEntity.ReportWeaponFire(effectEntity, muzzle, projectileEntity);
	}
}

modded class SCR_OptimizedMuzzleEffectComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnFired(effectEntity, muzzle, projectileEntity);
		LCN_CombatActivityMortarZoneEntity.ReportWeaponFire(effectEntity, muzzle, projectileEntity);
	}
}
