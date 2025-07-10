//Table that includes info for groupings of info (ie Loot and the faction it belongs to)
#ifndef TABLES_H
#define TABLES_H

#include <string>
#include <utility>
#include <span>

/**
 * @brief Finds the display name for a given internal game object name.
 *
 * This function performs a substring search instead of an exact match. This makes it
 * robust against variations in object names, such as network proxies (_Proxy_C) or
 * other suffixes added by the game engine.
 *
 * @param internalName The full internal name of the object from the game.
 * @param nameTable A span of pairs, where `pair.first` is the display name
 *                  and `pair.second` is the simplified, core internal name to search for.
 * @return The corresponding display name if a match is found, otherwise an empty string.
 */
inline std::string getDisplayName(const std::string& internalName, std::span<const std::pair<std::string, std::string>> nameTable) {
    for (const auto& entry : nameTable) {
        // Check if the core name (entry.second) is a substring of the full internalName from the game.
        if (internalName.find(entry.second) != std::string::npos) {
            return entry.first; // Found a match, return the display name.
        }
    }

    // If the loop completes without finding a match, return an empty string.
    return "";
}

namespace Tables {
    // These are items considered high-value and will be marked with a special color.
    // It includes top-tier loot, valuable quest items, and all emissary flags.
    inline std::pair<std::string, std::string> goodLoot[] = {
        // --- Top Tier / World Event Loot ---
        {"Chest of Fortune", "TreasureChest_ItemInfo_ChestofFortune"},
        {"Reapers Bounty", "ReapersBounty_ItemInfo"},
        {"Reapers Bounty (FoTD)", "FortReapersBountyChest_ItemInfo"},
        {"Chest of Legends", "TreasureChest_ItemInfo_PirateLegend"},
        {"Ashen Chest of Legends", "TreasureChest_ItemInfo_PirateLegend_DVR"},
        {"Box of Wondrous Secrets", "Wondrous_ItemInfo"},
        {"Ashen Winds Skull", "AshenWindsSkull_ItemInfo"},
        {"Chest of Ancient Tributes", "TreasureChest_Vault_ItemInfo"},
        {"Stronghold Chest", "TreasureChest_ItemInfo_Fort"},
        {"Stronghold Skull", "BountyRewardSkullItemInfo_Fort"},

        // --- Reaper's Chests & Cursed Chests ---
        {"Reapers Chest", "ReapersChest_ItemInfo"},
        {"Reapers Chest (FoTD)", "FortReapersChest_ItemInfo"},
        {"Chest of Rage", "TreasureChest_ItemInfo_ChestOfRage"},
        {"Chest of Sorrow", "TreasureChest_ItemInfo_Weeping"},
        {"Chest of Everlasting Sorrow", "TreasureChest_ItemInfo_EverlastingSorrow"},

        // --- High Value Athena & Faction Loot ---
        {"Offering of Legendary Goods", "trs_box_leg_01_a_ItemInfo"},
        {"Crate of Legendary Voyages", "MerchantCrate_CommonPirateLegend_ItemInfo"},
        {"Villainous Skull of Ancient Fortune", "BountyRewardSkullItemInfo_PirateLegendUncommon"},
        {"Revered Merchant Manifest", "MerchantManifest_01d_ItemInfo"},
        {"Magma Grail", "TreasureArtifact_ItemInfo_DVR_Mythical"},
        {"Wrathfull King's Bounty Skull", "BountyRewardSkullItemInfo_OnDemand_SRank"},
        {"Keg of Ancient Black Powder", "MerchantCrate_PirateLegendBigGunpowderBarrel"},
        {"Stronghold Gunpowder Barrel", "MerchantCrate_BigGunpowderBarrel"},

        // --- Valuable Gems & Quest Items ---
        {"Ruby Breath of the Sea", "SK_SirenEssence_Ruby_ItemInfo"},
        {"Emerald Breath of the Sea", "SK_SirenEssence_Emerald_ItemInfo"},
        {"Sapphire Breath of the Sea", "SK_SirenEssence_Sapphire_ItemInfo"},
        {"Ritual Skull", "Ritual_Skull_ItemInfo"},
        {"Skull of Destiny", "SkullOfDestiny_ItemInfo"},

        // --- Keys ---
        {"FoTD Key", "FotD_StrongholdKey_ItemInfo"},
        {"Fort of Fortune Key", "LegendaryFort_StrongholdKey_ItemInfo"},
        {"Ashen Key", "AshenKey_ItemInfo"},

        // --- All Emissary Flags (valuable to Reapers) ---
        {"Order of Souls Flag Lvl5", "EmissaryFlotsam_OrderOfSouls_Rank5"},
        {"Order of Souls Flag Lvl4", "EmissaryFlotsam_OrderOfSouls_Rank4"},
        {"Order of Souls Flag Lvl3", "EmissaryFlotsam_OrderOfSouls_Rank3"},
        {"Order of Souls Flag Lvl2", "EmissaryFlotsam_OrderOfSouls_Rank2"},
        {"Order of Souls Flag Lvl1", "EmissaryFlotsam_OrderOfSouls_Rank1"},
        {"Gold Hoarder Flag Lvl5", "EmissaryFlotsam_GoldHoarders_Rank5"},
        {"Gold Hoarder Flag Lvl4", "EmissaryFlotsam_GoldHoarders_Rank4"},
        {"Gold Hoarder Flag Lvl3", "EmissaryFlotsam_GoldHoarders_Rank3"},
        {"Gold Hoarder Flag Lvl2", "EmissaryFlotsam_GoldHoarders_Rank2"},
        {"Gold Hoarder Flag Lvl1", "EmissaryFlotsam_GoldHoarders"},
        {"Reaper's Bones Flag Lvl5", "EmissaryFlotsam_Reapers_Rank5"},
        {"Reaper's Bones Flag Lvl4", "EmissaryFlotsam_Reapers_Rank4"},
        {"Reaper's Bones Flag Lvl3", "EmissaryFlotsam_Reapers_Rank3"},
        {"Reaper's Bones Flag Lvl2", "EmissaryFlotsam_Reapers_Rank2"},
        {"Reaper's Bones Flag Lvl1", "EmissaryFlotsam_Reapers"},
        {"Athena's Fortune Flag Lvl5", "EmissaryFlotsam_AthenasFortune_Rank5"},
        {"Athena's Fortune Flag Lvl4", "EmissaryFlotsam_AthenasFortune_Rank4"},
        {"Athena's Fortune Flag Lvl3", "EmissaryFlotsam_AthenasFortune_Rank3"},
        {"Athena's Fortune Flag Lvl2", "EmissaryFlotsam_AthenasFortune_Rank2"},
        {"Athena's Fortune Flag Lvl1", "EmissaryFlotsam_AthenasFortune"},
        {"Merchant Alliance Flag Lvl5", "EmissaryFlotsam_MerchantAlliance_Rank5"},
        {"Merchant Alliance Flag Lvl4", "EmissaryFlotsam_MerchantAlliance_Rank4"},
        {"Merchant Alliance Flag Lvl3", "EmissaryFlotsam_MerchantAlliance_Rank3"},
        {"Merchant Alliance Flag Lvl2", "EmissaryFlotsam_MerchantAlliance_Rank2"},
        {"Merchant Alliance Flag Lvl1", "EmissaryFlotsam_MerchantAlliance"}
    };

