[BaseContainerProps()]
class LCN_FallingShellData
{
	IEntity m_ShellEntity;
	vector m_vVelocity;
}

[EntityEditorProps(category: "LCN/Internal", description: "Internal helper for modular mortar barrages")]
class LCN_ModularMortarBarrageEntityClass : SCR_ExplosionGeneratorClass
{
}

class LCN_ModularMortarBarrageEntity : SCR_ExplosionGenerator
{
	protected vector m_vBarrageCenter;
	protected float m_fSpreadRadius;
	protected float m_fShellSpawnHeight;
	protected float m_fInitialShellSpeed;
	protected bool m_bConfigured;
	protected ref array<ref LCN_FallingShellData> m_aActiveShells = {};

	//------------------------------------------------------------------------------------------------
	void LCN_ModularMortarBarrageEntity(IEntitySource src, IEntity parent)
	{
		// SCR_ExplosionGenerator expects these arrays to exist during its own init path.
		if (!m_ProjectilesToTrigger)
			m_ProjectilesToTrigger = {};

		if (!m_LoadedPrefabs)
			m_LoadedPrefabs = {};
	}

	//------------------------------------------------------------------------------------------------
	void Configure(notnull array<ResourceName> projectilePrefabs, float timeBetweenExplosions, float totalDuration, float spreadRadius, float initialDelay = 0, float shellSpawnHeight = 120, float initialShellSpeed = 55)
	{
		if (!m_ProjectilesToTrigger)
			m_ProjectilesToTrigger = {};
		else
			m_ProjectilesToTrigger.Clear();

		foreach (ResourceName projectilePrefab : projectilePrefabs)
		{
			if (!projectilePrefab)
				continue;

			m_ProjectilesToTrigger.Insert(projectilePrefab);
		}

		if (!m_LoadedPrefabs)
			m_LoadedPrefabs = {};
		else
			m_LoadedPrefabs.Clear();

		foreach (ResourceName projectileResourceName : m_ProjectilesToTrigger)
		{
			Resource loadedResource = Resource.Load(projectileResourceName);
			if (loadedResource && loadedResource.IsValid())
				m_LoadedPrefabs.Insert(loadedResource);
		}

		m_NumExplosions = 0;
		m_TimeBetweenExplosions = timeBetweenExplosions;
		m_TotalDuration = totalDuration;
		m_RemainingExplosions = 0;
		m_TimeUntilNextExplosion = initialDelay;
		m_RemainingDuration = totalDuration;
		m_CurrentExplosionPrefab = 0;
		m_fSpreadRadius = spreadRadius;
		m_fShellSpawnHeight = shellSpawnHeight;
		m_fInitialShellSpeed = initialShellSpeed;
		m_vBarrageCenter = GetOrigin();
		m_bConfigured = true;

		SetEventMask(EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bConfigured)
		{
			ClearEventMask(EntityEvent.FRAME);
			return;
		}

		UpdateFallingShells(owner, timeSlice);

		if (m_NumExplosions == 0 && m_RemainingDuration < 0 || m_NumExplosions != 0 && m_RemainingExplosions <= 0)
		{
			ClearEventMask(EntityEvent.FRAME);
			return;
		}

		m_TimeUntilNextExplosion -= timeSlice;
		m_RemainingDuration -= timeSlice;

		if (m_TimeUntilNextExplosion > 0)
			return;

		m_TimeUntilNextExplosion += m_TimeBetweenExplosions;
		MoveToRandomImpactPoint(owner);
		CreateExplosion(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFallingShells(IEntity owner, float timeSlice)
	{
		vector gravity = PhysicsWorld.GetGravity(GetGame().GetWorldEntity());

		for (int i = m_aActiveShells.Count() - 1; i >= 0; i--)
		{
			LCN_FallingShellData shellData = m_aActiveShells[i];
			if (!shellData || !shellData.m_ShellEntity)
			{
				m_aActiveShells.Remove(i);
				continue;
			}

			vector startPos = shellData.m_ShellEntity.GetOrigin();
			shellData.m_vVelocity = shellData.m_vVelocity + gravity * timeSlice;
			vector endPos = startPos + shellData.m_vVelocity * timeSlice;

			autoptr TraceParam trace = new TraceParam();
			trace.Start = startPos;
			trace.End = endPos;
			trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			trace.Exclude = shellData.m_ShellEntity;

			float traced = owner.GetWorld().TraceMove(trace, null);
			if (traced < 1)
			{
				vector hitPos = startPos + (endPos - startPos) * traced;
				shellData.m_ShellEntity.SetOrigin(hitPos);
				TriggerShell(shellData.m_ShellEntity);
				m_aActiveShells.Remove(i);
				continue;
			}

			shellData.m_ShellEntity.SetOrigin(endPos);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void MoveToRandomImpactPoint(IEntity owner)
	{
		vector randomPoint = m_vBarrageCenter;
		randomPoint[0] = randomPoint[0] + Math.RandomFloat(-m_fSpreadRadius, m_fSpreadRadius);
		randomPoint[2] = randomPoint[2] + Math.RandomFloat(-m_fSpreadRadius, m_fSpreadRadius);
		owner.SetOrigin(randomPoint);
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateExplosion(IEntity owner)
	{
		ref Resource prefab = m_LoadedPrefabs[m_CurrentExplosionPrefab];
		AdvanceExplosionPrefab();
		m_RemainingExplosions--;

		if (!prefab)
			return;

		vector shellMat[4];
		owner.GetTransform(shellMat);
		shellMat[3][1] = shellMat[3][1] + m_fShellSpawnHeight;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		for (int i = 0; i < 4; i++)
			spawnParams.Transform[i] = shellMat[i];

		IEntity shellEntity = GetGame().SpawnEntityPrefab(prefab, owner.GetWorld(), spawnParams);
		if (!shellEntity)
			return;

		ref LCN_FallingShellData shellData = new LCN_FallingShellData();
		shellData.m_ShellEntity = shellEntity;
		shellData.m_vVelocity = Vector(0, -m_fInitialShellSpeed, 0);
		m_aActiveShells.Insert(shellData);
	}

	//------------------------------------------------------------------------------------------------
	protected void TriggerShell(IEntity shellEntity)
	{
		BaseTriggerComponent triggerComponent = BaseTriggerComponent.Cast(shellEntity.FindComponent(BaseTriggerComponent));
		if (triggerComponent)
			triggerComponent.OnUserTrigger(shellEntity);
	}
}
