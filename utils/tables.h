//Table that includes info for groupings of info (ie Loot and the faction it belongs to)
#ifndef TABLES_H
#define TABLES_H

#include <string>
#include <utility>
#include <span>

inline std::string getDisplayName(const std::string& internalName, std::span<const std::pair<std::string, std::string>> lootTable) {
    for (const auto& item : lootTable) {
        if (item.second == internalName) {
            return item.first; // Found it, return the display name
        }
    }

    // If the loop completes without finding a match, return an empty string
    return "";
}

namespace Tables {
    //feel free to change but gets marked with special color
    inline std::pair<std::string, std::string> goodLoot[] = {
            // --- Top Tier / World Event Loot ---
    {"Chest of Fortune", "BP_TreasureChest_ItemInfo_ChestofFortune_C"},
    {"Reapers Bounty", "BP_ReapersBounty_ItemInfo_C"},
    {"Reapers Bounty (FoTD)", "BP_FortReapersBountyChest_ItemInfo_C"},
    {"Chest of Legends", "BP_TreasureChest_ItemInfo_PirateLegend_C"},
    {"Ashen Chest of Legends", "BP_TreasureChest_ItemInfo_PirateLegend_DVR_C"},
    {"Box of Wondrous Secrets", "BP_Wondrous_ItemInfo_C"},
    {"Ashen Winds Skull", "BP_AshenWindsSkull_ItemInfo_C"},
    {"Chest of Ancient Tributes", "BP_TreasureChest_Vault_ItemInfo_C"},
    {"Stronghold Chest", "BP_TreasureChest_ItemInfo_Fort_C"},
    {"Stronghold Skull", "BP_BountyRewardSkullItemInfo_Fort_C"},

    // --- Reaper's Chests & Cursed Chests ---
    {"Reapers Chest", "BP_ReapersChest_ItemInfo_C"},
    {"Reapers Chest (FoTD)", "BP_FortReapersChest_ItemInfo_C"},
    {"Chest of Rage", "BP_TreasureChest_ItemInfo_ChestOfRage_C"},
    {"Chest of Sorrow", "BP_TreasureChest_ItemInfo_Weeping_C"},
    {"Chest of Everlasting Sorrow", "BP_TreasureChest_ItemInfo_EverlastingSorrow_C"},

    // --- High Value Athena & Faction Loot ---
    {"Offering of Legendary Goods", "BP_trs_box_leg_01_a_ItemInfo_C"},
    {"Crate of Legendary Voyages", "BP_MerchantCrate_CommonPirateLegend_ItemInfo_C"},
    {"Villainous Skull of Ancient Fortune", "BP_BountyRewardSkullItemInfo_PirateLegendUncommon_C"},
    {"Revered Merchant Manifest", "BP_MerchantManifest_01d_ItemInfo_C"},
    {"Magma Grail", "BP_TreasureArtifact_ItemInfo_DVR_Mythical_C"},
    {"Wrathfull King's Bounty Skull", "BP_BountyRewardSkullItemInfo_OnDemand_SRank_C"},

    // --- Valuable Gems & Quest Items ---
    {"Ruby Breath of the Sea", "BP_SK_SirenEssence_Ruby_ItemInfo_C"},
    {"Emerald Breath of the Sea", "BP_SK_SirenEssence_Emerald_ItemInfo_C"},
    {"Sapphire Breath of the Sea", "BP_SK_SirenEssence_Sapphire_ItemInfo_C"},
    {"Ritual Skull", "BP_Ritual_Skull_ItemInfo_C"},
    {"Skull of Destiny", "BP_SkullOfDestiny_ItemInfo"},

    // --- All Emissary Flags (valuable to Reapers) ---
    {"Order of Souls Flag Lvl5", "BP_EmissaryFlotsam_OrderOfSouls_Rank5_ItemInfo_C"},
    {"Order of Souls Flag Lvl4", "BP_EmissaryFlotsam_OrderOfSouls_Rank4_ItemInfo_C"},
    {"Order of Souls Flag Lvl3", "BP_EmissaryFlotsam_OrderOfSouls_Rank3_ItemInfo_C"},
    {"Order of Souls Flag Lvl2", "BP_EmissaryFlotsam_OrderOfSouls_Rank2_ItemInfo_C"},
    {"Order of Souls Flag Lvl1", "BP_EmissaryFlotsam_OrderOfSouls_Rank1_ItemInfo_C"},
    {"Gold Hoarder Flag Lvl5", "BP_EmissaryFlotsam_GoldHoarders_Rank5_ItemInfo_C"},
    {"Gold Hoarder Flag Lvl4", "BP_EmissaryFlotsam_GoldHoarders_Rank4_ItemInfo_C"},
    {"Gold Hoarder Flag Lvl3", "BP_EmissaryFlotsam_GoldHoarders_Rank3_ItemInfo_C"},
    {"Gold Hoarder Flag Lvl2", "BP_EmissaryFlotsam_GoldHoarders_Rank2_ItemInfo_C"},
    {"Gold Hoarder Flag Lvl1", "BP_EmissaryFlotsam_GoldHoarders_ItemInfo_C"},
    {"Reaper's Bones Flag Lvl5", "BP_EmissaryFlotsam_Reapers_Rank5_ItemInfo_C"},
    {"Reaper's Bones Flag Lvl4", "BP_EmissaryFlotsam_Reapers_Rank4_ItemInfo_C"},
    {"Reaper's Bones Flag Lvl3", "BP_EmissaryFlotsam_Reapers_Rank3_ItemInfo_C"},
    {"Reaper's Bones Flag Lvl2", "BP_EmissaryFlotsam_Reapers_Rank2_ItemInfo_C"},
    {"Reaper's Bones Flag Lvl1", "BP_EmissaryFlotsam_Reapers_ItemInfo_C"},
    {"Athena's Fortune Flag Lvl5", "BP_EmissaryFlotsam_AthenasFortune_Rank5_ItemInfo_C"},
    {"Athena's Fortune Flag Lvl4", "BP_EmissaryFlotsam_AthenasFortune_Rank4_ItemInfo_C"},
    {"Athena's Fortune Flag Lvl3", "BP_EmissaryFlotsam_AthenasFortune_Rank3_ItemInfo_C"},
    {"Athena's Fortune Flag Lvl2", "BP_EmissaryFlotsam_AthenasFortune_Rank2_ItemInfo_C"},
    {"Athena's Fortune Flag Lvl1", "BP_EmissaryFlotsam_AthenasFortune_ItemInfo_C"},
    {"Merchant Alliance Flag Lvl5", "BP_EmissaryFlotsam_MerchantAlliance_Rank5_ItemInfo_C"},
    {"Merchant Alliance Flag Lvl4", "BP_EmissaryFlotsam_MerchantAlliance_Rank4_ItemInfo_C"},
    {"Merchant Alliance Flag Lvl3", "BP_EmissaryFlotsam_MerchantAlliance_Rank3_ItemInfo_C"},
    {"Merchant Alliance Flag Lvl2", "BP_EmissaryFlotsam_MerchantAlliance_Rank2_ItemInfo_C"},
    {"Merchant Alliance Flag Lvl1", "BP_EmissaryFlotsam_MerchantAlliance_ItemInfo_C"}
    };

