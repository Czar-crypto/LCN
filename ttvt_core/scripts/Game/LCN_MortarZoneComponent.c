[ComponentEditorProps(category: "LCN/Triggers", description: "Starts a timed mortar barrage when someone remains inside the zone")]
class LCN_MortarZoneComponentClass : ScriptComponentClass
{
}

class LCN_MortarZoneComponent : ScriptComponent
{
	[Attribute("25", UIWidgets.EditBox, "Zone radius in meters", params: "1 500 1", category: "Trigger")]
	protected float m_fTriggerRadius;

	[Attribute("30", UIWidgets.EditBox, "Seconds a character must stay in the zone before the barrage starts", params: "1 1800 1", category: "Trigger")]
	protected float m_fRequiredPresenceTime;

	[Attribute("0.25", UIWidgets.EditBox, "How often the zone is scanned", params: "0.05 5 0.05", category: "Trigger")]
	protected float m_fCheckPeriod;

	[Attribute("300", UIWidgets.EditBox, "How long the barrage lasts in seconds", params: "1 1800 1", category: "Barrage")]
	protected float m_fBarrageDuration;

	[Attribute("3.5", UIWidgets.EditBox, "Delay between impacts in seconds", params: "0.1 60 0.1", category: "Barrage")]
	protected float m_fRoundInterval;

	[Attribute("35", UIWidgets.EditBox, "Random impact radius around the zone center", params: "1 500 1", category: "Barrage")]
	protected float m_fImpactRadius;

	[Attribute("{E15B8A4A6D904A2E}Prefabs/Weapons/Projectiles/Mortar/Ammo_Shell_82mm_HE_O832DU.et", desc: "Projectile prefab that will be triggered at each impact point", category: "Barrage")]
	protected ResourceName m_sProjectilePrefab;

	[Attribute("0", UIWidgets.CheckBox, "Allow the barrage to trigger again after it has finished", category: "Barrage")]
	protected bool m_bAllowRetrigger;

	protected ref array<IEntity> m_aNearbyCharacters = {};
	protected ref array<IEntity> m_aTrackedCharacters = {};
	protected ref array<float> m_aTrackedDurations = {};

	protected ref Resource m_ProjectileResource;

	protected float m_fCheckDelay;
	protected float m_fRoundDelay;
	protected float m_fBarrageTimeRemaining;
	protected bool m_bBarrageActive;
	protected bool m_bHasTriggered;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Trigger evaluation and projectile spawning should only happen on the authority.
		if (RplSession.Mode() == RplMode.Client)
			return;

		if (m_sProjectilePrefab)
		{
			m_ProjectileResource = Resource.Load(m_sProjectilePrefab);

			if (!m_ProjectileResource || !m_ProjectileResource.IsValid())
				Print(string.Format("LCN_MortarZoneComponent: failed to load projectile prefab '%1'", m_sProjectilePrefab));
		}

		m_fCheckDelay = 0;
		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (m_bBarrageActive)
			UpdateBarrage(owner, timeSlice);

		m_fCheckDelay -= timeSlice;
		if (m_fCheckDelay > 0)
			return;

		m_fCheckDelay = m_fCheckPeriod;
		ScanZone(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScanZone(IEntity owner)
	{
		m_aNearbyCharacters.Clear();
		owner.GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), m_fTriggerRadius, QueryEntitiesCallback, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		array<IEntity> nextTracked = {};
		array<float> nextDurations = {};

		foreach (IEntity character : m_aNearbyCharacters)
		{
			float duration = m_fCheckPeriod;
			int existingIndex = m_aTrackedCharacters.Find(character);
			if (existingIndex != -1)
				duration = m_aTrackedDurations[existingIndex] + m_fCheckPeriod;

			nextTracked.Insert(character);
			nextDurations.Insert(duration);

			if (!m_bHasTriggered && duration >= m_fRequiredPresenceTime)
				StartBarrage();
		}

		m_aTrackedCharacters.Clear();
		m_aTrackedDurations.Clear();

		foreach (IEntity trackedCharacter : nextTracked)
			m_aTrackedCharacters.Insert(trackedCharacter);

		foreach (float trackedDuration : nextDurations)
			m_aTrackedDurations.Insert(trackedDuration);

		if (!m_bBarrageActive && m_bAllowRetrigger && m_aTrackedCharacters.IsEmpty())
			m_bHasTriggered = false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool QueryEntitiesCallback(IEntity entity)
	{
		if (!entity)
			return true;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return true;

		m_aNearbyCharacters.Insert(entity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void StartBarrage()
	{
		m_bHasTriggered = true;
		m_bBarrageActive = true;
		m_fBarrageTimeRemaining = m_fBarrageDuration;
		m_fRoundDelay = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateBarrage(IEntity owner, float timeSlice)
	{
		m_fBarrageTimeRemaining -= timeSlice;
		m_fRoundDelay -= timeSlice;

		while (m_fRoundDelay <= 0 && m_fBarrageTimeRemaining > 0)
		{
			SpawnImpact(owner);
			m_fRoundDelay += m_fRoundInterval;
		}

		if (m_fBarrageTimeRemaining > 0)
			return;

		m_bBarrageActive = false;
		m_fRoundDelay = 0;

		if (!m_bAllowRetrigger)
			return;

		if (m_aTrackedCharacters.IsEmpty())
			m_bHasTriggered = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnImpact(IEntity owner)
	{
		if (!m_ProjectileResource || !m_ProjectileResource.IsValid())
			return;

		vector impactPos = owner.GetOrigin();
		impactPos[0] = impactPos[0] + Math.RandomFloat(-m_fImpactRadius, m_fImpactRadius);
		impactPos[2] = impactPos[2] + Math.RandomFloat(-m_fImpactRadius, m_fImpactRadius);

		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = impactPos;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		for (int i = 0; i < 4; i++)
			spawnParams.Transform[i] = mat[i];

		IEntity projectile = GetGame().SpawnEntityPrefab(m_ProjectileResource, owner.GetWorld(), spawnParams);
		if (!projectile)
			return;

		BaseTriggerComponent triggerComponent = BaseTriggerComponent.Cast(projectile.FindComponent(BaseTriggerComponent));
		if (triggerComponent)
			triggerComponent.OnUserTrigger(projectile);
	}
}
