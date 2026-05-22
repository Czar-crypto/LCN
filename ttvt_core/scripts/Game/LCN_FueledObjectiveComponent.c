[ComponentEditorProps(category: "LCN/Objectives", description: "Consumes simple mission fuel and controls an LCN objective key")]
class LCN_FueledObjectiveComponentClass : ScriptComponentClass
{
}

class LCN_FueledObjectiveComponent : ScriptComponent
{
	protected static ref array<LCN_FueledObjectiveComponent> s_aFueledObjectives;

	[Attribute("Gen_1", UIWidgets.EditBox, "LCN objective key controlled by this fueled object", category: "LCN Fuel")]
	protected string m_sObjectiveKey;

	[Attribute("1", UIWidgets.CheckBox, "Copy Objective Key to the owner's LCN_ObjectiveStateComponent on init", category: "LCN Fuel")]
	protected bool m_bApplyObjectiveKeyToOwnerObjective;

	[Attribute("600", UIWidgets.EditBox, "Maximum fuel time in seconds", params: "1 7200 1", category: "LCN Fuel")]
	protected float m_fMaxFuelSeconds;

	[Attribute("600", UIWidgets.EditBox, "Fuel time at mission start in seconds", params: "0 7200 1", category: "LCN Fuel")]
	protected float m_fStartFuelSeconds;

	[Attribute("1", UIWidgets.EditBox, "Fuel seconds consumed per real second while the objective is active", params: "0 60 0.1", category: "LCN Fuel")]
	protected float m_fFuelDrainPerSecond;

	[Attribute("1", UIWidgets.CheckBox, "Disable linked objective when fuel reaches zero", category: "LCN Fuel")]
	protected bool m_bDisableObjectiveWhenEmpty;

	[Attribute("1", UIWidgets.CheckBox, "Print fuel state changes to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected IEntity m_Owner;
	protected LCN_ObjectiveStateComponent m_ObjectiveState;
	protected float m_fFuelSyncDelay;

	[RplProp()]
	protected float m_fCurrentFuelSeconds;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Owner = owner;
		m_ObjectiveState = LCN_ObjectiveStateComponent.Cast(owner.FindComponent(LCN_ObjectiveStateComponent));

		if (m_bApplyObjectiveKeyToOwnerObjective && m_ObjectiveState && !m_sObjectiveKey.IsEmpty())
			m_ObjectiveState.SetObjectiveKey(m_sObjectiveKey);

		m_fMaxFuelSeconds = Math.Max(m_fMaxFuelSeconds, 1.0);
		m_fCurrentFuelSeconds = Math.Clamp(m_fStartFuelSeconds, 0.0, m_fMaxFuelSeconds);
		m_fFuelSyncDelay = 0;

		array<LCN_FueledObjectiveComponent> fueledObjectives = GetFueledObjectiveRegistry();
		if (fueledObjectives.Find(this) == -1)
			fueledObjectives.Insert(this);

		SetEventMask(owner, EntityEvent.FRAME);

		if (m_fCurrentFuelSeconds <= 0 && m_bDisableObjectiveWhenEmpty)
			SetLinkedObjectiveActive(false);

		if (m_bDebug)
			Print(string.Format("LCN_FueledObjectiveComponent: registered '%1' fuel=%2/%3", m_sObjectiveKey, m_fCurrentFuelSeconds, m_fMaxFuelSeconds));
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (s_aFueledObjectives)
			s_aFueledObjectives.RemoveItem(this);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!IsMaster())
			return;

		if (m_fFuelDrainPerSecond <= 0)
			return;

		if (!IsLinkedObjectiveActive())
			return;

		if (!IsOwnerAlive())
			return;

		float oldFuel = m_fCurrentFuelSeconds;
		m_fCurrentFuelSeconds = Math.Clamp(m_fCurrentFuelSeconds - (timeSlice * m_fFuelDrainPerSecond), 0.0, m_fMaxFuelSeconds);

		if (oldFuel > 0 && m_fCurrentFuelSeconds <= 0)
		{
			if (m_bDisableObjectiveWhenEmpty)
				SetLinkedObjectiveActive(false);

			Replication.BumpMe();

			if (m_bDebug)
				Print(string.Format("LCN_FueledObjectiveComponent: '%1' fuel empty, objective disabled", m_sObjectiveKey));

			return;
		}

		m_fFuelSyncDelay -= timeSlice;
		if (m_fFuelSyncDelay <= 0)
		{
			m_fFuelSyncDelay = 5.0;
			Replication.BumpMe();
		}
	}

