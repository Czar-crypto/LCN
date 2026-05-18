[ComponentEditorProps(category: "LCN/Alarm", description: "Plays siren feedback while a base alarm objective is active")]
class LCN_BaseAlarmSirenComponentClass : ScriptComponentClass
{
}

class LCN_BaseAlarmSirenComponent : ScriptComponent
{
	[Attribute("LCN_BASE_ALARM_01", UIWidgets.EditBox, "LCN alarm objective key watched by this siren", category: "LCN Alarm")]
	protected string m_sAlarmObjectiveKey;

	[Attribute("LCN_GENERATOR_01", UIWidgets.EditBox, "Optional objective key required to keep the alarm valid", category: "LCN Alarm")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("300", UIWidgets.EditBox, "Alarm duration in seconds. Set 0 to keep alarm active until another script disables it", params: "0 3600 1", category: "LCN Alarm")]
	protected float m_fAlarmDuration;

	[Attribute("SOUND_SIREN_ALARM", UIWidgets.EditBox, "Sound event name to call on the owner's SoundComponent", category: "LCN Sound")]
	protected string m_sSoundEventName;

	[Attribute("8", UIWidgets.EditBox, "How often the sound event is retriggered while the alarm is active", params: "1 120 1", category: "LCN Sound")]
	protected float m_fSoundRetriggerPeriod;

	[Attribute("1", UIWidgets.CheckBox, "Print alarm state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected bool m_bWasAlarmActive;
	protected bool m_bWarnedMissingSound;
	protected float m_fSoundDelay;
	protected float m_fRemainingDuration;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		bool alarmActive = IsAlarmActive(owner);
		if (alarmActive && !m_bWasAlarmActive)
		{
			m_fSoundDelay = 0;
			m_fRemainingDuration = m_fAlarmDuration;
			if (m_bDebug)
				Print(string.Format("LCN_BaseAlarmSirenComponent: alarm '%1' started", m_sAlarmObjectiveKey));
		}
		else if (!alarmActive && m_bWasAlarmActive)
		{
			if (m_bDebug)
				Print(string.Format("LCN_BaseAlarmSirenComponent: alarm '%1' stopped", m_sAlarmObjectiveKey));
		}

		m_bWasAlarmActive = alarmActive;
		if (!alarmActive)
			return;

		UpdateAlarmDuration(owner, timeSlice);
		UpdateSirenSound(owner, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsAlarmActive(IEntity owner)
	{
		if (m_sAlarmObjectiveKey.IsEmpty())
			return false;

		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		if (!m_sRequiredObjectiveKey.IsEmpty() && !LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sRequiredObjectiveKey, world))
		{
			if (IsMaster())
				LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sAlarmObjectiveKey, false, world);

			return false;
		}

		return LCN_ObjectiveStateComponent.IsObjectiveKeyActive(m_sAlarmObjectiveKey, world);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateAlarmDuration(IEntity owner, float timeSlice)
	{
		if (m_fAlarmDuration <= 0)
			return;

		if (!IsMaster())
			return;

		m_fRemainingDuration -= timeSlice;
		if (m_fRemainingDuration > 0)
			return;

		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sAlarmObjectiveKey, false, world);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSirenSound(IEntity owner, float timeSlice)
	{
		m_fSoundDelay -= timeSlice;
		if (m_fSoundDelay > 0)
			return;

		m_fSoundDelay = Math.Max(m_fSoundRetriggerPeriod, 1);
		PlaySirenSound(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void PlaySirenSound(IEntity owner)
	{
		if (!owner)
			return;

		if (m_sSoundEventName.IsEmpty())
			return;

		SoundComponent soundComponent = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (!soundComponent)
		{
			if (m_bDebug && !m_bWarnedMissingSound)
			{
				Print("LCN_BaseAlarmSirenComponent: no SoundComponent on siren. Add an ACP-backed SoundComponent to hear the siren.");
				m_bWarnedMissingSound = true;
			}
			return;
		}

		soundComponent.SoundEvent(m_sSoundEventName);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMaster()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			return gameMode.IsMaster();

		return Replication.IsServer();
	}
}
