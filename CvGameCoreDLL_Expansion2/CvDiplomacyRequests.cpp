/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvDiplomacyRequests.h"
#include "CvPlayer.h"
#include "FStlContainerSerialization.h"
#include "ICvDLLUserInterface.h"
#include "CvEnumSerialization.h"
#include "CvDiplomacyAI.h"
#include "CvDllInterfaces.h"

// Include this after all other headers.
#include "LintFree.h"

/// Serialization read
FDataStream& operator>>(FDataStream& loadFrom, CvDiplomacyRequests::Request& writeTo)
{
	loadFrom >> writeTo.m_eDiploType;
	loadFrom >> writeTo.m_eAnimationType;
	loadFrom >> writeTo.m_strMessage;
	loadFrom >> writeTo.m_iExtraGameData;
	loadFrom >> writeTo.m_iTurn;
	loadFrom >> writeTo.m_iLookupIndex;
	loadFrom >> writeTo.m_eFromPlayer;

	return loadFrom;
}

/// Serialization write
FDataStream& operator<<(FDataStream& saveTo, const CvDiplomacyRequests::Request& readFrom)
{
	saveTo << readFrom.m_eDiploType;
	saveTo << readFrom.m_eAnimationType;
	saveTo << readFrom.m_strMessage;
	saveTo << readFrom.m_iExtraGameData;
	saveTo << readFrom.m_iTurn;
	saveTo << readFrom.m_iLookupIndex;
	saveTo << readFrom.m_eFromPlayer;

	return saveTo;
}

void CvDiplomacyRequests::Request::Clear()
{
	m_eDiploType = NO_DIPLO_UI_STATE;
	m_eAnimationType = NO_LEADERHEAD_ANIM;
	m_eFromPlayer = NO_PLAYER;
	m_strMessage = "";
	m_iTurn = -1;
	m_iLookupIndex = -1;
}

/// Constructor
CvDiplomacyRequests::CvDiplomacyRequests(void)
{
	Uninit();
}

/// Destructor
CvDiplomacyRequests::~CvDiplomacyRequests(void)
{
	Uninit();
}

/// Init
void CvDiplomacyRequests::Init(PlayerTypes ePlayer)
{
	Uninit();
	m_ePlayer = ePlayer;

	m_aRequests.clear();
}

/// Uninit
void CvDiplomacyRequests::Uninit(void)
{
	m_ePlayer = NO_PLAYER;
	m_eNextAIPlayer = NO_PLAYER;
	m_aRequests.clear();
	m_bRequestActive = false;
	m_bRequestActiveFromPlayer = NO_PLAYER;
}

/// Serialization read
void CvDiplomacyRequests::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	kStream >> m_ePlayer;

	uint uiListSize;
	kStream >> uiListSize;

	if(uiListSize > 0)
	{
		for(uint ui = 0; ui < uiListSize; ui++)
		{
			m_aRequests.push_back(Request());
			kStream >> m_aRequests.back();
		}
	}
}

/// Serialization write
void CvDiplomacyRequests::Write(FDataStream& kStream) const
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;

	// need to serialize notification list
	kStream << m_ePlayer;

	kStream << m_aRequests.size();

	for(RequestList::const_iterator i = m_aRequests.begin(); i != m_aRequests.end(); ++i)
	{
		kStream << (*i);
	}
}