	//------------------------------------------------------------------------------------------------
	static LCN_FueledObjectiveComponent FindFueledObjective(string objectiveKey, BaseWorld world = null)
	{
		if (objectiveKey.IsEmpty())
			return null;

		array<LCN_FueledObjectiveComponent> fueledObjectives = GetFueledObjectiveRegistry();
		foreach (LCN_FueledObjectiveComponent fueledObjective : fueledObjectives)
		{
			if (!fueledObjective)
				continue;

			if (fueledObjective.GetObjectiveKey() != objectiveKey)
				continue;

			IEntity owner = fueledObjective.GetOwnerEntity();
			if (!owner)
				continue;

			if (world && owner.GetWorld() != world)
				continue;

			return fueledObjective;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	string GetObjectiveKey()
	{
		return m_sObjectiveKey;
	}

	//------------------------------------------------------------------------------------------------
	IEntity GetOwnerEntity()
	{
		return m_Owner;
	}

	//------------------------------------------------------------------------------------------------
	float GetFuelSeconds()
	{
		return m_fCurrentFuelSeconds;
	}

	//------------------------------------------------------------------------------------------------
	float GetMaxFuelSeconds()
	{
		return m_fMaxFuelSeconds;
	}

	//------------------------------------------------------------------------------------------------
	float GetFuelPercent()
	{
		if (m_fMaxFuelSeconds <= 0)
			return 0;

		return Math.Clamp(m_fCurrentFuelSeconds / m_fMaxFuelSeconds, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	bool IsFuelFull()
	{
		return m_fCurrentFuelSeconds >= m_fMaxFuelSeconds;
	}

	//------------------------------------------------------------------------------------------------
	bool CanAcceptFuel()
	{
		return IsOwnerAlive() && !IsFuelFull();
	}

	//------------------------------------------------------------------------------------------------
	bool CanStartObjective()
	{
		return IsOwnerAlive() && m_fCurrentFuelSeconds > 0 && !IsLinkedObjectiveActive();
	}

	//------------------------------------------------------------------------------------------------
	bool IsRunning()
	{
		return IsLinkedObjectiveActive();
	}

	//------------------------------------------------------------------------------------------------
	float AddFuel(float fuelSeconds, bool startAfterRefuel)
	{
		if (!CanAcceptFuel())
			return 0;

		float oldFuel = m_fCurrentFuelSeconds;
		m_fCurrentFuelSeconds = Math.Clamp(m_fCurrentFuelSeconds + Math.Max(fuelSeconds, 0.0), 0.0, m_fMaxFuelSeconds);
		float addedFuel = m_fCurrentFuelSeconds - oldFuel;

		if (startAfterRefuel && m_fCurrentFuelSeconds > 0)
			SetLinkedObjectiveActive(true);

		Replication.BumpMe();

		if (m_bDebug)
			Print(string.Format("LCN_FueledObjectiveComponent: '%1' refueled +%2s, fuel=%3/%4, running=%5", m_sObjectiveKey, addedFuel, m_fCurrentFuelSeconds, m_fMaxFuelSeconds, IsLinkedObjectiveActive()));

		return addedFuel;
	}

	//------------------------------------------------------------------------------------------------
	bool StartObjective()
	{
		if (!CanStartObjective())
			return false;

		SetLinkedObjectiveActive(true);
		Replication.BumpMe();

		if (m_bDebug)
			Print(string.Format("LCN_FueledObjectiveComponent: '%1' started with fuel=%2/%3", m_sObjectiveKey, m_fCurrentFuelSeconds, m_fMaxFuelSeconds));

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsLinkedObjectiveActive()
	{
		LCN_ObjectiveStateComponent objective = ResolveOwnerObjective();
		if (objective)
			return objective.IsObjectiveActive();

		BaseWorld world = null;
		if (m_Owner)
			world = m_Owner.GetWorld();

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sObjectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetLinkedObjectiveActive(bool active)
	{
		BaseWorld world = null;
		if (m_Owner)
			world = m_Owner.GetWorld();

		int count = LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sObjectiveKey, active, world);
		if (count > 0)
			return;

		LCN_ObjectiveStateComponent objective = ResolveOwnerObjective();
		if (objective)
			objective.SetObjectiveActive(active);
	}

	//------------------------------------------------------------------------------------------------
	protected LCN_ObjectiveStateComponent ResolveOwnerObjective()
	{
		if (m_ObjectiveState)
			return m_ObjectiveState;

		if (!m_Owner)
			return null;

		m_ObjectiveState = LCN_ObjectiveStateComponent.Cast(m_Owner.FindComponent(LCN_ObjectiveStateComponent));
		return m_ObjectiveState;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsOwnerAlive()
	{
		if (!m_Owner)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(m_Owner);
		if (damageManager)
		{
			if (damageManager.IsDestroyed())
				return false;

			if (damageManager.GetState() == EDamageState.DESTROYED)
				return false;
		}

		return true;
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
	protected static array<LCN_FueledObjectiveComponent> GetFueledObjectiveRegistry()
	{
		if (!s_aFueledObjectives)
			s_aFueledObjectives = new array<LCN_FueledObjectiveComponent>();

		return s_aFueledObjectives;
	}
}
