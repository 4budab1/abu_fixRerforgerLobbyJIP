[EntityEditorProps(category: "GameScripted/Markers")]
class abu_JIP_Debug
{
	static const string TAG = "[abu_JIP_MarkersUI] ";

	static void Log(string message, LogLevel level = LogLevel.DEBUG)
	{
		Print(TAG + message, level);
	}

	static void LogError(string message)
	{
		Print(TAG + "[ERROR] " + message, LogLevel.ERROR);
	}

	static void LogWarning(string message)
	{
		Print(TAG + "[WARNING] " + message, LogLevel.WARNING);
	}

	static void LogInfo(string message)
	{
		Print(TAG + "[INFO] " + message, LogLevel.NORMAL);
	}

	static void LogDebug(string message)
	{
		Print(TAG + "[DEBUG] " + message, LogLevel.DEBUG);
	}

	static void LogVerbose(string message)
	{
		Print(TAG + "[VERBOSE] " + message, LogLevel.VERBOSE);
	}

	static Faction GetLocalPlayerFactionWithFallback()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
		{
			Faction localFaction = factionManager.SGetLocalPlayerFaction();
			if (localFaction)
				return localFaction;
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager)
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc)
			{
				FactionKey factionKey = playableManager.GetPlayerFactionKey(pc.GetPlayerId());
				if (factionKey == "")
					factionKey = playableManager.GetPlayerFactionKeyRemembered(pc.GetPlayerId());
				if (factionKey != "")
				{
					FactionManager fm = GetGame().GetFactionManager();
					if (fm)
					{
						Faction faction = fm.GetFactionByKey(factionKey);
						if (faction)
						{
							abu_JIP_Debug.LogInfo(string.Format("GetLocalPlayerFactionWithFallback: SCR_FactionManager returned null, using PS_PlayableManager fallback - FactionKey: %1", factionKey));
							return faction;
						}
					}
				}
			}
		}

		return null;
	}
}
