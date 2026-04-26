modded class PS_PolyZone
{
	protected bool m_bFactionChangeSubscribed = false;
	protected bool m_bPSFactionChangeSubscribed = false;

	override bool IsCurrentVisibility()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return true;

		if (m_aHideOnGameModeStates.Contains(gameMode.GetState()))
			return false;

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
		FactionKey factionKey = "";
		if (faction)
		{
			factionKey = faction.GetFactionKey();
		}
		else
		{
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager)
			{
				FactionKey psFactionKey = playableManager.GetPlayerFactionKey(playerController.GetPlayerId());
				if (psFactionKey == "")
					psFactionKey = playableManager.GetPlayerFactionKeyRemembered(playerController.GetPlayerId());
				if (psFactionKey != "")
					factionKey = psFactionKey;
			}

			if (factionKey == "")
			{
				if (!m_bFactionChangeSubscribed)
				{
					SubscribeToFactionChange();
				}
				if (!m_bPSFactionChangeSubscribed)
				{
					SubscribeToPSFactionChange();
				}
				return false;
			}
		}

		return m_aVisibleForFactions.Contains(factionKey);
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
	}

	protected void OnLocalPlayerFactionChanged(SCR_PlayerFactionAffiliationComponent component, Faction previousFaction, Faction newFaction)
	{
		if (!newFaction)
			return;

		UnsubscribeFromFactionChange();
		UnsubscribeFromPSFactionChange();

		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		if (IsCurrentVisibility())
		{
			if (!m_wCanvasWidget)
			{
				CreateMapWidget(m_MapEntity.GetMapConfig());
			}
		}
		else
		{
			if (m_wCanvasWidget)
			{
				m_wCanvasWidget.RemoveFromHierarchy();
				m_wCanvasWidget = null;
				ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);
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
	}

	protected void OnPSFactionChanged(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() != playerId)
			return;


		if (factionKey == "")
			return;

		UnsubscribeFromPSFactionChange();
		UnsubscribeFromFactionChange();

		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		if (IsCurrentVisibility())
		{
			if (!m_wCanvasWidget)
			{
				CreateMapWidget(m_MapEntity.GetMapConfig());
			}
		}
		else
		{
			if (m_wCanvasWidget)
			{
				m_wCanvasWidget.RemoveFromHierarchy();
				m_wCanvasWidget = null;
				ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);
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