    inline std::pair<std::string, std::string> projectiles[] = {
        {"Cannonball Projectile", "BP_Projectile_CannonBall_C"},
        {"Siren Trident Projectile", "BP_Projectile_SirenTrident_C"},
        {"Siren Song Projectile", "BP_Projectile_SirenSong_C"}
    };

    //good stuff thats not loot
    inline std::pair<std::string, std::string> goodItems[] = {
        // --- Weapons ---
        {"Trident of Dark Tides", "BP_SirenTrident_ItemInfo_C"},
        {"Siren Trident", "BP_SirenTridentSellable_ItemInfo_C"},

        // --- Keys & Quest Progression Items ---
        {"FoTD Key", "BP_FotD_StrongholdKey_ItemInfo_C"},
        {"Fort of Fortune Key", "BP_LegendaryFort_StrongholdKey_ItemInfo_C"},
        {"Captain's Key", "BP_MA_CabinDoorKey_ItemInfo_C"},
        {"Shipwreck Graveyard Key", "ShipWreckGraveyardKeyItemInfo_C"},
        {"Sea Fort Key", "BP_SeaFort_Key_Vault_ItemInfo_C"},
        {"Sea Fort Store Room Key", "BP_SeaFort_Key_StoreRoom_ItemInfo_C"},
        {"Stronghold Key", "BP_StrongholdKey_ItemInfo_C"},
        {"Ashen Key", "BP_AshenKey_ItemInfo_C"},
        {"Old Sailor's Key", "BP_OldKey_Goblet2_ItemInfo_C"},
        {"Krakens Fall Gold Key", "BP_Totem_GoldShark_ItemInfo_C"},
        {"Devils Ridge Gold Key", "BP_Totem_GoldBoar_ItemInfo_C"},
        {"Crescent Isle Gold Key", "BP_Totem_GoldMoon_ItemInfo_C"},
        {"Mermaids Hideaway Gold Key", "BP_Totem_GoldSnake_ItemInfo_C"},
        {"Crooks Hollow Gold Key", "BP_Totem_GoldScarab_ItemInfo_C"},
        {"N-13 Gold Key", "BP_Totem_GoldCrab_ItemInfo_C"},
        {"Fletcher's Rest Gold Key", "BP_Totem_GoldEagle_ItemInfo_C"},
        {"Ashen Reaches Gold Key", "BP_Totem_GoldSun_ItemInfo_C"},
        {"Shark Medallion", "BP_Medallion_Shark_ItemInfo_C"},
        {"Boar Medallion", "BP_Medallion_Boar_ItemInfo_C"},
        {"Moon Medallion", "BP_Medallion_Moon_ItemInfo_C"},
        {"Snake Medallion", "BP_Medallion_Snake_ItemInfo_C"},
        {"Scarab Medallion", "BP_Medallion_Scarab_ItemInfo_C"},
        {"Crab Medallion", "BP_Medallion_Crab_ItemInfo_C"},
        {"Eagle Medallion", "BP_Medallion_Eagle_ItemInfo_C"},
        {"Sun Medallion", "BP_Medallion_Sun_ItemInfo_C"},

        // --- Utility Crates & Storage ---
        {"Storage Crate", "BP_MerchantCrate_AnyItemCrate_ItemInfo_C"},
        {"Ammo Chest", "BP_PortableAmmoCrate_ItemInfo_C"},
        {"Storage Crate of the Damned", "BP_MerchantCrate_GhostResourceCrate_ItemInfo_C"},
        {"Cannonball Crate of the Damned", "BP_MerchantCrate_GhostCannonballCrate_ItemInfo_C"},
        {"Ashen Collectors Chest", "BP_AshenChestCollectorsChest_ItemInfo_C"},
        {"Collectors Chest", "BP_CollectorsChest_ItemInfo_C"},

        //Kegs
        {"Keg of Ancient Black Powder", "BP_MerchantCrate_PirateLegendBigGunpowderBarrel_ItemInfo_C"},
        {"Stronghold Gunpowder Barrel", "BP_MerchantCrate_BigGunpowderBarrel_ItemInfo_C"},
    };

