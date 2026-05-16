[EntityEditorProps(category: "LCN/Internal", description: "Internal helper for modular mortar barrages")]
class LCN_ModularMortarBarrageEntityClass : SCR_ExplosionGeneratorClass
{
}

class LCN_ModularMortarBarrageEntity : SCR_ExplosionGenerator
{
	protected vector m_vBarrageCenter;
	protected float m_fSpreadRadius;
	protected bool m_bConfigured;

	//------------------------------------------------------------------------------------------------
	void Configure(notnull array<ResourceName> projectilePrefabs, float timeBetweenExplosions, float totalDuration, float spreadRadius, float initialDelay = 0)
	{
		m_ProjectilesToTrigger = {};
		foreach (ResourceName projectilePrefab : projectilePrefabs)
		{
			if (!projectilePrefab)
				continue;

			m_ProjectilesToTrigger.Insert(projectilePrefab);
		}

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
	protected void MoveToRandomImpactPoint(IEntity owner)
	{
		vector randomPoint = m_vBarrageCenter;
		randomPoint[0] = randomPoint[0] + Math.RandomFloat(-m_fSpreadRadius, m_fSpreadRadius);
		randomPoint[2] = randomPoint[2] + Math.RandomFloat(-m_fSpreadRadius, m_fSpreadRadius);
		owner.SetOrigin(randomPoint);
	}
}
