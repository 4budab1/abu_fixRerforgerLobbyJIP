modded class SCR_MapMarkersUI
{
	protected bool m_bFactionInitialized = false;
	protected bool m_bFactionChangeSubscribed = false;
	protected bool m_bPSFactionChangeSubscribed = false;
	protected int m_iRetryCount = 0;
	protected const int MAX_RETRIES = 50;

	//------------------------------------------------------------------------------------------------
	override protected void CreateStaticMarkers()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction localFaction;

		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		abu_JIP_Debug.LogInfo(string.Format("CreateStaticMarkers() called - PlayerID: %1, RetryCount: %2", playerId, m_iRetryCount));

		if (factionManager)
			localFaction = factionManager.SGetLocalPlayerFaction();

		if (!localFaction)
			localFaction = abu_JIP_Debug.GetLocalPlayerFactionWithFallback();

		if (!localFaction)
		{
			m_iRetryCount++;
			abu_JIP_Debug.LogWarning(string.Format("Faction NOT SET yet! PlayerID: %1, Retry: %2/%3 - Subscribing to faction change events", playerId, m_iRetryCount, MAX_RETRIES));

			if (!m_bFactionChangeSubscribed)
			{
				SubscribeToFactionChangeUI();
			}
			if (!m_bPSFactionChangeSubscribed)
			{
				SubscribeToPSFactionChange();
			}
			return;
		}

		m_bFactionInitialized = true;
		string factionName = localFaction.GetFactionName();
		int localFactionIndex = factionManager.GetFactionIndex(localFaction);

		abu_JIP_Debug.LogInfo(string.Format("Faction FOUND! PlayerID: %1, Faction: %2, FactionIndex: %3", playerId, factionName, localFactionIndex));

		if (m_bFactionChangeSubscribed)
		{
			UnsubscribeFromFactionChangeUI();
		}
		if (m_bPSFactionChangeSubscribed)
		{
			UnsubscribeFromPSFactionChange();
		}

		array<SCR_MapMarkerBase> markersSimple = m_MarkerMgr.GetStaticMarkers();
		foreach (SCR_MapMarkerBase markerDis : m_MarkerMgr.GetDisabledMarkers())
		{
			markersSimple.Insert(markerDis);
		}

		int totalMarkers = markersSimple.Count();
		int visibleMarkers = 0;
		int enemyMarkers = 0;
		int noFactionMarkers = 0;

		abu_JIP_Debug.LogInfo(string.Format("Processing %1 markers - PlayerID: %2, Faction: %3", totalMarkers, playerId, factionName));

		foreach (SCR_MapMarkerBase marker : markersSimple)
		{
			if (!factionManager || marker.GetMarkerFactionFlags() == 0)
			{
				marker.OnCreateMarker(true);
				noFactionMarkers++;
				continue;
			}

			if (!localFaction || !marker.IsFaction(localFactionIndex))
			{
				if (Replication.IsServer())
				{
					marker.SetServerDisabled(true);
					abu_JIP_Debug.LogDebug(string.Format("Server disabled enemy marker - MarkerID: %1", marker.GetMarkerID()));
				}
				else
				{
					enemyMarkers++;
					abu_JIP_Debug.LogDebug(string.Format("Skipping enemy marker - MarkerID: %1, FactionFlags: %2", marker.GetMarkerID(), marker.GetMarkerFactionFlags()));
					continue;
				}
			}

			marker.OnCreateMarker(true);
			visibleMarkers++;
		}

		abu_JIP_Debug.LogInfo(string.Format("Markers created - Total: %1, Visible: %2, Enemy(Hidden): %3, NoFaction: %4 - PlayerID: %5, Faction: %6",
			totalMarkers, visibleMarkers, enemyMarkers, noFactionMarkers, playerId, factionName));

		m_iRetryCount = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void SubscribeToFactionChangeUI()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			abu_JIP_Debug.LogWarning("No PlayerController for UI faction subscription, retrying");
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChangeUI, 100);
			return;
		}

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionAffil)
		{
			abu_JIP_Debug.LogWarning("No FactionAffiliationComponent for UI, retrying");
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChangeUI, 100);
			return;
		}

		factionAffil.GetOnPlayerFactionChangedInvoker().Insert(OnLocalFactionChangedUI);
		m_bFactionChangeSubscribed = true;
		abu_JIP_Debug.LogInfo("Subscribed to OnPlayerFactionChangedInvoker (UI)");
	}

	//------------------------------------------------------------------------------------------------
	protected void UnsubscribeFromFactionChangeUI()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (factionAffil)
		{
			factionAffil.GetOnPlayerFactionChangedInvoker().Remove(OnLocalFactionChangedUI);
			m_bFactionChangeSubscribed = false;
			abu_JIP_Debug.LogInfo("Unsubscribed from OnPlayerFactionChangedInvoker (UI)");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SubscribeToPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
		{
			abu_JIP_Debug.LogWarning("No PS_PlayableManager for UI PS faction subscription, retrying");
			GetGame().GetCallqueue().CallLater(SubscribeToPSFactionChange, 100);
			return;
		}

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			abu_JIP_Debug.LogWarning("No PlayerController for UI PS faction subscription, retrying");
			GetGame().GetCallqueue().CallLater(SubscribeToPSFactionChange, 100);
			return;
		}

		playableManager.GetOnFactionChange().Insert(OnPSFactionChanged);
		m_bPSFactionChangeSubscribed = true;
		abu_JIP_Debug.LogInfo("Subscribed to PS_PlayableManager OnFactionChange (UI)");
	}

	//------------------------------------------------------------------------------------------------
	protected void UnsubscribeFromPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			playableManager.GetOnFactionChange().Remove(OnPSFactionChanged);
			m_bPSFactionChangeSubscribed = false;
			abu_JIP_Debug.LogInfo("Unsubscribed from PS_PlayableManager OnFactionChange (UI)");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPSFactionChanged(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		int localPlayerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			localPlayerId = pc.GetPlayerId();

		if (playerId != localPlayerId)
			return;

		abu_JIP_Debug.LogInfo(string.Format("OnPSFactionChanged() triggered - PlayerID: %1, FactionKey: %2, OldFactionKey: %3", playerId, factionKey, factionKeyOld));

		if (factionKey == "")
		{
			abu_JIP_Debug.LogWarning("New faction key is empty, skipping");
			return;
		}

		Faction newFaction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (!newFaction)
		{
			abu_JIP_Debug.LogWarning(string.Format("Could not resolve Faction from key: %1", factionKey));
			return;
		}

		m_bFactionInitialized = true;
		m_iRetryCount = 0;

		UnsubscribeFromPSFactionChange();
		UnsubscribeFromFactionChangeUI();

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen())
		{
			abu_JIP_Debug.LogInfo(string.Format("PS faction change processed - PlayerID: %1, Faction: %2 - Map is open, creating markers now", playerId, factionKey));

			CreateStaticMarkers();
			CreateDynamicMarkers();

			abu_JIP_Debug.LogInfo(string.Format("Markers created after PS faction change - PlayerID: %1", playerId));
		}
		else
		{
			abu_JIP_Debug.LogInfo(string.Format("PS faction change processed - PlayerID: %1, Faction: %2 - Map not open, markers will render on next map open", playerId, factionKey));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLocalFactionChangedUI(SCR_PlayerFactionAffiliationComponent component, Faction previousFaction, Faction newFaction)
	{
		string factionName = "Unknown";
		if (newFaction)
			factionName = newFaction.GetFactionName();

		int localPlayerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			localPlayerId = pc.GetPlayerId();

		abu_JIP_Debug.LogInfo(string.Format("OnLocalFactionChangedUI() triggered - PlayerID: %1, Faction: %2", localPlayerId, factionName));

		if (!newFaction)
		{
			abu_JIP_Debug.LogWarning("New faction is null, skipping");
			return;
		}

		m_bFactionInitialized = true;
		m_iRetryCount = 0;

		UnsubscribeFromFactionChangeUI();

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen())
		{
			abu_JIP_Debug.LogInfo(string.Format("Faction change processed - PlayerID: %1, Faction: %2 - Map is open, creating markers now", localPlayerId, factionName));

			CreateStaticMarkers();
			CreateDynamicMarkers();

			abu_JIP_Debug.LogInfo(string.Format("Markers created after faction change - PlayerID: %1", localPlayerId));
		}
		else
		{
			abu_JIP_Debug.LogInfo(string.Format("Faction change processed - PlayerID: %1, Faction: %2 - Map not open, markers will render on next map open", localPlayerId, factionName));
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		abu_JIP_Debug.LogInfo(string.Format("OnMapClose() - PlayerID: %1, FactionInitialized: %2", playerId, m_bFactionInitialized));

		super.OnMapClose(config);

		m_bFactionInitialized = false;
		m_iRetryCount = 0;

		UnsubscribeFromFactionChangeUI();
		UnsubscribeFromPSFactionChange();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		abu_JIP_Debug.LogInfo(string.Format("OnMapOpen() - PlayerID: %1", playerId));

		super.OnMapOpen(config);
	}
}