    inline std::pair<std::string, std::string> enemyEntity[] = {
        {"Ashen Lord", "BP_GiantSkeletonPawnBase_C"},
        {"Megalodon", "BP_TinyShark_C"},
        {"Megalodon Respawn Zone", "BP_TinySharkExperience_C"},
        {"Skeleton", "BP_SkeletonPawnBase_C"},
        {"Shark", "BP_Shark_C"},
        {"Siren", "BP_SirenGruntPawn_C"},
        {"Siren Leader", "BP_SirenLeaderPawn_C"},
        {"Phantom", "BP_PhantomPawnBase_C"},
        {"Hermit", "BP_OceanCrawlerCharacter_Hermit_C"},
        {"Eelectric", "BP_OceanCrawlerCharacter_Eelectric_C"},
        {"Crab", "BP_OceanCrawlerCharacter_Crab_C"}
    };


    inline std::pair<std::string, std::string> islandnames[] = {
        // Shores of Plenty
        {"Sailor's Bounty", "wsp_feature_sailors_bounty"},
        {"Smuggler's Bay", "wsp_feature_smugglers_bay"},
        {"Cannon Cove", "wsp_feature_cannon_cove"},
        {"Lonely Isle", "wsp_feature_lonely_isle"},
        {"Mermaid's Hideaway", "wsp_feature_mermaids_hideaway"},
        {"Wanderers Refuge", "wsp_feature_wanderers_refuge"},

        // Ancient Isles
        {"Crook's Hollow", "wsp_feature_crooks_hollow"},
        {"Thieves' Haven", "wsp_feature_thieves_haven"},
        {"Sharkbait Cove", "wsp_feature_sharkbait_cove"},
        {"Plunder Valley", "wsp_feature_plunder_valley"},
        {"Paradise Spring", "wsp_feature_paradise_spring"},

        // The Wilds
        {"Marauder's Arch", "wsp_feature_marauders_arch"},
        {"Kraken's Fall", "wsp_feature_krakens_fall"},
        {"Blind Man's Lagoon", "wsp_feature_blind_mans_lagoon"},
        {"The Crooked Masts", "wsp_feature_the_crooked_masts"},
        {"Galleon's Grave Outpost", "wsp_feature_galleons_grave_outpost"},

        // The Devil's Roar
        {"The Devil's Thirst", "wsp_feature_the_devils_thirst"},
        {"Fetcher's Rest", "wsp_feature_fetchers_rest"},
        {"Ruby's Fall", "wsp_feature_rubys_fall"},
        {"Flintlock Peninsula", "wsp_feature_flintlock_peninsula"},
        {"Molten Sands Fortress", "wsp_feature_molten_sands_fortress"},

        // Outposts
        {"Dagger Tooth Outpost", "wsp_outpost_dagger_tooth"},
        {"Galleon's Grave Outpost", "wsp_outpost_galleons_grave"},
        {"Golden Sands Outpost", "wsp_outpost_golden_sands"},
        {"Sanctuary Outpost", "wsp_outpost_sanctuary"},
        {"Ancient Spire Outpost", "wsp_outpost_ancient_spire"},
        {"Plunder Outpost", "wsp_outpost_plunder"},

        // Seaposts
        {"The Spoils of Plenty Store", "wsp_seapost_spoils_of_plenty"},
        {"Stephen's Spoils", "wsp_seapost_stephens_spoils"},
        {"Three Paces East Seapost", "wsp_seapost_three_paces_east"},
        {"Brian's Bazaar", "wsp_seapost_brians_bazaar"},
        {"The Wild Treasures Store", "wsp_seapost_wild_treasures_store"},
        {"Merrick's Seapost", "wsp_seapost_merricks_seapost"}
    };

    inline std::pair<std::string, std::string> emmisaryFlags[] = {
        {"Order of Souls Flag Lvl5", "BP_EmissaryFlotsam_OrderOfSouls_Rank5_ItemInfo_C"},
        {"Order of Souls Flag Lvl4", "BP_EmissaryFlotsam_OrderOfSouls_Rank4_ItemInfo_C"},
        {"Order of Souls Flag Lvl3", "BP_EmissaryFlotsam_OrderOfSouls_Rank3_ItemInfo_C"},
        {"Order of Souls Flag Lvl2", "BP_EmissaryFlotsam_OrderOfSouls_Rank2_ItemInfo_C"},
        {"Order of Souls Flag Lvl1", "BP_EmissaryFlotsam_OrderOfSouls_Rank1_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl5", "BP_EmissaryFlotsam_GoldHoarders_Rank5_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl4", "BP_EmissaryFlotsam_GoldHoarders_Rank4_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl3", "BP_EmissaryFlotsam_GoldHoarders_Rank3_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl2", "BP_EmissaryFlotsam_GoldHoarders_Rank2_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl1", "BP_EmissaryFlotsam_GoldHoarders_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl5", "BP_EmissaryFlotsam_Reapers_Rank5_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl4", "BP_EmissaryFlotsam_Reapers_Rank4_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl3", "BP_EmissaryFlotsam_Reapers_Rank3_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl2", "BP_EmissaryFlotsam_Reapers_Rank2_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl1", "BP_EmissaryFlotsam_Reapers_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl5", "BP_EmissaryFlotsam_AthenasFortune_Rank5_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl4", "BP_EmissaryFlotsam_AthenasFortune_Rank4_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl3", "BP_EmissaryFlotsam_AthenasFortune_Rank3_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl2", "BP_EmissaryFlotsam_AthenasFortune_Rank2_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl1", "BP_EmissaryFlotsam_AthenasFortune_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl5", "BP_EmissaryFlotsam_MerchantAlliance_Rank5_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl4", "BP_EmissaryFlotsam_MerchantAlliance_Rank4_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl3", "BP_EmissaryFlotsam_MerchantAlliance_Rank3_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl2", "BP_EmissaryFlotsam_MerchantAlliance_Rank2_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl1", "BP_EmissaryFlotsam_MerchantAlliance_ItemInfo_C"},
    };