    inline std::pair<std::string, std::string> projectiles[] = {
        {"Cannonball", "Projectile_CannonBall"},
        {"Trident Projectile", "Projectile_SirenTrident"},
        {"Siren Song Projectile", "Projectile_SirenSong"}
    };

    // Useful items that are not typically sold for gold, such as keys, crates, and Tridents.
    inline std::pair<std::string, std::string> usefulItems[] = {
        // --- Weapons ---
        {"Trident of Dark Tides", "SirenTrident_ItemInfo"},

        // --- Keys & Quest Progression Items ---
        {"FoTD Key", "FotD_StrongholdKey_ItemInfo"},
        {"Fort of Fortune Key", "LegendaryFort_StrongholdKey_ItemInfo"},
        {"Captain's Key", "MA_CabinDoorKey_ItemInfo"},
        {"Shipwreck Graveyard Key", "ShipWreckGraveyardKeyItemInfo"},
        {"Sea Fort Key", "SeaFort_Key_Vault_ItemInfo"},
        {"Sea Fort Store Room Key", "SeaFort_Key_StoreRoom_ItemInfo"},
        {"Stronghold Key", "StrongholdKey_ItemInfo"},
        {"Ashen Key", "AshenKey_ItemInfo"},
        {"Old Sailor's Key", "OldKey_Goblet2_ItemInfo"},
        {"Krakens Fall Gold Key", "Totem_GoldShark_ItemInfo"},
        {"Devils Ridge Gold Key", "Totem_GoldBoar_ItemInfo"},
        {"Crescent Isle Gold Key", "Totem_GoldMoon_ItemInfo"},
        {"Mermaids Hideaway Gold Key", "Totem_GoldSnake_ItemInfo"},
        {"Crooks Hollow Gold Key", "Totem_GoldScarab_ItemInfo"},
        {"N-13 Gold Key", "Totem_GoldCrab_ItemInfo"},
        {"Fletcher's Rest Gold Key", "Totem_GoldEagle_ItemInfo"},
        {"Ashen Reaches Gold Key", "Totem_GoldSun_ItemInfo"},
        {"Shark Medallion", "Medallion_Shark_ItemInfo"},
        {"Boar Medallion", "Medallion_Boar_ItemInfo"},
        {"Moon Medallion", "Medallion_Moon_ItemInfo"},
        {"Snake Medallion", "Medallion_Snake_ItemInfo"},
        {"Scarab Medallion", "Medallion_Scarab_ItemInfo"},
        {"Crab Medallion", "Medallion_Crab_ItemInfo"},
        {"Eagle Medallion", "Medallion_Eagle_ItemInfo"},
        {"Sun Medallion", "Medallion_Sun_ItemInfo"},

        // --- Utility Crates & Storage ---
        {"Storage Crate", "MerchantCrate_AnyItemCrate_ItemInfo"},
        {"Ammo Chest", "PortableAmmoCrate_ItemInfo"},
        {"Storage Crate of the Damned", "MerchantCrate_GhostResourceCrate_ItemInfo"},
        {"Cannonball Crate of the Damned", "MerchantCrate_GhostCannonballCrate_ItemInfo"},
        {"Ashen Collectors Chest", "AshenChestCollectorsChest_ItemInfo"},
        {"Collectors Chest", "CollectorsChest_ItemInfo"},
    };

