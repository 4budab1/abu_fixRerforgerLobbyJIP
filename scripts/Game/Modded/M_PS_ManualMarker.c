modded class PS_ManualMarker
{
	protected bool m_bFactionChangeSubscribed = false;
	protected bool m_bPSFactionChangeSubscribed = false;

	override bool IsCurrentFactionVisibility()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return true;

		if (m_aHideOnGameModeStates.Contains(gameMode.GetState()))
			return false;

		if (m_bShowForAnyFaction)
			return true;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return true;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return true;

		SCR_PlayerFactionAffiliationComponent playerFactionAffiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!playerFactionAffiliationComponent)
			return true;

		Faction faction = playerFactionAffiliationComponent.GetAffiliatedFaction();
		if (faction == null)
		{
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager)
			{
				FactionKey psFactionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
				if (psFactionKey == "")
					psFactionKey = playableManager.GetPlayerFactionKeyRemembered(playerController.GetPlayerId());
				if (psFactionKey != "")
					return GetVisibleForFaction(psFactionKey);
			}

			if (!m_bFactionChangeSubscribed)
			{
				SubscribeToFactionChange();
			}
			if (!m_bPSFactionChangeSubscribed)
			{
				SubscribeToPSFactionChange();
			}
			return m_bVisibleForEmptyFaction;
		}

		return GetVisibleForFaction(faction.GetFactionKey());
	}

	protected void SubscribeToFactionChange()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChange, 100);
			return;
		}

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionAffil)
		{
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChange, 100);
			return;
		}

		factionAffil.GetOnPlayerFactionChangedInvoker().Insert(OnLocalPlayerFactionChanged);
		m_bFactionChangeSubscribed = true;
		abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker Subscribed to faction change - Marker: %1", GetOrigin().ToString()));
	}

	protected void OnLocalPlayerFactionChanged(SCR_PlayerFactionAffiliationComponent component, Faction previousFaction, Faction newFaction)
	{
		string newFactionName = "Unknown";
		if (newFaction)
			newFactionName = newFaction.GetFactionName();

		abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker OnLocalPlayerFactionChanged - NewFaction: %1, Marker: %2", newFactionName, GetOrigin().ToString()));

		UnsubscribeFromFactionChange();
		UnsubscribeFromPSFactionChange();

		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		if (IsCurrentFactionVisibility())
		{
			if (!m_wRoot)
			{
				CreateMapWidget(m_MapEntity.GetMapConfig());
				abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker created widget after faction change - Faction: %1, Marker: %2", newFactionName, GetOrigin().ToString()));
			}
		}
		else
		{
			if (m_wRoot)
			{
				DeleteMapWidget(m_MapEntity.GetMapConfig());
				abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker removed widget after faction change - Faction: %1, Marker: %2", newFactionName, GetOrigin().ToString()));
			}
		}
	}

	protected void SubscribeToPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
		{
			GetGame().GetCallqueue().CallLater(SubscribeToPSFactionChange, 100);
			return;
		}

		playableManager.GetOnFactionChange().Insert(OnPSFactionChanged);
		m_bPSFactionChangeSubscribed = true;
		abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker Subscribed to PS_PlayableManager faction change - Marker: %1", GetOrigin().ToString()));
	}

	protected void OnPSFactionChanged(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() != playerId)
			return;

		abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker OnPSFactionChanged - FactionKey: %1, Marker: %2", factionKey, GetOrigin().ToString()));

		if (factionKey == "")
			return;

		UnsubscribeFromPSFactionChange();
		UnsubscribeFromFactionChange();

		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		if (IsCurrentFactionVisibility())
		{
			if (!m_wRoot)
			{
				CreateMapWidget(m_MapEntity.GetMapConfig());
				abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker created widget after PS faction change - FactionKey: %1, Marker: %2", factionKey, GetOrigin().ToString()));
			}
		}
		else
		{
			if (m_wRoot)
			{
				DeleteMapWidget(m_MapEntity.GetMapConfig());
				abu_JIP_Debug.LogInfo(string.Format("PS_ManualMarker removed widget after PS faction change - FactionKey: %1, Marker: %2", factionKey, GetOrigin().ToString()));
			}
		}
	}

	protected void UnsubscribeFromPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			playableManager.GetOnFactionChange().Remove(OnPSFactionChanged);
			m_bPSFactionChangeSubscribed = false;
		}
	}

	protected void UnsubscribeFromFactionChange()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (factionAffil)
		{
			factionAffil.GetOnPlayerFactionChangedInvoker().Remove(OnLocalPlayerFactionChanged);
			m_bFactionChangeSubscribed = false;
		}
	}

	override void DeleteMapWidget(MapConfiguration mapConfig)
	{
		UnsubscribeFromFactionChange();
		UnsubscribeFromPSFactionChange();
		super.DeleteMapWidget(mapConfig);
	}
}