    inline std::pair<std::string, std::string> gold_hoarders_loot[] = {
        {"Magma Grail", "BP_TreasureArtifact_ItemInfo_DVR_Mythical_C"},
        {"Devil's Remnant", "BP_TreasureArtifact_ItemInfo_DVR_Legendary_C"},
        {"Opulent Curio (Vault)", "BP_TreasureArtifact_Vault_impressive_01_a_ItemInfo_C"},
        {"Opulent Curio", "BP_TreasureArtifact_ItemInfo_impressive_01_a_C"},
        {"Adorned Receptacle (Vault)", "BP_TreasureArtifact_Vault_impressive_02_a_ItemInfo_C"},
        {"Adorned Receptacle", "BP_TreasureArtifact_ItemInfo_impressive_02_a_C"},
        {"Peculiar Relic (Vault)", "BP_TreasureArtifact_Vault_impressive_03_a_ItemInfo_C"},
        {"Peculiar Relic", "BP_TreasureArtifact_ItemInfo_impressive_03_a_C"},
        {"Peculiar Coral Relic", "BP_SKCoralTrinket_Mythical_ItemInfo_C"},
        {"Brimstone Casket", "BP_TreasureArtifact_ItemInfo_DVR_Rare_C"},
        {"Golden Reliquary (Vault)", "BP_TreasureArtifact_Vault_box_03_a_ItemInfo_C"},
        {"Golden Reliquary", "BP_TreasureArtifact_ItemInfo_box_03_a_C"},
        {"Golden Coral Reliquary", "BP_SKCoralTrinket_Legendary_ItemInfo_C"},
        {"Gilded Chalice (Vault)", "BP_TreasureArtifact_Vault_goblet_03_a_ItemInfo_C"},
        {"Gilded Chalice", "BP_TreasureArtifact_ItemInfo_goblet_03_a_C"},
        {"Ornate Carafe (Vault)", "BP_TreasureArtifact_Vault_vase_03_a_ItemInfo_C"},
        {"Ornate Carafe", "BP_TreasureArtifact_ItemInfo_vase_03_a_C"},
        {"Roaring Goblet", "BP_TreasureArtifact_ItemInfo_DVR_Common_C"},
        {"Silvered Cup (Vault)", "BP_TreasureArtifact_Vault_goblet_02_a_ItemInfo_C"},
        {"Silvered Cup", "BP_TreasureArtifact_ItemInfo_goblet_02_a_C"},
        {"Silvered Coral Cup", "BP_SKCoralTrinket_Rare_ItemInfo_C"},
        {"Elaborate Flagon (Vault)", "BP_TreasureArtifact_Vault_vase_02_a_ItemInfo_C"},
        {"Elaborate Flagon", "BP_TreasureArtifact_ItemInfo_vase_02_a_C"},
        {"Decorative Coffer (Vault)", "BP_TreasureArtifact_Vault_box_02_a_ItemInfo_C"},
        {"Decorative Coffer", "BP_TreasureArtifact_ItemInfo_box_02_a_C"},
        {"Mysterious Vessel (Vault)", "BP_TreasureArtifact_Vault_vase_01_a_ItemInfo_C"},
        {"Mysterious Vessel", "BP_TreasureArtifact_ItemInfo_vase_01_a_C"},
        {"Mysterious Coral Vessel", "BP_SKCoralTrinket_Common_ItemInfo_C"},
        {"Bronze Secret-Keeper (Vault)", "BP_TreasureArtifact_Vault_box_01_a_ItemInfo_C"},
        {"Bronze Secret-Keeper", "BP_TreasureArtifact_ItemInfo_box_01_a_C"},
        {"Ancient Goblet (Vault)", "BP_TreasureArtifact_Vault_goblet_01_a_ItemInfo_C"},
        {"Ancient Goblet", "BP_TreasureArtifact_ItemInfo_goblet_01_a_C"},
        {"Stronghold Chest", "BP_TreasureChest_ItemInfo_Fort_C"},
        {"Chest of Ancient Tributes", "BP_TreasureChest_Vault_ItemInfo_C"},
        {"Chest of the Damned", "BP_TreasureChest_ItemInfo_Ghost_C"},
        {"Skeleton Captain's Chest", "BP_TreasureChest_ItemInfo_AIShip_C"},
        {"Ashen Captain's Chest", "BP_TreasureChest_ItemInfo_Mythical_DVR_C"},
        {"Shipwrecked Captain's Chest", "BP_ShipwreckTreasureChest_ItemInfo_Mythical_C"},
        {"Chest of a Thousand Grogs", "BP_TreasureChest_ItemInfo_Drunken_C"},
        {"Chest of Rage", "BP_TreasureChest_ItemInfo_ChestOfRage_C"},
        {"Chest of Sorrow", "BP_TreasureChest_ItemInfo_Weeping_C"},
        {"Chest of Everlasting Sorrow", "BP_TreasureChest_ItemInfo_EverlastingSorrow_C"},
        {"Captain's Chest (Vault)", "BP_TreasureChest_Vault_Mythical_ItemInfo_C"},
        {"Captain's Chest", "BP_TreasureChest_ItemInfo_Mythical_C"},
        {"Coral Captain's Chest", "BP_SK_CoralChest_ItemInfo_Mythical_C"},
        {"Ashen Marauder's Chest", "BP_TreasureChest_ItemInfo_Legendary_DVR_C"},
        {"Shipwrecked Marauder's Chest", "BP_ShipwreckTreasureChest_ItemInfo_Legendary_C"},
        {"Marauder's Chest (Vault)", "BP_TreasureChest_Vault_Legendary_ItemInfo_C"},
        {"Marauder's Chest", "BP_TreasureChest_ItemInfo_Legendary_C"},
        {"Coral Marauder Chest", "BP_SK_CoralChest_ItemInfo_Legendary_C"},
        {"Ashen Seafarer's Chest", "BP_TreasureChest_ItemInfo_Rare_DVR_C"},
        {"Shipwrecked Seafarer's Chest", "BP_ShipwreckTreasureChest_ItemInfo_Rare_C"},
        {"Seafarer's Chest (Vault)", "BP_TreasureChest_Vault_Rare_ItemInfo_C"},
        {"Seafarer's Chest", "BP_TreasureChest_ItemInfo_Rare_C"},
        {"Coral Seafarer's Chest", "BP_SK_CoralChest_ItemInfo_Rare_C"},
        {"Ashen Castaway's Chest", "BP_TreasureChest_ItemInfo_Common_DVR_C"},
        {"Shipwrecked Castaway's Chest", "BP_ShipwreckTreasureChest_ItemInfo_Common_C"},
        {"Castaway's Chest (Vault)", "BP_TreasureChest_Vault_Common_ItemInfo_C"},
        {"Castaway's Chest", "BP_TreasureChest_ItemInfo_Common_C"},
        {"Coral Castaway Chest", "BP_SK_CoralChest_ItemInfo_Common_C"},
        {"Vestige of Power", "BP_TreasureArtifact_ItemInfo_OnDemand_Tier01_C"},
        {"Sailor's Chest", "BP_TreasureChest_ItemInfo_OnDemand_Traditional_Tier01_C"},
        {"Ashen King's Chest", "BP_TreasureChest_ItemInfo_OnDemand_XMarks_DVRSRank_C"}
    };