    inline std::pair<std::string, std::string> enemyEntity[] = {
        {"Ashen Lord", "GiantSkeletonPawnBase"},
        {"Megalodon", "TinyShark"},
        {"Megalodon Zone", "TinySharkExperience"},
        {"Skeleton", "SkeletonPawnBase"},
        {"Shark", "Shark_C"},
        {"Siren", "SirenGruntPawn"},
        {"Siren Leader", "SirenLeaderPawn"},
        {"Phantom", "PhantomPawnBase"},
        {"Ocean Crawler (Hermit)", "OceanCrawlerCharacter_Hermit"},
        {"Ocean Crawler (Electric)", "OceanCrawlerCharacter_Eelectric"},
        {"Ocean Crawler (Crab)", "OceanCrawlerCharacter_Crab"}
    };


    inline std::pair<std::string, std::string> islandNames[] = {
        // Shores of Plenty
        {"Sailor's Bounty", "feature_sailors_bounty"},
        {"Smuggler's Bay", "feature_smugglers_bay"},
        {"Cannon Cove", "feature_cannon_cove"},
        {"Lonely Isle", "feature_lonely_isle"},
        {"Mermaid's Hideaway", "feature_mermaids_hideaway"},
        {"Wanderers Refuge", "feature_wanderers_refuge"},

        // Ancient Isles
        {"Crook's Hollow", "feature_crooks_hollow"},
        {"Thieves' Haven", "feature_thieves_haven"},
        {"Sharkbait Cove", "feature_sharkbait_cove"},
        {"Plunder Valley", "feature_plunder_valley"},
        {"Paradise Spring", "feature_paradise_spring"},

        // The Wilds
        {"Marauder's Arch", "feature_marauders_arch"},
        {"Kraken's Fall", "feature_krakens_fall"},
        {"Blind Man's Lagoon", "feature_blind_mans_lagoon"},
        {"The Crooked Masts", "feature_the_crooked_masts"},

        // The Devil's Roar
        {"The Devil's Thirst", "feature_the_devils_thirst"},
        {"Fetcher's Rest", "feature_fetchers_rest"},
        {"Ruby's Fall", "feature_rubys_fall"},
        {"Flintlock Peninsula", "feature_flintlock_peninsula"},
        {"Molten Sands Fortress", "feature_molten_sands_fortress"},

        // Outposts
        {"Dagger Tooth Outpost", "outpost_dagger_tooth"},
        {"Galleon's Grave Outpost", "outpost_galleons_grave"},
        {"Golden Sands Outpost", "outpost_golden_sands"},
        {"Sanctuary Outpost", "outpost_sanctuary"},
        {"Ancient Spire Outpost", "outpost_ancient_spire"},
        {"Plunder Outpost", "outpost_plunder"},
        {"Morrow's Peak Outpost", "outpost_morrows_peak"},

        // Seaposts
        {"The Spoils of Plenty Store", "seapost_spoils_of_plenty"},
        {"Stephen's Spoils", "seapost_stephens_spoils"},
        {"Three Paces East Seapost", "seapost_three_paces_east"},
        {"Brian's Bazaar", "seapost_brians_bazaar"},
        {"The Wild Treasures Store", "seapost_wild_treasures_store"},
        {"Merrick's Seapost", "seapost_merricks_seapost"}
    };

