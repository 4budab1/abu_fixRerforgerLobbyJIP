modded class SCR_MapMarkersUI
{
	protected bool m_bFactionInitialized = false;
	protected bool m_bFactionChangeSubscribed = false;
	protected bool m_bPSFactionChangeSubscribed = false;

	//------------------------------------------------------------------------------------------------
	override protected void CreateStaticMarkers()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction localFaction;

		PlayerController pc = GetGame().GetPlayerController();

		if (factionManager)
			localFaction = SCR_FactionManager.SGetLocalPlayerFaction();

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
		if (!m_bFactionChangeSubscribed)
				SubscribeToFactionChangeUI();
			if (!m_bPSFactionChangeSubscribed)
				SubscribeToPSFactionChange();
			return;
		}

	m_bFactionInitialized = true;
	int localFactionIndex = factionManager.GetFactionIndex(localFaction);

	if (m_bFactionChangeSubscribed)
		UnsubscribeFromFactionChangeUI();
	if (m_bPSFactionChangeSubscribed)
		UnsubscribeFromPSFactionChange();

	array<SCR_MapMarkerBase> markersSimple = m_MarkerMgr.GetStaticMarkers();
	foreach (SCR_MapMarkerBase markerDis : m_MarkerMgr.GetDisabledMarkers())
	{
		markersSimple.Insert(markerDis);
	}

	foreach (SCR_MapMarkerBase marker : markersSimple)
	{
		if (!factionManager || marker.GetMarkerFactionFlags() == 0)
		{
			marker.OnCreateMarker(true);
			continue;
		}

		if (!localFaction || !marker.IsFaction(localFactionIndex))
		{
			if (Replication.IsServer())
			{
				marker.SetServerDisabled(true);
			}
			else
			{
				continue;
			}
		}

		marker.OnCreateMarker(true);
	}
	}

	//------------------------------------------------------------------------------------------------
	protected void SubscribeToFactionChangeUI()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChangeUI, 100);
			return;
		}

		SCR_PlayerFactionAffiliationComponent factionAffil = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionAffil)
		{
			GetGame().GetCallqueue().CallLater(SubscribeToFactionChangeUI, 100);
			return;
		}

		factionAffil.GetOnPlayerFactionChangedInvoker().Insert(OnLocalFactionChangedUI);
		m_bFactionChangeSubscribed = true;
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
		}
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

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
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
		if (!pc || pc.GetPlayerId() != playerId)
			return;

		if (factionKey == "")
			return;

		Faction newFaction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (!newFaction)
			return;

	m_bFactionInitialized = true;

	UnsubscribeFromPSFactionChange();
		UnsubscribeFromFactionChangeUI();

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen())
		{
			CreateStaticMarkers();
			CreateDynamicMarkers();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLocalFactionChangedUI(SCR_PlayerFactionAffiliationComponent component, Faction previousFaction, Faction newFaction)
	{
		if (!newFaction)
			return;

	m_bFactionInitialized = true;

	UnsubscribeFromFactionChangeUI();
	UnsubscribeFromPSFactionChange();

		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (mapEnt && mapEnt.IsOpen())
		{
			CreateStaticMarkers();
			CreateDynamicMarkers();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		super.OnMapClose(config);

	m_bFactionInitialized = false;

	UnsubscribeFromFactionChangeUI();
		UnsubscribeFromPSFactionChange();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
	}
}