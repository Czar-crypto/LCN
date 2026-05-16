[BaseContainerProps()]
class LCN_FallingShellData
{
	IEntity m_ShellEntity;
	vector m_vVelocity;
}

[EntityEditorProps(category: "LCN/Internal", description: "Internal helper for modular mortar barrages")]
class LCN_ModularMortarBarrageEntityClass : GenericEntityClass
{
}

class LCN_ModularMortarBarrageEntity : GenericEntity
{
	protected ref array<ResourceName> m_aProjectilePrefabs = {};
	protected ref array<ref Resource> m_aLoadedPrefabs = {};
	protected ref array<ref LCN_FallingShellData> m_aActiveShells = {};

	protected vector m_vBarrageCenter;
	protected float m_fSpreadRadius;
	protected float m_fShellSpawnHeight;
	protected float m_fInitialShellSpeed;
	protected float m_fTimeBetweenShots;
	protected float m_fTimeUntilNextShot;
	protected float m_fRemainingDuration;
	protected int m_iCurrentProjectileIndex;
	protected bool m_bConfigured;

	//------------------------------------------------------------------------------------------------
	void Configure(notnull array<ResourceName> projectilePrefabs, float timeBetweenExplosions, float totalDuration, float spreadRadius, float initialDelay = 0, float shellSpawnHeight = 120, float initialShellSpeed = 55)
	{
		m_aProjectilePrefabs.Clear();
		foreach (ResourceName projectilePrefab : projectilePrefabs)
		{
			if (!projectilePrefab)
				continue;

			m_aProjectilePrefabs.Insert(projectilePrefab);
		}

		m_aLoadedPrefabs.Clear();
		foreach (ResourceName projectileResourceName : m_aProjectilePrefabs)
		{
			Resource loadedResource = Resource.Load(projectileResourceName);
			if (loadedResource && loadedResource.IsValid())
				m_aLoadedPrefabs.Insert(loadedResource);
		}

		m_fTimeBetweenShots = timeBetweenExplosions;
		m_fTimeUntilNextShot = initialDelay;
		m_fRemainingDuration = totalDuration;
		m_fSpreadRadius = spreadRadius;
		m_fShellSpawnHeight = shellSpawnHeight;
		m_fInitialShellSpeed = initialShellSpeed;
		m_iCurrentProjectileIndex = 0;
		m_vBarrageCenter = GetOrigin();
		m_bConfigured = !m_aLoadedPrefabs.IsEmpty();

		if (!m_bConfigured)
		{
			Print("LCN_ModularMortarBarrageEntity: no valid loaded projectile prefabs");
			return;
		}

		SetEventMask(EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bConfigured)
		{
			ClearEventMask(EntityEvent.FRAME);
			return;
		}

		UpdateFallingShells(owner, timeSlice);

		m_fRemainingDuration -= timeSlice;
		m_fTimeUntilNextShot -= timeSlice;

		if (m_fRemainingDuration <= 0)
		{
			if (m_aActiveShells.IsEmpty())
				ClearEventMask(EntityEvent.FRAME);

			return;
		}

		if (m_fTimeUntilNextShot > 0)
			return;

		m_fTimeUntilNextShot += m_fTimeBetweenShots;
		SpawnShell(owner);
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
	protected void SpawnShell(IEntity owner)
	{
		if (m_aLoadedPrefabs.IsEmpty())
			return;

		ref Resource prefab = m_aLoadedPrefabs[m_iCurrentProjectileIndex];
		m_iCurrentProjectileIndex++;
		if (m_iCurrentProjectileIndex >= m_aLoadedPrefabs.Count())
			m_iCurrentProjectileIndex = 0;

		if (!prefab)
			return;

		vector spawnPos = m_vBarrageCenter;
		spawnPos[0] = spawnPos[0] + Math.RandomFloat(-m_fSpreadRadius, m_fSpreadRadius);
		spawnPos[2] = spawnPos[2] + Math.RandomFloat(-m_fSpreadRadius, m_fSpreadRadius);
		spawnPos[1] = spawnPos[1] + m_fShellSpawnHeight;

		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = spawnPos;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		for (int i = 0; i < 4; i++)
			spawnParams.Transform[i] = transform[i];

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
