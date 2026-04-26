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
				return;

			FactionManager factionManager = GetGame().GetFactionManager();
			if (factionManager && marker.GetMarkerFactionFlags() != 0)
			{
				Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
				if (localFaction)
				{
					int localFactionIndex = factionManager.GetFactionIndex(localFaction);
					if (!marker.IsFaction(localFactionIndex))
						return;
				}
			}
		}

		super.SetStaticMarkerDisabled(marker, state);
	}

	//------------------------------------------------------------------------------------------------
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return;

		FactionManager factionManager = GetGame().GetFactionManager();

		int playerId = 0;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			playerId = pc.GetPlayerId();

		if (System.IsConsoleApp())
		{
			m_aStaticMarkers.Insert(marker);
			marker.SetServerDisabled(true);
			return;
		}

		if (marker.GetMarkerOwnerID() > -1)
			marker.RequestProfanityFilter();

		if (factionManager && marker.GetMarkerFactionFlags() != 0)
		{
			Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();

		if (!localFaction)
		{
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager && pc)
			{
				FactionKey psFactionKey = playableManager.GetPlayerFactionKey(pc.GetPlayerId());
					if (psFactionKey == "")
						psFactionKey = playableManager.GetPlayerFactionKeyRemembered(pc.GetPlayerId());
					if (psFactionKey != "")
						localFaction = GetGame().GetFactionManager().GetFactionByKey(psFactionKey);
				}
			}

			if (!localFaction)
			{
				m_aEnemyMarkers.Insert(marker);
				m_aDisabledMarkers.Insert(marker);
				marker.SetUpdateDisabled(true);

				if (!m_bFactionChangeSubscribed)
					SubscribeToFactionChange();
				if (!m_bPSFactionChangeSubscribed)
					SubscribeToPSFactionChange();

				return;
			}

			int localFactionIndex = factionManager.GetFactionIndex(localFaction);
			bool isMyFaction = marker.IsFaction(localFactionIndex);

			if (!isMyFaction)
			{
				m_aEnemyMarkers.Insert(marker);
				m_aDisabledMarkers.Insert(marker);
				marker.SetUpdateDisabled(true);
				marker.SetServerDisabled(true);
				return;
			}
		}

		m_aStaticMarkers.Insert(marker);

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen() && mapEnt.GetMapUIComponent(SCR_MapMarkersUI))
			marker.OnCreateMarker(true);

		CheckMarkersUserRestrictions();
	}

	//------------------------------------------------------------------------------------------------
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
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLocalPlayerFactionChanged(SCR_PlayerFactionAffiliationComponent component, Faction previousFaction, Faction newFaction)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		if (!newFaction)
			return;

		if (m_aStaticMarkers.Count() == 0 && m_aDisabledMarkers.Count() == 0 && m_aEnemyMarkers.Count() == 0)
			return;

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (factionAffil)
		{
			factionAffil.GetOnPlayerFactionChangedInvoker().Remove(OnLocalPlayerFactionChanged);
			m_bFactionChangeSubscribed = false;
		}

		UnsubscribeFromPSFactionChange();

		FactionManager factionManager = GetGame().GetFactionManager();
		int localFactionIndex = factionManager.GetFactionIndex(newFaction);

		FilterMarkersByFaction(localFactionIndex, pc.GetPlayerId(), newFaction.GetFactionName());

		m_bRplLoadProcessed = true;
	}

	//------------------------------------------------------------------------------------------------
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
	}

	//------------------------------------------------------------------------------------------------
	protected void UnsubscribeFromPSFactionChange()
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			playableManager.GetOnFactionChange().Remove(OnPSFactionChanged);
			m_bPSFactionChangeSubscribed = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPSFactionChanged(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		if (playerId != pc.GetPlayerId())
			return;

		if (factionKey == "")
			return;

		if (m_aStaticMarkers.Count() == 0 && m_aDisabledMarkers.Count() == 0 && m_aEnemyMarkers.Count() == 0)
			return;

		Faction newFaction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (!newFaction)
			return;

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
		array<SCR_MapMarkerBase> markersToRestore = {};
		for (int dIdx = 0; dIdx < m_aDisabledMarkers.Count(); dIdx++)
		{
			SCR_MapMarkerBase marker = m_aDisabledMarkers[dIdx];
			if (!marker)
				continue;
			if (m_aEnemyMarkers.Contains(marker))
				continue;
			if (marker.GetMarkerFactionFlags() == 0 || marker.IsFaction(localFactionIndex))
				markersToRestore.Insert(marker);
		}

		foreach (SCR_MapMarkerBase marker : markersToRestore)
		{
			m_aDisabledMarkers.RemoveItem(marker);
			m_aStaticMarkers.Insert(marker);
			marker.SetServerDisabled(false);
			marker.SetUpdateDisabled(false);
		}

		array<SCR_MapMarkerBase> enemyMarkersToProcess = {};
		foreach (SCR_MapMarkerBase eMarker : m_aEnemyMarkers)
		{
			if (eMarker)
				enemyMarkersToProcess.Insert(eMarker);
		}

		foreach (SCR_MapMarkerBase marker : enemyMarkersToProcess)
		{
			if (marker.GetMarkerFactionFlags() != 0 && !marker.IsFaction(localFactionIndex))
			{
				if (!m_aDisabledMarkers.Contains(marker))
					m_aDisabledMarkers.Insert(marker);
				if (m_aStaticMarkers.Contains(marker))
					m_aStaticMarkers.RemoveItem(marker);
				marker.SetServerDisabled(true);
				marker.SetUpdateDisabled(true);
			}
			else
			{
				m_aDisabledMarkers.RemoveItem(marker);
				if (!m_aStaticMarkers.Contains(marker))
					m_aStaticMarkers.Insert(marker);
				marker.SetServerDisabled(false);
				marker.SetUpdateDisabled(false);
			}
		}
		m_aEnemyMarkers.Clear();

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		bool mapOpen = mapEnt && mapEnt.IsOpen() && mapEnt.GetMapUIComponent(SCR_MapMarkersUI);

		foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
		{
			if (!marker)
				continue;
			if (!marker.GetRootWidget() && !marker.GetBlocked())
			{
				if (mapOpen)
					marker.OnCreateMarker(true);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override event protected bool RplLoad(ScriptBitReader reader)
	{
		bool result = super.RplLoad(reader);

		if (!System.IsConsoleApp())
		{
			Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();

			if (!localFaction)
			{
				PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
				if (playableManager)
				{
					PlayerController pc = GetGame().GetPlayerController();
					if (pc)
					{
						FactionKey psFactionKey = playableManager.GetPlayerFactionKey(pc.GetPlayerId());
						if (psFactionKey == "")
							psFactionKey = playableManager.GetPlayerFactionKeyRemembered(pc.GetPlayerId());
						if (psFactionKey != "")
							localFaction = GetGame().GetFactionManager().GetFactionByKey(psFactionKey);
					}
				}
			}

			if (!localFaction)
			{
				foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
				{
					if (!marker)
						continue;
					if (marker.GetMarkerFactionFlags() != 0)
						m_aEnemyMarkers.Insert(marker);
					m_aDisabledMarkers.Insert(marker);
					marker.SetUpdateDisabled(true);
				}
				m_aStaticMarkers.Clear();

				if (!m_bFactionChangeSubscribed)
					SubscribeToFactionChange();
				if (!m_bPSFactionChangeSubscribed)
					SubscribeToPSFactionChange();
			}
			else
			{
				FactionManager factionManager = GetGame().GetFactionManager();
				int localFactionIndex = factionManager.GetFactionIndex(localFaction);

				array<SCR_MapMarkerBase> markersToRemove = {};
				foreach (SCR_MapMarkerBase marker : m_aStaticMarkers)
				{
					if (!marker)
						continue;
					if (marker.GetMarkerFactionFlags() != 0)
					{
						if (!marker.IsFaction(localFactionIndex))
							markersToRemove.Insert(marker);
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

				m_bRplLoadProcessed = true;
			}
		}

		return result;
	}
}