    inline std::pair<std::string, std::string> gold_hoarders_loot[] = {
        {"Magma Grail", "TreasureArtifact_ItemInfo_DVR_Mythical"},
        {"Devil's Remnant", "TreasureArtifact_ItemInfo_DVR_Legendary"},
        {"Opulent Curio", "TreasureArtifact_impressive_01_a"},
        {"Adorned Receptacle", "TreasureArtifact_impressive_02_a"},
        {"Peculiar Relic", "TreasureArtifact_impressive_03_a"},
        {"Peculiar Coral Relic", "SKCoralTrinket_Mythical_ItemInfo"},
        {"Brimstone Casket", "TreasureArtifact_ItemInfo_DVR_Rare"},
        {"Golden Reliquary", "TreasureArtifact_box_03_a"},
        {"Golden Coral Reliquary", "SKCoralTrinket_Legendary_ItemInfo"},
        {"Gilded Chalice", "TreasureArtifact_goblet_03_a"},
        {"Ornate Carafe", "TreasureArtifact_vase_03_a"},
        {"Roaring Goblet", "TreasureArtifact_ItemInfo_DVR_Common"},
        {"Silvered Cup", "TreasureArtifact_goblet_02_a"},
        {"Silvered Coral Cup", "SKCoralTrinket_Rare_ItemInfo"},
        {"Elaborate Flagon", "TreasureArtifact_vase_02_a"},
        {"Decorative Coffer", "TreasureArtifact_box_02_a"},
        {"Mysterious Vessel", "TreasureArtifact_vase_01_a"},
        {"Mysterious Coral Vessel", "SKCoralTrinket_Common_ItemInfo"},
        {"Bronze Secret-Keeper", "TreasureArtifact_box_01_a"},
        {"Ancient Goblet", "TreasureArtifact_goblet_01_a"},
        {"Stronghold Chest", "TreasureChest_ItemInfo_Fort"},
        {"Chest of Ancient Tributes", "TreasureChest_Vault_ItemInfo"},
        {"Chest of the Damned", "TreasureChest_ItemInfo_Ghost"},
        {"Skeleton Captain's Chest", "TreasureChest_ItemInfo_AIShip"},
        {"Ashen Captain's Chest", "TreasureChest_ItemInfo_Mythical_DVR"},
        {"Shipwrecked Captain's Chest", "ShipwreckTreasureChest_ItemInfo_Mythical"},
        {"Chest of a Thousand Grogs", "TreasureChest_ItemInfo_Drunken"},
        {"Chest of Rage", "TreasureChest_ItemInfo_ChestOfRage"},
        {"Chest of Sorrow", "TreasureChest_ItemInfo_Weeping"},
        {"Chest of Everlasting Sorrow", "TreasureChest_ItemInfo_EverlastingSorrow"},
        {"Captain's Chest", "TreasureChest_ItemInfo_Mythical"},
        {"Coral Captain's Chest", "SK_CoralChest_ItemInfo_Mythical"},
        {"Ashen Marauder's Chest", "TreasureChest_ItemInfo_Legendary_DVR"},
        {"Shipwrecked Marauder's Chest", "ShipwreckTreasureChest_ItemInfo_Legendary"},
        {"Marauder's Chest", "TreasureChest_ItemInfo_Legendary"},
        {"Coral Marauder Chest", "SK_CoralChest_ItemInfo_Legendary"},
        {"Ashen Seafarer's Chest", "TreasureChest_ItemInfo_Rare_DVR"},
        {"Shipwrecked Seafarer's Chest", "ShipwreckTreasureChest_ItemInfo_Rare"},
        {"Seafarer's Chest", "TreasureChest_ItemInfo_Rare"},
        {"Coral Seafarer's Chest", "SK_CoralChest_ItemInfo_Rare"},
        {"Ashen Castaway's Chest", "TreasureChest_ItemInfo_Common_DVR"},
        {"Shipwrecked Castaway's Chest", "ShipwreckTreasureChest_ItemInfo_Common"},
        {"Castaway's Chest", "TreasureChest_ItemInfo_Common"},
        {"Coral Castaway Chest", "SK_CoralChest_ItemInfo_Common"},
        {"Sailor's Chest", "TreasureChest_ItemInfo_OnDemand_Traditional_Tier01"},
        {"Ashen King's Chest", "TreasureChest_ItemInfo_OnDemand_XMarks_DVRSRank"}
    };