//	----------------------------------------------------------------------------
/// Update - called from within CvPlayer
void CvDiplomacyRequests::Update(void)
{
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
	if (m_ePlayer == NO_PLAYER || m_aRequests.size() <= 0)
		return;
	CvPlayer& kToPlayer = GET_PLAYER(m_ePlayer);
#ifdef AUI_GAME_BETTER_HYBRID_MODE
	if (GC.getGame().isNetworkMultiPlayer() && !(GC.getMULTIPLAYER_AI_DIPLOMACY_NOTIFICATIONS_ONLY_SIMULTANEOUS() > 0 && GC.getGame().isAnySimultaneousTurns() && kToPlayer.isHuman()))
#else
	if (GC.getGame().isNetworkMultiPlayer() && !(GC.getMULTIPLAYER_AI_DIPLOMACY_NOTIFICATIONS_ONLY_SIMULTANEOUS() > 0 && kToPlayer.isSimultaneousTurns()))
#endif
	{
		CvNotifications* pNotifications = kToPlayer.GetNotifications();
		if (pNotifications)
		{
			Localization::String strSummary = Localization::Lookup("TXT_KEY_DEAL_OFFERED");
			for (CvDiplomacyRequests::RequestList::iterator itRequest = m_aRequests.begin(); itRequest != m_aRequests.end(); ++itRequest)
			{
				// Make sure the player this is from is still alive.
				if (itRequest->m_eFromPlayer != NO_PLAYER && itRequest->m_bIsValid)
				{
					CvPlayer& kFromPlayer = GET_PLAYER(itRequest->m_eFromPlayer);
					if (kFromPlayer.isAlive())
					{
						Localization::String strMessage = Localization::Lookup("TXT_KEY_DEAL_OFFERED_BY_THEM");
						strMessage << kFromPlayer.getNameKey();
						pNotifications->Add(NOTIFICATION_PLAYER_DEAL, strMessage.toUTF8(), strSummary.toUTF8(), itRequest->m_eFromPlayer, std::distance(m_aRequests.begin(), itRequest), -1);
						itRequest->m_bIsValid = false;
					}
				}
			}
		}
	}
	else if (kToPlayer.isTurnActive() && GC.getGame().getActivePlayer() == m_ePlayer)
#else
	PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
	// If we are active, send out the requests
	if(m_aRequests.size() && m_ePlayer == eActivePlayer && GET_PLAYER(eActivePlayer).isTurnActive())
#endif
	{
		CvDiplomacyRequests::Request& kRequest = m_aRequests.front();

		// Make sure the player this is from is still alive.
		if(kRequest.m_eFromPlayer != NO_PLAYER && GET_PLAYER(kRequest.m_eFromPlayer).isAlive())
		{
			Send(kRequest.m_eFromPlayer, kRequest.m_eDiploType, kRequest.m_strMessage, kRequest.m_eAnimationType, kRequest.m_iExtraGameData);
		}
		m_aRequests.pop_front();
	}
}

//	----------------------------------------------------------------------------
//	Called from within CvPlayer at the beginning of the turn
void CvDiplomacyRequests::BeginTurn(void)
{
	m_eNextAIPlayer = (PlayerTypes)0;
}

//	----------------------------------------------------------------------------
//	Called from within CvPlayer at the end of turn
void CvDiplomacyRequests::EndTurn(void)
{
	m_eNextAIPlayer = NO_PLAYER;
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
	m_aRequests.clear();
#endif
}

//	----------------------------------------------------------------------------
/// Adds a new notification to the list
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
bool CvDiplomacyRequests::Add(PlayerTypes eFromPlayer, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType, int iExtraGameData /*= -1*/, CvDeal* pDeal)
#else
bool CvDiplomacyRequests::Add(PlayerTypes eFromPlayer, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType, int iExtraGameData /*= -1*/)
#endif
{
	// Queue it up
	m_aRequests.push_back(Request());
	Request& newRequest = m_aRequests.back();

	newRequest.Clear();
	newRequest.m_eFromPlayer = eFromPlayer;
	newRequest.m_eDiploType = eDiploType;
	newRequest.m_strMessage = pszMessage;
	newRequest.m_iExtraGameData = iExtraGameData;
	newRequest.m_eAnimationType = eAnimationType;
	newRequest.m_iTurn = GC.getGame().getGameTurn();
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
	newRequest.m_pDeal = pDeal;
#endif

	return true;
}

