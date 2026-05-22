[ComponentEditorProps(category: "LCN/Alarm", description: "Plays siren feedback while a base alarm objective is active")]
class LCN_BaseAlarmSirenComponentClass : ScriptComponentClass
{
}

class LCN_BaseAlarmSirenComponent : ScriptComponent
{
	[Attribute("LCN_BASE_ALARM_01", UIWidgets.EditBox, "LCN alarm objective key watched by this siren", category: "LCN Alarm")]
	protected string m_sAlarmObjectiveKey;

	[Attribute("", UIWidgets.EditBox, "Optional objective key required to keep the alarm valid", category: "LCN Alarm")]
	protected string m_sRequiredObjectiveKey;

	[Attribute("300", UIWidgets.EditBox, "Alarm duration in seconds. Set 0 to keep alarm active until another script disables it", params: "0 3600 1", category: "LCN Alarm")]
	protected float m_fAlarmDuration;

	[Attribute("SOUND_SIREN_LP", UIWidgets.EditBox, "Sound event name to call on the owner's SoundComponent", category: "LCN Sound")]
	protected string m_sSoundEventName;

	[Attribute("{6F18248533DE2C10}Sounds/Structures/Military/Sirens/Structures_Siren.acp", UIWidgets.ResourcePickerThumbnail, "ACP sound project used by the siren SoundComponent", "acp", category: "LCN Sound")]
	protected ResourceName m_sSoundProject;

	[Attribute("8", UIWidgets.EditBox, "How often the sound event is retriggered while the alarm is active", params: "1 120 1", category: "LCN Sound")]
	protected float m_fSoundRetriggerPeriod;

	[Attribute("1", UIWidgets.CheckBox, "Print alarm state to the script log", category: "LCN Debug")]
	protected bool m_bDebug;

	protected bool m_bWasAlarmActive;
	protected bool m_bWarnedMissingSound;
	protected float m_fSoundDelay;
	protected float m_fRemainingDuration;
	protected AudioHandle m_hSirenSound;
	protected bool m_bSirenSoundStarted;
	protected IEntity m_Owner;

	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Owner = owner;
		m_hSirenSound = AudioHandle.Invalid;
		SetEventMask(owner, EntityEvent.FRAME);

		if (m_bDebug)
			Print(string.Format("LCN_BaseAlarmSirenComponent: initialized alarm='%1' required='%2' soundProject='%3' event='%4'", m_sAlarmObjectiveKey, m_sRequiredObjectiveKey, m_sSoundProject, m_sSoundEventName));
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

			PlaySirenSound(owner);
		}
		else if (!alarmActive && m_bWasAlarmActive)
		{
			StopSirenSound();

			if (m_bDebug)
				Print(string.Format("LCN_BaseAlarmSirenComponent: alarm '%1' stopped", m_sAlarmObjectiveKey));
		}

		m_bWasAlarmActive = alarmActive;
		if (!alarmActive)
			return;

		if (UpdateAlarmDuration(owner, timeSlice))
		{
			m_bWasAlarmActive = false;
			return;
		}

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
	protected bool UpdateAlarmDuration(IEntity owner, float timeSlice)
	{
		if (m_fAlarmDuration <= 0)
			return false;

		if (!IsMaster())
			return false;

		m_fRemainingDuration -= timeSlice;
		if (m_fRemainingDuration > 0)
			return false;

		BaseWorld world = null;
		if (owner)
			world = owner.GetWorld();

		LCN_ObjectiveStateComponent.SetObjectiveKeyActive(m_sAlarmObjectiveKey, false, world);
		StopSirenSound();

		if (m_bDebug)
			Print(string.Format("LCN_BaseAlarmSirenComponent: alarm '%1' duration expired after %2 seconds", m_sAlarmObjectiveKey, m_fAlarmDuration));

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSirenSound(IEntity owner, float timeSlice)
	{
		if (m_bSirenSoundStarted)
			return;

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

		PlaySirenSoundLocal(owner);

		if (IsMaster())
			Rpc(RpcDo_PlaySirenSound);

		if (m_bDebug)
			Print(string.Format("LCN_BaseAlarmSirenComponent: sound event '%1' requested, master=%2", m_sSoundEventName, IsMaster()));
	}

	//------------------------------------------------------------------------------------------------
	protected void PlaySirenSoundLocal(IEntity owner)
	{
		if (!owner)
		{
			if (m_bDebug)
				Print("LCN_BaseAlarmSirenComponent: local sound skipped, owner is null");
			return;
		}

		if (m_sSoundEventName.IsEmpty())
		{
			if (m_bDebug)
				Print("LCN_BaseAlarmSirenComponent: local sound skipped, sound event name is empty");
			return;
		}

		if (m_bSirenSoundStarted && AudioSystem.IsSoundPlayed(m_hSirenSound))
		{
			if (m_bDebug)
				Print(string.Format("LCN_BaseAlarmSirenComponent: local sound '%1' already playing", m_sSoundEventName));
			return;
		}

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

		m_hSirenSound = soundComponent.SoundEvent(m_sSoundEventName);
		m_bSirenSoundStarted = true;

		if (m_bDebug)
			Print(string.Format("LCN_BaseAlarmSirenComponent: local sound event '%1' triggered, playing=%2", m_sSoundEventName, AudioSystem.IsSoundPlayed(m_hSirenSound)));
	}

	//------------------------------------------------------------------------------------------------
	protected void StopSirenSound()
	{
		StopSirenSoundLocal();

		if (IsMaster())
			Rpc(RpcDo_StopSirenSound);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopSirenSoundLocal()
	{
		if (m_bSirenSoundStarted)
			AudioSystem.TerminateSoundFadeOut(m_hSirenSound, true, 1);

		m_hSirenSound = AudioHandle.Invalid;
		m_bSirenSoundStarted = false;
		m_fSoundDelay = 0;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlaySirenSound()
	{
		PlaySirenSoundLocal(m_Owner);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_StopSirenSound()
	{
		StopSirenSoundLocal();
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