    inline std::pair<std::string, std::string> order_of_souls_loot[] = {
        {"Ashen King's Bounty Skull", "BountyRewardSkullItemInfo_OnDemand_DVRSRank"},
        {"Wrathfull King's Bounty Skull", "BountyRewardSkullItemInfo_OnDemand_SRank"},
        {"Ashen Winds Skull", "AshenWindsSkull_ItemInfo"},
        {"Stronghold Skull", "BountyRewardSkullItemInfo_Fort"},
        {"Captain Skull of the Damned", "BountyRewardSkullItemInfo_Ghost_Captain"},
        {"Skull of the Damned", "BountyRewardSkullItemInfo_Ghost_Common"},
        {"Skeleton Captain's Skull", "BountyRewardSkullItemInfo_AIShip"},
        {"Ashen Villainous Bounty Skull", "BountyRewardSkullItemInfo_Mythical_DVR"},
        {"Villainous Bounty Skull", "BountyRewardSkullItemInfo_Mythical"},
        {"Villainous Coral Skull", "SKLostCapSkullItemInfo_Mythical"},
        {"Ashen Hateful Bounty Skull", "BountyRewardSkullItemInfo_Legendary_DVR"},
        {"Hateful Bounty Skull", "BountyRewardSkullItemInfo_Legendary"},
        {"Hateful Coral Skull", "SKLostCapSkullItemInfo_Legendary"},
        {"Ashen Disgraced Bounty Skull", "BountyRewardSkullItemInfo_Rare_DVR"},
        {"Disgraced Bounty Skull", "BountyRewardSkullItemInfo_Rare"},
        {"Disgraced Coral Skull", "SKLostCapSkullItemInfo_Rare"},
        {"Ashen Foul Bounty Skull", "BountyRewardSkullItemInfo_Common_DVR"},
        {"Foul Bounty Skull", "BountyRewardSkullItemInfo_Common"},
        {"Foul Coral Skull", "SKLostCapSkullItemInfo_Common"},
        {"Ritual Skull", "Ritual_Skull_ItemInfo"},
        {"Skull of Destiny", "SkullOfDestiny_ItemInfo"}
    };