#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
void CvDiplomacyRequests::SendExistingRequest(uint uiDistance)
{
	if (uiDistance < m_aRequests.size())
	{
		CvDiplomacyRequests::RequestList::iterator itRequest = m_aRequests.begin();
		std::advance(itRequest, uiDistance);
		// Make sure the player this is from is still alive.
		if (itRequest->m_eFromPlayer != NO_PLAYER && GET_PLAYER(itRequest->m_eFromPlayer).isAlive())
		{
			Send(itRequest->m_eFromPlayer, itRequest->m_eDiploType, itRequest->m_strMessage, itRequest->m_eAnimationType, itRequest->m_iExtraGameData, itRequest->m_pDeal);
		}
	}
}
#endif

//	----------------------------------------------------------------------------
//	Send the request immediately
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
void CvDiplomacyRequests::Send(PlayerTypes eFromPlayer, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType, int iExtraGameData, CvDeal* pDeal)
{
	if (pDeal)
	{
		auto_ptr<ICvDeal1> pIDeal = GC.WrapDealPointer(pDeal);
		GC.GetEngineUserInterface()->SetScratchDeal(pIDeal.get());
	}
#else
void CvDiplomacyRequests::Send(PlayerTypes eFromPlayer, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType, int iExtraGameData /*= -1*/)
{
#endif
	gDLL->GameplayDiplomacyAILeaderMessage(eFromPlayer, eDiploType, pszMessage, eAnimationType, iExtraGameData);
	m_bRequestActiveFromPlayer = eFromPlayer;
	m_bRequestActive = true;
}

//	----------------------------------------------------------------------------
PlayerTypes CvDiplomacyRequests::GetNextAIPlayer() const
{
	return m_eNextAIPlayer;
}
//	----------------------------------------------------------------------------
void CvDiplomacyRequests::SetNextAIPlayer(PlayerTypes eNextPlayer)
{
	m_eNextAIPlayer = eNextPlayer;
}

//	----------------------------------------------------------------------------
bool CvDiplomacyRequests::HasPendingRequests() const
{
	return !m_aRequests.empty() || m_bRequestActive;
}

//	----------------------------------------------------------------------------
bool CvDiplomacyRequests::HasActiveRequest() const
{
	return m_bRequestActive;
}

//	----------------------------------------------------------------------------
bool CvDiplomacyRequests::HasActiveRequestFrom(PlayerTypes eFromPlayer) const
{
	return m_bRequestActive && m_bRequestActiveFromPlayer == eFromPlayer;
}

//	----------------------------------------------------------------------------
//	Send a request from a player to another player.
//	If the toPlayer is the active human player, it will be sent right away, else
//	it will be queued.
// static
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
void CvDiplomacyRequests::SendRequest(PlayerTypes eFromPlayer, PlayerTypes eToPlayer, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType, int iExtraGameData, CvDeal* pDeal)
#else
void CvDiplomacyRequests::SendRequest(PlayerTypes eFromPlayer, PlayerTypes eToPlayer, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType, int iExtraGameData /*= -1*/)
#endif
{
	CvPlayer& kPlayer = GET_PLAYER(eToPlayer);
	CvDiplomacyRequests* pkDiploRequests = kPlayer.GetDiplomacyRequests();
	if(pkDiploRequests)
	{
		if(!CvPreGame::isNetworkMultiplayerGame() && GC.getGame().getActivePlayer() == eToPlayer)
		{
			// Target is the active player, just send it right now
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
			pkDiploRequests->Send(eFromPlayer, eDiploType, pszMessage, eAnimationType, iExtraGameData, pDeal);
		}
		else
			pkDiploRequests->Add(eFromPlayer, eDiploType, pszMessage, eAnimationType, iExtraGameData, pDeal);
#else
			pkDiploRequests->Send(eFromPlayer, eDiploType, pszMessage, eAnimationType, iExtraGameData);
		}
		else
			pkDiploRequests->Add(eFromPlayer, eDiploType, pszMessage, eAnimationType, iExtraGameData);
#endif
	}
}

