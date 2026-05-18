class LCN_ConfiguredObjectiveAction : SCR_ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!IsMaster())
			return;

		if (!CanBePerformedScript(pUserEntity))
			return;

		LCN_ConfiguredObjectiveActionComponent settings = GetSettings();
		if (!settings)
			return;

		if (settings.Perform(pOwnerEntity, pUserEntity) && settings.IsOneUse())
			SetActionEnabled_S(false);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!super.CanBeShownScript(user))
			return false;

		LCN_ConfiguredObjectiveActionComponent settings = GetSettings();
		if (!settings)
			return false;

		ApplyConfiguredDuration(settings);
		return settings.CanBeShown(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		LCN_ConfiguredObjectiveActionComponent settings = GetSettings();
		if (!settings)
		{
			SetCannotPerformReason("Action config missing");
			return false;
		}

		ApplyConfiguredDuration(settings);

		string reason;
		if (!settings.CanBePerformed(GetOwner(), user, reason))
		{
			SetCannotPerformReason(reason);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		LCN_ConfiguredObjectiveActionComponent settings = GetSettings();
		if (!settings)
			return false;

		outName = settings.GetActionName();
		return !outName.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionDescriptionScript(out string outName)
	{
		LCN_ConfiguredObjectiveActionComponent settings = GetSettings();
		if (!settings)
			return false;

		outName = settings.GetActionDescription();
		return !outName.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		LCN_ConfiguredObjectiveActionComponent settings = GetSettings();
		if (settings)
			ApplyConfiguredDuration(settings);

		super.OnActionStart(pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected LCN_ConfiguredObjectiveActionComponent GetSettings()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		return LCN_ConfiguredObjectiveActionComponent.Cast(owner.FindComponent(LCN_ConfiguredObjectiveActionComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyConfiguredDuration(LCN_ConfiguredObjectiveActionComponent settings)
	{
		SetActionDuration(settings.GetActionDuration());
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
