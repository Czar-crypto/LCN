[ComponentEditorProps(category: "LCN/Objectives", description: "Tracks whether a mission object is active, disabled, or destroyed")]
class LCN_ObjectiveStateComponentClass : ScriptComponentClass
{
}

class LCN_ObjectiveStateComponent : ScriptComponent
{
	protected static ref array<LCN_ObjectiveStateComponent> s_aObjectives;

	[Attribute("", UIWidgets.EditBox, "Logical key used by other LCN systems, for example LCN_COMMS_01", category: "LCN Objective")]
	protected string m_sObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Optional objective key that must also be active", category: "LCN Objective")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("1", UIWidgets.CheckBox, "Objective starts active", category: "LCN Objective")]
	protected bool m_bStartActive;

	[Attribute("1", UIWidgets.CheckBox, "Destroyed owner counts as inactive", category: "LCN Objective")]
	protected bool m_bDestroyedMeansInactive;

	[Attribute("0", UIWidgets.EditBox, "If above 0, objective becomes inactive when owner health scale is equal or below this value", params: "0 1 0.01", category: "LCN Objective")]
	protected float m_fInactiveHealthScaledThreshold;

	[Attribute("1", UIWidgets.CheckBox, "Print state changes to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected IEntity m_Owner;

	[RplProp()]
	protected bool m_bActive;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Owner = owner;
		m_bActive = m_bStartActive;

		array<LCN_ObjectiveStateComponent> objectives = GetObjectiveRegistry();
		if (objectives.Find(this) == -1)
			objectives.Insert(this);

		if (m_bDebug && !m_sObjectiveKey.IsEmpty())
			Print(string.Format("LCN_ObjectiveStateComponent: registered '%1' active=%2", m_sObjectiveKey, m_bActive));
	}

	//------------------------------------------------------------------------------------------------
	static LCN_ObjectiveStateComponent FindObjective(string objectiveKey, BaseWorld world = null)
	{
		if (objectiveKey.IsEmpty())
			return null;

		array<LCN_ObjectiveStateComponent> objectives = GetObjectiveRegistry();
		foreach (LCN_ObjectiveStateComponent objective : objectives)
		{
			if (!objective)
				continue;

			if (objective.GetObjectiveKey() != objectiveKey)
				continue;

			IEntity owner = objective.GetOwnerEntity();
			if (!owner)
				continue;

			if (world && owner.GetWorld() != world)
				continue;

			return objective;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsObjectiveKeyActive(string objectiveKey, BaseWorld world = null)
	{
		if (objectiveKey.IsEmpty())
			return false;

		array<LCN_ObjectiveStateComponent> objectives = GetObjectiveRegistry();
		foreach (LCN_ObjectiveStateComponent objective : objectives)
		{
			if (!objective)
				continue;

			if (objective.GetObjectiveKey() != objectiveKey)
				continue;

			IEntity owner = objective.GetOwnerEntity();
			if (!owner)
				continue;

			if (world && owner.GetWorld() != world)
				continue;

			if (objective.IsObjectiveActive())
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static int SetObjectiveKeyActive(string objectiveKey, bool active, BaseWorld world = null)
	{
		if (objectiveKey.IsEmpty())
			return 0;

		int count = 0;
		array<LCN_ObjectiveStateComponent> objectives = GetObjectiveRegistry();
		foreach (LCN_ObjectiveStateComponent objective : objectives)
		{
			if (!objective)
				continue;

			if (objective.GetObjectiveKey() != objectiveKey)
				continue;

			IEntity owner = objective.GetOwnerEntity();
			if (!owner)
				continue;

			if (world && owner.GetWorld() != world)
				continue;

			objective.SetObjectiveActive(active);
			count++;
		}

		return count;
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
	bool IsObjectiveActive()
	{
		if (!m_bActive)
			return false;

		if (m_bDestroyedMeansInactive && !IsEntityAlive(m_Owner))
			return false;

		if (!m_sRequiredObjectiveKey.IsEmpty())
		{
			BaseWorld world = null;
			if (m_Owner)
				world = m_Owner.GetWorld();

			if (!IsObjectiveKeyActive(m_sRequiredObjectiveKey, world))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void SetObjectiveActive(bool active)
	{
		if (m_bActive == active)
			return;

		m_bActive = active;
		Replication.BumpMe();

		if (m_bDebug)
			Print(string.Format("LCN_ObjectiveStateComponent: '%1' active=%2", m_sObjectiveKey, m_bActive));
	}

	//------------------------------------------------------------------------------------------------
	void DisableObjective()
	{
		SetObjectiveActive(false);
	}

	//------------------------------------------------------------------------------------------------
	void EnableObjective()
	{
		SetObjectiveActive(true);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsEntityAlive(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (damageManager)
		{
			if (damageManager.IsDestroyed())
				return false;

			if (damageManager.GetState() == EDamageState.DESTROYED)
				return false;

			if (m_fInactiveHealthScaledThreshold > 0 && damageManager.GetHealthScaled() <= m_fInactiveHealthScaledThreshold)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static array<LCN_ObjectiveStateComponent> GetObjectiveRegistry()
	{
		if (!s_aObjectives)
			s_aObjectives = new array<LCN_ObjectiveStateComponent>();

		return s_aObjectives;
	}
}
