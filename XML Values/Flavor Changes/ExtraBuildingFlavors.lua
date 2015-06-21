function OnCheckUnmoddedBuildings(iPlayer, iCity, iBuilding, iFlavor)
	local pPlayer = Players[iPlayer];
	if (pPlayer == nil) then
		return 0
	end
	local iExtraFlavor = 0;
    if (iBuilding == GameInfoTypes["BUILDING_PETRA"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_GROWTH"] or iFlavor == GameInfoTypes["FLAVOR_PRODUCTION"]) then
			local pCity = pPlayer:GetCityByID(iCity);
			for i = 0, pCity:GetNumCityPlots() - 1, 1 do
				local pPlot = pCity:GetCityIndexPlot(i);
				if pPlot ~= nil and pPlot:GetTerrainType() == TerrainTypes.TERRAIN_DESERT and (pPlot:getOwner() == -1 or pPlot:getOwner() == iPlayer) then
					if pPlot:GetFeatureType() ~= FeatureTypes.FEATURE_FLOOD_PLAINS and pPlot:IsImpassable() == false then
						iExtraFlavor = iExtraFlavor + 10;
					end
				end
			end
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_MAUSOLEUM_HALICARNASSUS"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_GOLD"]) then
			local pCity = pPlayer:GetCityByID(iCity);
			for i = 0, pCity:GetNumCityPlots() - 1, 1 do
				local pPlot = pCity:GetCityIndexPlot(i);
				if pPlot ~= nil and (pPlot:getOwner() == -1 or pPlot:getOwner() == iPlayer) then
					local resourceType = pPlot:GetResourceType();
					if resourceType == GameInfoTypes["RESOURCE_STONE"] or resourceType == GameInfoTypes["RESOURCE_MARBLE"] then
						iExtraFlavor = iExtraFlavor + 20;
					end
				end
			end
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_STATUE_OF_LIBERTY"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_PRODUCTION"]) then
			for pCity in pPlayer:Cities() do
				iExtraFlavor = iExtraFlavor + 10 * pCity:GetSpecialistCount();
			end
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_NEUSCHWANSTEIN"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_CULTURE"] or iFlavor == GameInfoTypes["FLAVOR_GOLD"] or iFlavor == GameInfoTypes["FLAVOR_HAPPINESS"]) then
			local iCastleCount = pPlayer:CountNumBuildings(GameInfoTypes["BUILDING_CASTLE"]);
			if (iFlavor == GameInfoTypes["FLAVOR_GOLD"]) then
				iExtraFlavor = iExtraFlavor + 30 * iCastleCount;
			elseif (iFlavor == GameInfoTypes["FLAVOR_CULTURE"]) then
				iExtraFlavor = iExtraFlavor + 20 * iCastleCount;
			else
				iExtraFlavor = iExtraFlavor + 10 * iCastleCount;
			end
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_MACHU_PICHU"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_GOLD"]) then
			for pCity in pPlayer:Cities() do
				iExtraFlavor = iExtraFlavor + pPlayer:GetCityConnectionRouteGoldTimes100(pCity) / 10;
			end
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_SISTINE_CHAPEL"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_CULTURE"]) then
			iExtraFlavor = iExtraFlavor + 30 * pPlayer:GetNumCities();
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_CN_TOWER"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_GROWTH"]) then
			iExtraFlavor = iExtraFlavor + 20 * pPlayer:GetNumCities();
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_RED_FORT"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_CITY_DEFENSE"]) then
			iExtraFlavor = iExtraFlavor + 10 * pPlayer:GetNumCities();
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_PRORA"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_HAPPINESS"]) then
			iExtraFlavor = iExtraFlavor + 5 * pPlayer:GetNumPolicies();
		end
	elseif (iBuilding == GameInfoTypes["BUILDING_MONASTERY"]) then
		if (iFlavor == GameInfoTypes["FLAVOR_CULTURE"] or iFlavor == GameInfoTypes["FLAVOR_RELIGION"]) then
			local pCity = pPlayer:GetCityByID(iCity);
			for i = 0, pCity:GetNumCityPlots() - 1, 1 do
				local pPlot = pCity:GetCityIndexPlot(i);
				if pPlot ~= nil and (pPlot:getOwner() == -1 or pPlot:getOwner() == iPlayer) then
					local resourceType = pPlot:GetResourceType();
					if resourceType == GameInfoTypes["RESOURCE_WINE"] or resourceType == GameInfoTypes["RESOURCE_INCENSE"] then
						iExtraFlavor = iExtraFlavor + 10;
					end
				end
			end
		end
	end
	
	return iExtraFlavor;
end
GameEvents.ExtraBuildingFlavor.Add(OnCheckUnmoddedBuildings);