    inline std::pair<std::string, std::string> merchants_loot[] = {
        {"Logbook", "CaptainsLog"},
        {"Snake Basket", "MerchantCrate_SnakeBasket_ItemInfo"},
        {"Chicken Coop", "MerchantCrate_ChickenCrate"},
        {"Pig Crate", "MerchantCrate_PigCrate_ItemInfo"},
        {"Stronghold Gunpowder Barrel", "MerchantCrate_BigGunpowderBarrel"},
        {"Gunpowder Barrel", "MerchantCrate_GunpowderBarrel"},
        {"Ashes of the Damned", "MerchantCrate_Commodity_GhostCrate_ItemInfo"},
        {"Prosperous Manifest", "MerchantManifest_01a_ItemInfo"},
        {"Esteemed Manifest", "MerchantManifest_01b_ItemInfo"},
        {"Eminent Manifest", "MerchantManifest_01c_ItemInfo"},
        {"Revered Merchant Manifest", "MerchantManifest_01d_ItemInfo"},
        {"Crate of Ancient Bone Dust", "MerchantCrate_Commodity_Fort_ItemInfo"},
        {"Crate of Precious Gemstones", "MerchantCrate_Commodity_Gemstones"},
        {"Crate of Minerals", "MerchantCrate_Commodity_Minerals"},
        {"Casket of Forgotten Jewels", "SKMerchantCommodity_ForgottenJewels"},
        {"Casket of Antiquated Coffee", "SKMerchantCommodity_AntiCoffee"},
        {"Crate of Exquisite Spices", "MerchantCrate_Commodity_SpiceCrate"},
        {"Crate of Fine Ore", "MerchantCrate_Commodity_Ore"},
        {"Crate of Exotic Silks", "MerchantCrate_Commodity_SilkCrate"},
        {"Crate of Volcanic Stone", "MerchantCrate_Commodity_VolcanicStone"},
        {"Crate of Rare Tea", "MerchantCrate_Commodity_TeaCrate"},
        {"Crate of Fine Sugar", "MerchantCrate_Commodity_SugarCrate"},
        {"Crate of Unclassified Gemstones", "UnsortedCommodity_Gemstones"},
        {"Crate of Unfiltered Minerals", "UnsortedCommodity_Minerals"},
        {"Crate of Broken Stone", "UnsortedCommodity_Stone"},
        {"Crate of Unrefined Spices", "UnsortedCommodity_Spices"},
        {"Crate of Unsorted Silks", "UnsortedCommodity_Silks"},
        {"Crate of Ungraded Tea", "UnsortedCommodity_Tea"},
        {"Crate of Raw Sugar", "UnsortedCommodity_Sugar"},
        {"Crate of Luxurious Cloth", "CargoRunCrate_Cloth_ItemInfo"},
        {"Crate of Plants", "CargoRunCrate_Plants_ItemInfo"},
        {"Crate of Rum", "CargoRunCrate_Rum_ItemInfo"},
        {"Storage Crate of the Damned", "MerchantCrate_GhostResourceCrate"},
        {"Cannonball Crate of the Damned", "MerchantCrate_GhostCannonballCrate"}
    };

