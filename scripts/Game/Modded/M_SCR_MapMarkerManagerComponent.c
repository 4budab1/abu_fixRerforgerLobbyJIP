modded class SCR_MapMarkerManagerComponent
{
	protected bool m_bFactionChangeSubscribed = false;
	protected bool m_bPSFactionChangeSubscribed = false;
	protected bool m_bRplLoadProcessed = false;
	protected ref array<SCR_MapMarkerBase> m_aEnemyMarkers = {};

	//------------------------------------------------------------------------------------------------
	override void SetStaticMarkerDisabled(notnull SCR_MapMarkerBase marker, bool state)
	{
		if (!state && m_aEnemyMarkers.Contains(marker) && !System.IsConsoleApp())
		{
			if (!m_bRplLoadProcessed)
			{
				abu_JIP_Debug.LogDebug(string.Format("SetStaticMarkerDisabled blocked re-enable of enemy marker before faction processed - MarkerID: %1", marker.GetMarkerID()));
				return;
			}

			FactionManager factionManager = GetGame().GetFactionManager();
			if (factionManager && marker.GetMarkerFactionFlags() != 0)
			{
				SCR_FactionManager scrFactionManager = SCR_FactionManager.Cast(factionManager);
				Faction localFaction = scrFactionManager.SGetLocalPlayerFaction();
				if (localFaction)
				{
					int localFactionIndex = factionManager.GetFactionIndex(localFaction);
					if (!marker.IsFaction(localFactionIndex))
					{
						abu_JIP_Debug.LogDebug(string.Format("SetStaticMarkerDisabled blocked re-enable of enemy marker - MarkerID: %1", marker.GetMarkerID()));
						return;
					}
				}
			}
		}

		super.SetStaticMarkerDisabled(marker, state);
	}

	//------------------------------------------------------------------------------------------------
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		SCR_FactionManager scrFactionManager = SCR_FactionManager.Cast(factionManager);

		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		int markerId = marker.GetMarkerID();
		int markerType = marker.GetType();
		int factionFlags = marker.GetMarkerFactionFlags();

		abu_JIP_Debug.LogInfo(string.Format("OnAddSynchedMarker() - PlayerID: %1, MarkerID: %2, Type: %3, FactionFlags: %4, IsServer: %5",
			playerId, markerId, markerType, factionFlags, System.IsConsoleApp()));

		if (System.IsConsoleApp())
		{
			m_aStaticMarkers.Insert(marker);
			marker.SetServerDisabled(true);
			abu_JIP_Debug.LogInfo(string.Format("Server side - marker disabled - MarkerID: %1", markerId));
			return;
		}

		if (marker.GetMarkerOwnerID() > -1)
			marker.RequestProfanityFilter();

		if (factionManager && marker.GetMarkerFactionFlags() != 0)
		{
			Faction localFaction = scrFactionManager.SGetLocalPlayerFaction();

			if (!localFaction)
				localFaction = abu_JIP_Debug.GetLocalPlayerFactionWithFallback();

			if (!localFaction)
			{
				abu_JIP_Debug.LogWarning(string.Format("Faction NOT SET - Deferring marker to enemy list! PlayerID: %1, MarkerID: %2", playerId, markerId));

				m_aEnemyMarkers.Insert(marker);
				m_aDisabledMarkers.Insert(marker);
				marker.SetUpdateDisabled(true);

				if (!m_bFactionChangeSubscribed)
				{
					SubscribeToFactionChange();
				}
				if (!m_bPSFactionChangeSubscribed)
				{
					SubscribeToPSFactionChange();
				}

				return;
			}

			int localFactionIndex = factionManager.GetFactionIndex(localFaction);
			bool isMyFaction = marker.IsFaction(localFactionIndex);

			abu_JIP_Debug.LogInfo(string.Format("Faction found - Faction: %1, Index: %2, IsMyFaction: %3",
				localFaction.GetFactionName(), localFactionIndex, isMyFaction));

			if (!isMyFaction)
			{
				m_aEnemyMarkers.Insert(marker);
				m_aDisabledMarkers.Insert(marker);
				marker.SetUpdateDisabled(true);
				marker.SetServerDisabled(true);
				abu_JIP_Debug.LogInfo(string.Format("Enemy marker moved to disabled - MarkerID: %1", markerId));
				return;
			}
		}

		m_aStaticMarkers.Insert(marker);

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen() && mapEnt.GetMapUIComponent(SCR_MapMarkersUI))
		{
			marker.OnCreateMarker(true);
			abu_JIP_Debug.LogInfo(string.Format("Marker widget created - MarkerID: %1", markerId));
		}

		CheckMarkersUserRestrictions();
	}

	//------------------------------------------------------------------------------------------------
	protected void SubscribeToFactionChange()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			abu_JIP_Debug.LogWarning("No PlayerController, retrying subscription");
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChange, 100);
			return;
		}

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionAffil)
		{
			abu_JIP_Debug.LogWarning("No FactionAffiliationComponent, retrying subscription");
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChange, 100);
			return;
		}

		factionAffil.GetOnPlayerFactionChangedInvoker().Insert(OnLocalPlayerFactionChanged);
		m_bFactionChangeSubscribed = true;
		abu_JIP_Debug.LogInfo("Subscribed to OnPlayerFactionChangedInvoker");
	}

	//------------------------------------------------------------------------------------------------
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
			abu_JIP_Debug.LogInfo("Unsubscribed from OnPlayerFactionChangedInvoker");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLocalPlayerFactionChanged(SCR_PlayerFactionAffiliationComponent component, Faction previousFaction, Faction newFaction)
	{
		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		string newFactionName = "Unknown";
		if (newFaction)
			newFactionName = newFaction.GetFactionName();

		string prevFactionName = "None";
		if (previousFaction)
			prevFactionName = previousFaction.GetFactionName();

		abu_JIP_Debug.LogInfo(string.Format("OnLocalPlayerFactionChanged() triggered - PlayerID: %1, PreviousFaction: %2, NewFaction: %3",
			playerId, prevFactionName, newFactionName));

		if (!newFaction)
		{
			abu_JIP_Debug.LogWarning("New faction is null, skipping");
			return;
		}

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (factionAffil)
		{
			factionAffil.GetOnPlayerFactionChangedInvoker().Remove(OnLocalPlayerFactionChanged);
			m_bFactionChangeSubscribed = false;
		}

		UnsubscribeFromPSFactionChange();

		FactionManager factionManager = GetGame().GetFactionManager();
		int localFactionIndex = factionManager.GetFactionIndex(newFaction);

		FilterMarkersByFaction(localFactionIndex, playerId, newFactionName);

		m_bRplLoadProcessed = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void SubscribeToPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
		{
			abu_JIP_Debug.LogWarning("No PS_PlayableManager, retrying PS faction subscription");
			GetGame().GetCallqueue().CallLater(SubscribeToPSFactionChange, 100);
			return;
		}

		playableManager.GetOnFactionChange().Insert(OnPSFactionChanged);
		m_bPSFactionChangeSubscribed = true;
		abu_JIP_Debug.LogInfo("Subscribed to PS_PlayableManager OnFactionChange");
	}

	//------------------------------------------------------------------------------------------------
	protected void UnsubscribeFromPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			playableManager.GetOnFactionChange().Remove(OnPSFactionChanged);
			m_bPSFactionChangeSubscribed = false;
			abu_JIP_Debug.LogInfo("Unsubscribed from PS_PlayableManager OnFactionChange");
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

		UnsubscribeFromPSFactionChange();
		UnsubscribeFromFactionChange();

		FactionManager factionManager = GetGame().GetFactionManager();
		int localFactionIndex = factionManager.GetFactionIndex(newFaction);

		FilterMarkersByFaction(localFactionIndex, playerId, factionKey);

		m_bRplLoadProcessed = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void FilterMarkersByFaction(int localFactionIndex, int playerId, string factionName)
	{
		abu_JIP_Debug.LogInfo(string.Format("FilterMarkersByFaction - FactionIndex: %1, StaticCount: %2, DisabledCount: %3, EnemyCount: %4",
			localFactionIndex, m_aStaticMarkers.Count(), m_aDisabledMarkers.Count(), m_aEnemyMarkers.Count()));

		array<SCR_MapMarkerBase> markersToRestore = {};
		foreach (SCR_MapMarkerBase marker : m_aDisabledMarkers)
		{
			if (m_aEnemyMarkers.Contains(marker))
				continue;
			if (marker.GetMarkerFactionFlags() == 0 || marker.IsFaction(localFactionIndex))
			{
				markersToRestore.Insert(marker);
			}
		}

		foreach (SCR_MapMarkerBase marker : markersToRestore)
		{
			m_aDisabledMarkers.RemoveItem(marker);
			m_aStaticMarkers.Insert(marker);
			marker.SetServerDisabled(false);
			marker.SetUpdateDisabled(false);
			abu_JIP_Debug.LogInfo(string.Format("Non-enemy marker restored from disabled - MarkerID: %1", marker.GetMarkerID()));
		}

		foreach (SCR_MapMarkerBase marker : m_aEnemyMarkers)
		{
			if (marker.GetMarkerFactionFlags() != 0 && !marker.IsFaction(localFactionIndex))
			{
				if (!m_aDisabledMarkers.Contains(marker))
					m_aDisabledMarkers.Insert(marker);
				if (m_aStaticMarkers.Contains(marker))
					m_aStaticMarkers.RemoveItem(marker);
				marker.SetServerDisabled(true);
				marker.SetUpdateDisabled(true);
				abu_JIP_Debug.LogInfo(string.Format("Enemy marker confirmed disabled - MarkerID: %1", marker.GetMarkerID()));
			}
			else
			{
				m_aDisabledMarkers.RemoveItem(marker);
				if (!m_aStaticMarkers.Contains(marker))
					m_aStaticMarkers.Insert(marker);
				marker.SetServerDisabled(false);
				marker.SetUpdateDisabled(false);
				m_aEnemyMarkers.RemoveItem(marker);
				abu_JIP_Debug.LogInfo(string.Format("Former enemy marker now friendly - MarkerID: %1", marker.GetMarkerID()));
			}
		}

		int visibleCount = 0;
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		bool mapOpen = mapEnt && mapEnt.IsOpen() && mapEnt.GetMapUIComponent(SCR_MapMarkersUI);

		foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
		{
			if (!marker.GetRootWidget() && !marker.GetBlocked())
			{
				if (mapOpen)
				{
					marker.OnCreateMarker(true);
					visibleCount++;
					abu_JIP_Debug.LogInfo(string.Format("Friendly marker widget created - MarkerID: %1", marker.GetMarkerID()));
				}
			}
		}

		abu_JIP_Debug.LogInfo(string.Format("FilterMarkersByFaction done - Restored: %1, VisibleCreated: %2, StaticCount: %3, DisabledCount: %4, EnemyCount: %5, PlayerID: %6, Faction: %7",
			markersToRestore.Count(), visibleCount, m_aStaticMarkers.Count(), m_aDisabledMarkers.Count(), m_aEnemyMarkers.Count(), playerId, factionName));
	}

	//------------------------------------------------------------------------------------------------
	override event protected bool RplLoad(ScriptBitReader reader)
	{
		abu_JIP_Debug.LogInfo("RplLoad() called - Loading markers from replication");

		bool result = super.RplLoad(reader);

		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		abu_JIP_Debug.LogInfo(string.Format("RplLoad() completed - PlayerID: %1, Result: %2, StaticMarkersCount: %3",
			playerId, result, m_aStaticMarkers.Count()));

		if (!System.IsConsoleApp())
		{
			SCR_FactionManager scrFactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction localFaction = scrFactionManager.SGetLocalPlayerFaction();

			if (!localFaction)
				localFaction = abu_JIP_Debug.GetLocalPlayerFactionWithFallback();

			if (!localFaction)
			{
				abu_JIP_Debug.LogWarning(string.Format("RplLoad: Faction NOT SET - Moving all markers to enemy/disabled list to defer! Count: %1", m_aStaticMarkers.Count()));

				foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
				{
					if (marker.GetMarkerFactionFlags() != 0)
						m_aEnemyMarkers.Insert(marker);
					m_aDisabledMarkers.Insert(marker);
					marker.SetUpdateDisabled(true);
				}
				m_aStaticMarkers.Clear();

				if (!m_bFactionChangeSubscribed)
				{
					SubscribeToFactionChange();
				}
				if (!m_bPSFactionChangeSubscribed)
				{
					SubscribeToPSFactionChange();
				}
			}
			else
			{
				abu_JIP_Debug.LogInfo(string.Format("RplLoad: Faction FOUND - %1, filtering markers now", localFaction.GetFactionName()));

				FactionManager factionManager = GetGame().GetFactionManager();
				int localFactionIndex = factionManager.GetFactionIndex(localFaction);

				array<SCR_MapMarkerBase> markersToRemove = {};
				foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
				{
					if (marker.GetMarkerFactionFlags() != 0)
					{
						if (!marker.IsFaction(localFactionIndex))
						{
							markersToRemove.Insert(marker);
							abu_JIP_Debug.LogInfo(string.Format("RplLoad: Enemy marker queued for removal - MarkerID: %1", marker.GetMarkerID()));
						}
					}
				}

				foreach (SCR_MapMarkerBase marker : markersToRemove)
				{
					m_aEnemyMarkers.Insert(marker);
					m_aDisabledMarkers.Insert(marker);
					marker.SetServerDisabled(true);
					marker.SetUpdateDisabled(true);
					m_aStaticMarkers.RemoveItem(marker);
				}

				abu_JIP_Debug.LogInfo(string.Format("RplLoad: Filtered - Removed: %1, Remaining: %2", markersToRemove.Count(), m_aStaticMarkers.Count()));
				m_bRplLoadProcessed = true;
			}
		}

		return result;
	}
}