    inline std::pair<std::string, std::string> order_of_souls_loot[] = {
        {"Ashen King's Bounty Skull", "BP_BountyRewardSkullItemInfo_OnDemand_DVRSRank_C"},
        {"Wrathfull King's Bounty Skull", "BP_BountyRewardSkullItemInfo_OnDemand_SRank_C"},
        {"Ashen Winds Skull", "BP_AshenWindsSkull_ItemInfo_C"},
        {"Stronghold Skull", "BP_BountyRewardSkullItemInfo_Fort_C"},
        {"Captain Skull of the Damned", "BP_BountyRewardSkullItemInfo_Ghost_Captain_C"},
        {"Skull of the Damned", "BP_BountyRewardSkullItemInfo_Ghost_Common_C"},
        {"Skeleton Captain's Skull", "BP_BountyRewardSkullItemInfo_AIShip_C"},
        {"Ashen Villainous Bounty Skull", "BP_BountyRewardSkullItemInfo_Mythical_DVR_C"},
        {"Villainous Bounty Skull", "BP_BountyRewardSkullItemInfo_Mythical_C"},
        {"Villainous Coral Skull", "BP_SKLostCapSkullItemInfo_Mythical_C"},
        {"Ashen Hateful Bounty Skull", "BP_BountyRewardSkullItemInfo_Legendary_DVR_C"},
        {"Hateful Bounty Skull", "BP_BountyRewardSkullItemInfo_Legendary_C"},
        {"Hateful Coral Skull", "BP_SKLostCapSkullItemInfo_Legendary_C"},
        {"Ashen Disgraced Bounty Skull", "BP_BountyRewardSkullItemInfo_Rare_DVR_C"},
        {"Disgraced Bounty Skull", "BP_BountyRewardSkullItemInfo_Rare_C"},
        {"Disgraced Coral Skull", "BP_SKLostCapSkullItemInfo_Rare_C"},
        {"Ashen Foul Bounty Skull", "BP_BountyRewardSkullItemInfo_Common_DVR_C"},
        {"Foul Bounty Skull", "BP_BountyRewardSkullItemInfo_Common_C"},
        {"Foul Coral Skull", "BP_SKLostCapSkullItemInfo_Common_C"},
        {"Ritual Skull", "BP_Ritual_Skull_ItemInfo_C"},
        {"Skull of Destiny", "BP_SkullOfDestiny_ItemInfo"}
    };