    inline std::pair<std::string, std::string> reapers_loot[] = {
        {"Reapers Bounty", "ReapersBounty_ItemInfo"},
        {"Reapers Bounty (FoTD)", "FortReapersBountyChest_ItemInfo"},
        {"Reapers Chest", "ReapersChest_ItemInfo"},
        {"Reapers Chest (FoTD)", "FortReapersChest_ItemInfo"},
        {"Generous Gift", "HighValueGift_ItemInfo"},
        {"Humble Gift", "LowValueGift_ItemInfo"},
        // Note: Emissary flags are also Reaper loot, but are defined in the 'goodLoot' table
        // for global highlighting.
    };

    inline std::pair<std::string, std::string> athena_loot[]{
        {"Box of Wondrous Secrets", "Wondrous_ItemInfo"},
        {"Ashen Chest of Legends", "TreasureChest_ItemInfo_PirateLegend_DVR"},
        {"Chest of Legends", "TreasureChest_ItemInfo_PirateLegend"},
        {"Offering of Legendary Goods", "trs_box_leg_01_a"},
        {"Artifact of Legendary Hunger", "trs_dark_shark_leg_01_a"},
        {"Athena's Relic", "trs_impressive_leg_01_a"},
        {"Jar of Athena's Incense", "trs_jar_leg_01_a"},
        {"Legendary Fortune Keeper", "trs_jewellery_box_leg_01_a"},
        {"Stone of Ancients", "PL_StoneOfAncients_ItemInfo"},
        {"Skull of Athena's Blessing", "trs_leg_crain_leg_01_a"},
        {"Keg of Ancient Black Powder", "MerchantCrate_PirateLegendBigGunpowderBarrel"},
        {"Chest of Fortune", "TreasureChest_ItemInfo_ChestofFortune"},
        {"Villainous Skull of Ancient Fortune", "BountyRewardSkullItemInfo_PirateLegendUncommon"},
        {"Skull of Ancient Fortune", "BountyRewardSkull_PirateLegend"},
        {"Gilded Relic of Ancient Fortune", "TreasureArtifact_ItemInfo_piratelegendimpressive_03_a"},
        {"Crate of Legendary Voyages", "MerchantCrate_CommonPirateLegend_ItemInfo"},
        {"Chalice of Ancient Fortune", "TreasureArtifact_ItemInfo_piratelegend_goblet_02_a"},
    };

    // Common, low-value, or otherwise uninteresting items.
    inline std::pair<std::string, std::string> low_priority_items[]{
        {"Sea Fort Gold Pouch", "SeaFortGoldPouches_ItemInfo"},
        {"Phantom Gold Pouch", "GoldPouches_Phantom_ItemInfo"},
        {"Gold Pouch", "GoldPouches_ItemInfo"},
        {"Pieces O Eight Pouch", "Mojo_PiecesOEightPouch_Collectable"},
        {"Pieces O Eight Coin", "Mojo_PiecesOEightCoin_Collectable"},
        {"Stool", "StoolItem"},
        {"Fireworks", "gmp_fireworks"} //This will catch rocket, flare, and cake types
    };