//	----------------------------------------------------------------------------
//	Request for a deal
//static
void CvDiplomacyRequests::SendDealRequest(PlayerTypes eFromPlayer, PlayerTypes eToPlayer, CvDeal* pkDeal, DiploUIStateTypes eDiploType, const char* pszMessage, LeaderheadAnimationTypes eAnimationType)
{
#ifdef AUI_DIPLOMACY_AI_LEADERHEAD_DEALS_IN_MULTIPLAYER
	SendRequest(eFromPlayer, eToPlayer, eDiploType, pszMessage, eAnimationType, -1, pkDeal);
#else
	// Deals must currently happen on the active player's turn...
	if(GC.getGame().getActivePlayer() == eToPlayer)
	{
		auto_ptr<ICvDeal1> pDeal = GC.WrapDealPointer(pkDeal);
		GC.GetEngineUserInterface()->SetScratchDeal(pDeal.get());
		SendRequest(eFromPlayer, eToPlayer, eDiploType, pszMessage, eAnimationType, -1);
	}
#endif
}

//	---------------------------------------------------------------------------
//	Have all the AIs do a diplomacy evaluation with the supplied player.
//	Please note that the destination player may not be the active player.
//	static
void CvDiplomacyRequests::DoAIDiplomacy(PlayerTypes eTargetPlayer)
{
	if(eTargetPlayer != NO_PLAYER)
	{
		ICvUserInterface2* pkIFace = GC.GetEngineUserInterface();
		// WARNING: Processing depends on the state of the interface!
		CvAssert(!CvPreGame::isNetworkMultiplayerGame());

		// Don't process while a modal dialog is up or another diplo or popup is up.
		if(pkIFace->IsModalStackEmpty() && !pkIFace->isDiploOrPopupWaiting() && !pkIFace->isCityScreenUp())
		{
			CvPlayer& kTargetPlayer = GET_PLAYER((PlayerTypes) eTargetPlayer);
			CvDiplomacyRequests* pkTargetRequests = kTargetPlayer.GetDiplomacyRequests();
			if(pkTargetRequests)
			{
				if(!pkTargetRequests->HasPendingRequests())
				{
					PlayerTypes ePlayerLoop = pkTargetRequests->GetNextAIPlayer();
					if(ePlayerLoop != NO_PLAYER)
					{
						int iMaxCivPlayers = MAX_CIV_PLAYERS;
						while((int)ePlayerLoop < iMaxCivPlayers)
						{
							CvPlayer& kPlayer = GET_PLAYER(ePlayerLoop);

							if(eTargetPlayer != ePlayerLoop && kPlayer.isAlive() && !kPlayer.isHuman() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian())
							{
								kPlayer.GetDiplomacyAI()->DoTurn(eTargetPlayer);
								if(pkTargetRequests->HasPendingRequests())
								{
									ePlayerLoop = (PlayerTypes)((int)ePlayerLoop + 1);
									break;
								}
							}

							ePlayerLoop = (PlayerTypes)((int)ePlayerLoop + 1);
						}

						pkTargetRequests->SetNextAIPlayer((((int)ePlayerLoop >= iMaxCivPlayers)?NO_PLAYER:ePlayerLoop));
					}
				}
			}
		}
	}

}

//	----------------------------------------------------------------------------
void CvDiplomacyRequests::ActiveRequestComplete()
{
	m_bRequestActive = false;
	m_bRequestActiveFromPlayer = NO_PLAYER;
}

//	---------------------------------------------------------------------------
// Return true if the supplied player has an active diplo request with a human.
// The diplo requests are stored on the target player, so we have to check each player
// Overall, this really only needs to check the active player, since this is not currently valid in MP
// but it will be one less thing to change if AI initiated diplo is ever added to MP.
//static 
bool CvDiplomacyRequests::HasActiveDiploRequestWithHuman(PlayerTypes eSourcePlayer)
{
	for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		CvPlayer& kTargetPlayer = GET_PLAYER((PlayerTypes)i);
		if (kTargetPlayer.isHuman() && kTargetPlayer.isAlive() && (PlayerTypes)i != eSourcePlayer)
		{
			if (kTargetPlayer.GetDiplomacyRequests()->HasActiveRequestFrom(eSourcePlayer))
				return true;
		}
	}
	return false;
}