    inline std::pair<std::string, std::string> merchants_loot[] = {
        {"Logbook", "BP_CaptainsLog_Proxy_C"},
        {"Snake Basket", "BP_MerchantCrate_SnakeBasket_ItemInfo_C"},
        {"Chicken Coop", "BP_MerchantCrate_ChickenCrate_C"},
        {"Pig Crate", "BP_MerchantCrate_PigCrate_ItemInfo_C"},
        {"Stronghold Gunpowder Barrel", "BP_MerchantCrate_BigGunpowderBarrel_ItemInfo_C"},
        {"Gunpowder Barrel", "BP_MerchantCrate_GunpowderBarrel_ItemInfo_C"},
        {"Ashes of the Damned", "BP_MerchantCrate_Commodity_GhostCrate_ItemInfo_C"},
        {"Prosperous Manifest", "BP_MerchantManifest_01a_ItemInfo_C"},
        {"Esteemed Manifest", "BP_MerchantManifest_01b_ItemInfo_C"},
        {"Eminent Manifest", "BP_MerchantManifest_01c_ItemInfo_C"},
        {"Revered Merchant Manifest", "BP_MerchantManifest_01d_ItemInfo_C"},
        {"Crate of Ancient Bone Dust", "BP_MerchantCrate_Commodity_Fort_ItemInfo_C"},
        {"Crate of Precious Gemstones", "BP_MerchantCrate_Commodity_Gemstones_ItemInfo_C"},
        {"Crate of Minerals", "BP_MerchantCrate_Commodity_Minerals_ItemInfo_C"},
        {"Casket of Forgotten Jewels", "BP_SKMerchantCommodity_ForgottenJewels_ItemInfo_C"},
        {"Casket of Antiquated Coffee", "BP_SKMerchantCommodity_AntiCoffee_ItemInfo_C"},
        {"Crate of Exquisite Spices", "BP_MerchantCrate_Commodity_SpiceCrate_ItemInfo_C"},
        {"Crate of Fine Ore", "BP_MerchantCrate_Commodity_Ore_ItemInfo_C"},
        {"Crate of Exotic Silks", "BP_MerchantCrate_Commodity_SilkCrate_ItemInfo_C"},
        {"Crate of Volcanic Stone", "BP_MerchantCrate_Commodity_VolcanicStone_ItemInfo_C"},
        {"Crate of Rare Tea", "BP_MerchantCrate_Commodity_TeaCrate_ItemInfo_C"},
        {"Crate of Fine Sugar", "BP_MerchantCrate_Commodity_SugarCrate_ItemInfo_C"},
        {"Crate of Unclassified Gemstones", "BP_UnsortedCommodity_Gemstones_ItemInfo_C"},
        {"Crate of Unfiltered Minerals", "BP_UnsortedCommodity_Minerals_ItemInfo_C"},
        {"Crate of Broken Stone", "BP_UnsortedCommodity_Stone_ItemInfo_C"},
        {"Crate of Unrefined Spices", "BP_UnsortedCommodity_Spices_ItemInfo_C"},
        {"Crate of Unsorted Silks", "BP_UnsortedCommodity_Silks_ItemInfo_C"},
        {"Crate of Ungraded Tea", "BP_UnsortedCommodity_Tea_ItemInfo_C"},
        {"Crate of Raw Sugar", "BP_UnsortedCommodity_Sugar_ItemInfo_C"},
        {"Crate of Luxurious Cloth", "BP_CargoRunCrate_Cloth_ItemInfo_C"},
        {"Crate of Plants", "BP_CargoRunCrate_Plants_ItemInfo_C"},
        {"Crate of Rum", "BP_CargoRunCrate_Rum_ItemInfo_C"},
        {"Storage Crate of the Damned", "BP_MerchantCrate_GhostResourceCrate_ItemInfo_C"},
        {"Cannonball Crate of the Damned", "BP_MerchantCrate_GhostCannonballCrate_ItemInfo_C"}
    };

    inline std::pair<std::string, std::string> reapers_loot[] = {
        {"Order of Souls Flag Lvl5", "BP_EmissaryFlotsam_OrderOfSouls_Rank5_ItemInfo_C"},
        {"Order of Souls Flag Lvl4", "BP_EmissaryFlotsam_OrderOfSouls_Rank4_ItemInfo_C"},
        {"Order of Souls Flag Lvl3", "BP_EmissaryFlotsam_OrderOfSouls_Rank3_ItemInfo_C"},
        {"Order of Souls Flag Lvl2", "BP_EmissaryFlotsam_OrderOfSouls_Rank2_ItemInfo_C"},
        {"Order of Souls Flag Lvl1", "BP_EmissaryFlotsam_OrderOfSouls_Rank1_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl5", "BP_EmissaryFlotsam_GoldHoarders_Rank5_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl4", "BP_EmissaryFlotsam_GoldHoarders_Rank4_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl3", "BP_EmissaryFlotsam_GoldHoarders_Rank3_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl2", "BP_EmissaryFlotsam_GoldHoarders_Rank2_ItemInfo_C"},
        {"Gold Hoarder Flag Lvl1", "BP_EmissaryFlotsam_GoldHoarders_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl5", "BP_EmissaryFlotsam_Reapers_Rank5_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl4", "BP_EmissaryFlotsam_Reapers_Rank4_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl3", "BP_EmissaryFlotsam_Reapers_Rank3_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl2", "BP_EmissaryFlotsam_Reapers_Rank2_ItemInfo_C"},
        {"Reaper's Bones Flag Lvl1", "BP_EmissaryFlotsam_Reapers_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl5", "BP_EmissaryFlotsam_AthenasFortune_Rank5_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl4", "BP_EmissaryFlotsam_AthenasFortune_Rank4_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl3", "BP_EmissaryFlotsam_AthenasFortune_Rank3_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl2", "BP_EmissaryFlotsam_AthenasFortune_Rank2_ItemInfo_C"},
        {"Athena's Fortune Flag Lvl1", "BP_EmissaryFlotsam_AthenasFortune_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl5", "BP_EmissaryFlotsam_MerchantAlliance_Rank5_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl4", "BP_EmissaryFlotsam_MerchantAlliance_Rank4_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl3", "BP_EmissaryFlotsam_MerchantAlliance_Rank3_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl2", "BP_EmissaryFlotsam_MerchantAlliance_Rank2_ItemInfo_C"},
        {"Merchant Alliance Flag Lvl1", "BP_EmissaryFlotsam_MerchantAlliance_ItemInfo_C"},
        {"Reapers Bounty", "BP_ReapersBounty_ItemInfo_C"},
        {"Reapers Bounty (FoTD)", "BP_FortReapersBountyChest_ItemInfo_C"},
        {"Reapers Chest", "BP_ReapersChest_ItemInfo_C"},
        {"Reapers Chest (FoTD)", "BP_FortReapersChest_ItemInfo_C"},
        {"Generous Gift", "BP_HighValueGift_ItemInfo_C"},
        {"Humble Gift", "BP_LowValueGift_ItemInfo_C"}
    };