    // Gems and Tridents which can be sold to any faction.
    inline std::pair<std::string, std::string> any_faction_loot[]{
        {"Ruby Mermaid Gem", "MermaidGem_ItemInfo_Ruby"},
        {"Ruby Siren Gem", "SirenGem_ItemInfo_Ruby"},
        {"Ruby Breath of the Sea", "SK_SirenEssence_Ruby"},
        {"Emerald Mermaid Gem", "MermaidGem_ItemInfo_Emerald"},
        {"Emerald Siren Gem", "SirenGem_ItemInfo_Emerald"},
        {"Emerald Breath of the Sea", "SK_SirenEssence_Emerald"},
        {"Sapphire Mermaid Gem", "MermaidGem_ItemInfo_Sapphire"},
        {"Sapphire Siren Gem", "SirenGem_ItemInfo_Sapphire"},
        {"Sapphire Breath of the Sea", "SK_SirenEssence_Sapphire"},
        {"Trident of Dark Tides", "SirenTrident_ItemInfo"},
        {"Siren Trident (Sellable)", "SirenTridentSellable_ItemInfo"},
    };

    // Miscellaneous quest items and world objects.
    inline std::pair<std::string, std::string> misc_items[]{
        {"Merchant Quest Bottle", "MessageInABottle_Clue_ItemInfo"},
        {"Skeleton's Orders", "SkeletonOrdersRiddle_ItemInfo"},
        {"Message in a Bottle", "MessageInABottle_ItemInfo"},
        {"Coral Quest Bottle", "MessageInABottle_Coral_ItemInfo"},
        {"Emergent LockedAshenChest", "SOTOnDemand_Emergent_XMarks_LockedAshenChest"},
        {"Skeleton Orders", "SOTOnDemand_Emergent_XMarks_SkeletonCaptain"},
        {"Ancient's Brazier", "PL_HauntedBrazier"},
        {"Ruby Cursed Mermaid Statue", "SunkenCurseArtefact_Ruby"},
        {"Emerald Cursed Mermaid Statue", "SunkenCurseArtefact_Emerald"},
        {"Sapphire Cursed Mermaid Statue", "SunkenCurseArtefact_Sapphire"},
    };

    // Resource crates and other non-sellable utility items.
    inline std::pair<std::string, std::string> resource_crates[] = {
        {"Ammo Chest", "PortableAmmoCrate_ItemInfo"},
        {"Firebomb Crate", "MerchantCrate_FirebombCrate_ItemInfo"},
        {"Plank Crate (Purchased)", "MerchantCrate_WoodCrate_FullyStocked"},
        {"Plank Crate", "MerchantCrate_WoodCrate_ItemInfo"},
        {"Cannonball Crate (Purchased)", "MerchantCrate_CannonballCrate_FullyStocked"},
        {"Cannonball Crate", "MerchantCrate_CannonballCrate_ItemInfo"},
        {"Food Crate (Purchased)", "MerchantCrate_BananaCrate_FullyStocked"},
        {"Food Crate", "MerchantCrate_BananaCrate_ItemInfo"},
        {"Bait Box", "MerchantCrate_BaitBox"},
        {"Storage Crate (AI Drop)", "MerchantCrate_AIShipAnyItemCrate"},
        {"Storage Crate", "MerchantCrate_AnyItemCrate_ItemInfo"},
        {"Storage Crate of the Damned", "MerchantCrate_GhostResourceCrate"},
        {"Cannonball Crate of the Damned", "MerchantCrate_GhostCannonballCrate"},
    };

    inline std::pair<std::string, std::string> world_events[] = {
        {"Fort of the Damned", "BP_SkellyFort_RitualSkullCloud_C"},
        {"Fort of Fortune", "BP_LegendSkellyFort_SkullCloud_C"},
        {"Flameheart/Ghost Fleet", "BP_GhostShips_Signal_Flameheart_NetProxy_C"},
        {"Flameheart/Ghost Fleet", "BP_GhostShip_TornadoCloud_C"},
        {"Skeleton Fort", "BP_SkellyFort_SkullCloud_C"},
        {"Skeleton Fleet", "BP_SkellyShip_ShipCloud_C"},
        {"Ashen Winds", "BP_AshenLord_SkullCloud_C"},
        {"Burning Blade", "BP_ReaperTributeShipNetProxy_C"},
        {"Burning Blade", "BP_ReapersTributeShipTemplate_C"},
    };

}

#endif //TABLES_H