    inline std::pair<std::string, std::string> athena_loot[]{
        // Athena's Fortune & Special Loot
        {"Box of Wondrous Secrets", "BP_Wondrous_ItemInfo_C"},
        {"Ashen Chest of Legends", "BP_TreasureChest_ItemInfo_PirateLegend_DVR_C"},
        {"Chest of Legends", "BP_TreasureChest_ItemInfo_PirateLegend_C"},
        {"Offering of Legendary Goods", "BP_trs_box_leg_01_a_ItemInfo_C"},
        {"Artifact of Legendary Hunger", "BP_trs_dark_shark_leg_01_a_ItemInfo_C"},
        {"Athena's Relic", "BP_trs_impressive_leg_01_a_ItemInfo_C"},
        {"Jar of Athena's Incense", "BP_trs_jar_leg_01_a_ItemInfo_C"},
        {"Legendary Fortune Keeper", "BP_trs_jewellery_box_leg_01_a_ItemInfo_C"},
        {"Stone of Ancients", "BP_PL_StoneOfAncients_ItemInfo_C"},
        {"Skull of Athena's Blessing", "BP_trs_leg_crain_leg_01_a_C"},
        {"Keg of Ancient Black Powder", "BP_MerchantCrate_PirateLegendBigGunpowderBarrel_ItemInfo_C"},
        {"Chest of Fortune", "BP_TreasureChest_ItemInfo_ChestofFortune_C"},
        {"Villainous Skull of Ancient Fortune", "BP_BountyRewardSkullItemInfo_PirateLegendUncommon_C"},
        {"Villainous Skull of Ancient_Fortune", "BP_BountyRewardSkull_UncommonPirateLegend_C"},
        {"Skull of Ancient Fortune", "BP_BountyRewardSkull_PirateLegend_C"},
        {"Gilded Relic of Ancient Fortune", "BP_TreasureArtifact_ItemInfo_piratelegendimpressive_03_a_C"},
        {"Crate of Legendary Voyages", "BP_MerchantCrate_CommonPirateLegend_ItemInfo_C"},
        {"Chalice of Ancient Fortune", "BP_TreasureArtifact_ItemInfo_piratelegend_goblet_02_a_C"},
    };

    inline std::pair<std::string, std::string> useless_garbage[]{
        {"Sea Fort Gold Pouch", "BP_SeaFortGoldPouches_ItemInfo_C"},
        {"Phantom Gold Pouch", "BP_GoldPouches_Phantom_ItemInfo_C"},
        {"Gold Pouch", "BP_GoldPouches_ItemInfo_C"},
        {"Pieces O Eight Pouch", "BP_Mojo_PiecesOEightPouch_Collectable_C"},
        {"Pieces O Eight Coin", "BP_Mojo_PiecesOEightCoin_Collectable%"},
        {"Stool", "BP_StoolItem_Proxy_C"},
        {"Fireworks Rocket", "BP_gmp_fireworks_rocket_%_ItemDesc_C"},
        {"Fireworks Flare", "BP_gmp_fireworks_flare_%_ItemDesc_C"},
        {"Fireworks Cake", "BP_gmp_fireworks_cake%_ItemDesc_C"},
        {"Fireworks", "BP_gmp_fireworks_%_ItemDesc_C"}
    };

    inline std::pair<std::string, std::string> anyFactionLoot[]{
        {"Ruby Mermaid Gem", "BP_MermaidGem_ItemInfo_Ruby_C"},
        {"Ruby Siren Gem", "BP_SirenGem_ItemInfo_Ruby_C"},
        {"Ruby Breath of the Sea", "BP_SK_SirenEssence_Ruby_ItemInfo_C"},
        {"Emerald Mermaid Gem", "BP_MermaidGem_ItemInfo_Emerald_C"},
        {"Emerald Siren Gem", "BP_SirenGem_ItemInfo_Emerald_C"},
        {"Emerald Breath of the Sea", "BP_SK_SirenEssence_Emerald_ItemInfo_C"},
        {"Sapphire Mermaid Gem", "BP_MermaidGem_ItemInfo_Sapphire_C"},
        {"Sapphire Siren Gem", "BP_SirenGem_ItemInfo_Sapphire_C"},
        {"Sapphire Breath of the Sea", "BP_SK_SirenEssence_Sapphire_ItemInfo_C"},
        {"Trident of Dark Tides", "BP_SirenTrident_ItemInfo_C"},
        {"Siren Trident", "BP_SirenTridentSellable_ItemInfo_C"},
    };

    inline std::pair<std::string, std::string> misc[]{
        {"FoTD Key", "BP_FotD_StrongholdKey_ItemInfo_C"},
        {"Fort of Fortune Key", "BP_LegendaryFort_StrongholdKey_ItemInfo_C"},
        {"Captain's Key", "BP_MA_CabinDoorKey_ItemInfo_C"},
        {"Shipwreck Graveyard Key", "ShipWreckGraveyardKeyItemInfo_C"},
        {"Sea Fort Key", "BP_SeaFort_Key_Vault_ItemInfo_C"},
        {"Sea Fort Store Room Key", "BP_SeaFort_Key_StoreRoom_ItemInfo_C"},
        {"Stronghold Key", "BP_StrongholdKey_ItemInfo_C"},
        {"Ashen Key", "BP_AshenKey_ItemInfo_C"},
        {"Old Sailor's Key", "BP_OldKey_Goblet2_ItemInfo_C"},
        {"Krakens Fall Gold Key", "BP_Totem_GoldShark_ItemInfo_C"},
        {"Devils Ridge Gold Key", "BP_Totem_GoldBoar_ItemInfo_C"},
        {"Crescent Isle Gold Key", "BP_Totem_GoldMoon_ItemInfo_C"},
        {"Mermaids Hideaway Gold Key", "BP_Totem_GoldSnake_ItemInfo_C"},
        {"Crooks Hollow Gold Key", "BP_Totem_GoldScarab_ItemInfo_C"},
        {"N-13 Gold Key", "BP_Totem_GoldCrab_ItemInfo_C"},
        {"Fletcher's Rest Gold Key", "BP_Totem_GoldEagle_ItemInfo_C"},
        {"Ashen Reaches Gold Key", "BP_Totem_GoldSun_ItemInfo_C"},
        {"Shark Medallion", "BP_Medallion_Shark_ItemInfo_C"},
        {"Boar Medallion", "BP_Medallion_Boar_ItemInfo_C"},
        {"Moon Medallion", "BP_Medallion_Moon_ItemInfo_C"},
        {"Snake Medallion", "BP_Medallion_Snake_ItemInfo_C"},
        {"Scarab Medallion", "BP_Medallion_Scarab_ItemInfo_C"},
        {"Crab Medallion", "BP_Medallion_Crab_ItemInfo_C"},
        {"Eagle Medallion", "BP_Medallion_Eagle_ItemInfo_C"},
        {"Sun Medallion", "BP_Medallion_Sun_ItemInfo_C"},
        {"Merchant Quest Bottle", "BP_MessageInABottle_Clue_ItemInfo_C"},
        {"Skeleton's Orders", "BP_SkeletonOrdersRiddle_ItemInfo_C"},
        {"Message in a Bottle", "BP_MessageInABottle_ItemInfo_C"},
        {"Coral Quest Bottle", "BP_MessageInABottle_Coral_ItemInfo_C"},
        {"Emergent LockedAshenChest", "BP_SOTOnDemand_Emergent_XMarks_LockedAshenChest_ItemInfo_C"},
        {"Skeleton Orders", "BP_SOTOnDemand_Emergent_XMarks_SkeletonCaptain_ItemInfo_C"},
        {"Ancient's Brazier", "BP_PL_HauntedBrazier_C"},
        {"Ruby Cursed Mermaid Statue", "BP_SunkenCurseArtefact_Ruby_EasierBetterRewards_C"},
        {"Emerald Cursed Mermaid Statue", "BP_SunkenCurseArtefact_Emerald_EasierBetterRewards_C"},
        {"Sapphire Cursed Mermaid Statue", "BP_SunkenCurseArtefact_Sapphire_EasierBetterRewards_C"},
    };

    inline std::pair<std::string, std::string> nonLootItems[] = {
        {"Ammo Chest", "BP_PortableAmmoCrate_ItemInfo_C"},
        {"Firebomb Crate", "BP_MerchantCrate_FirebombCrate_ItemInfo_C"},
        {"Plank Crate (Purchased)", "BP_MerchantCrate_WoodCrate_FullyStocked_ItemInfo_C"},
        {"Plank Crate", "BP_MerchantCrate_WoodCrate_ItemInfo_C"},
        {"Cannonball Crate (Purchased)", "BP_MerchantCrate_CannonballCrate_FullyStocked_ItemInfo_C"},
        {"Cannonball Crate", "BP_MerchantCrate_CannonballCrate_ItemInfo_C"},
        {"Food Crate (Purchased)", "BP_MerchantCrate_BananaCrate_FullyStocked_ItemInfo_C"},
        {"Food Crate", "BP_MerchantCrate_BananaCrate_ItemInfo_C"},
        {"BaitBox", "BP_MerchantCrate_BaitBox%ItemInfo_C"},
        {"Storage Crate (AI Drop)", "BP_MerchantCrate_AIShipAnyItemCrate_ItemInfo_C"},
        {"Storage Crate", "BP_MerchantCrate_AnyItemCrate_ItemInfo_C"},

        {"Storage Crate of the Damned", "BP_MerchantCrate_GhostResourceCrate_ItemInfo_C"},
        {"Cannonball Crate of the Damned", "BP_MerchantCrate_GhostCannonballCrate_ItemInfo_C"},


        //projectiles
        {"Cannonball Projectile", "BP_Projectile_CannonBall_C"},
        {"Siren Trident Projectile", "BP_Projectile_SirenTrident_C"},
        {"Siren Song Projectile", "BP_Projectile_SirenSong_C"},
    };
}

#endif //TABLES_H
