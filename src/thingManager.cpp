/*
  Tibia CLient
  Copyright (C) 2019 Saiyans King

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "thingManager.h"
#include "animator.h"
#include "spriteManager.h"
#include "game.h"

ThingManager g_thingManager;

extern Game g_game;
extern SpriteManager g_spriteManager;
extern Uint32 g_datRevision;

/*flags: These are values that represent how the client can interact with the appearance, and it's mostly used for Objects.

container: If the appearance is a Container

cumulative: If the appearance is stackable.

usable: If the appearance has any use function.

forceuse:

multiuse: If the appearance has any use with function.

write: If the object is writable and editable.
max_text_length: Maximum number of characters it can hold.

write_once: If the object is writable, but only once.
max_text_length_once: Maximum number of characters it can hold.

unmove: If it's impossible to move the appearance.

unsight: If the appearance blocks the vision when trying to i.e. shoot projectiles or spells.

take: If the object is pickupable, i.e. can be taken by players and stored into containers.

rotate: If the appearance can be rotated (such as many Furnitures).

lenshelp: If the appearance should trigger the help icon when the help feature is used on the client.
id: The ID of the help tooltip.

ignore_look: If the appearance must be ignored when the player looks it, meaning the appearance under it will be the one seen.

clothes: If the object can be worn as an equipment:
slot: The ID of the inventory slot it can be worn on.

default_action: The default action when using Left-Smart click mouse controls.
action: The default action type:
None
Look
Use
Open
Autowalk_Highlight

market: If an object is displayed in The Market.
category: The Market Category of the object: Armors, Amulets, Boots, Cotainers, Decoration, Food, Helmets_Hats, Legs, Others, Potions, Rings, Runes, Shields, Tools, Valuables, Ammunition, Axes, Clubs, Distance_Weapons, Swords, Wands_Rods, Premium_Scrolls, Tibia_Coins, Creature_Products.
trade_as_object_id:
show_as_object_id:
restrict_to_profession: The Vocation restriction of the object: Any, Knight, Paladin, Sorcerer, Druid, Promoted.
minimum_level: The required level to use the object.

wrap: If the object can be into a furniture kit or similar.

unwrap: If the object can be unwrapped.

topeffect:

npcsaledata: NPC Buy and Sell data as displayed in the Cyclopedia:
name: Name of the NPC.
location: Location of the NPC.
sale_price: Price the NPC sells the object for.
buy_price: Price the NPC buys the object for.

changedtoexpire: If the item can be switched on Action Bars using the Smart Switch option, currently available for Rings only.
former_object_typeid: The object ID of the corresponding "activated" version of the object.

corpse: If the object is a corpse.

player_corpse: If the object is a player's corpse.

cyclopediaitem: If the object is in the Cyclopedia.
cyclopedia_type: The Object ID that will be used to represent this object, which is not always the same as its own ID.
*/

ThingType::ThingType()
{
	for(Sint32 i = ThingFrameGroup_Default; i < ThingFrameGroup_Last; ++i)
	{
		FrameGroup& frame = m_frameGroup[i];
		frame.m_animator = NULL;
		frame.m_width = frame.m_height = 0;
		frame.m_layers = 0;
		frame.m_patternX = frame.m_patternY = frame.m_patternZ = 0;
		frame.m_animCount = 0;
	}

	m_flags = 0;

	m_groundSpeed = 100;
	m_writableSize[0] = m_writableSize[1] = 0;
	m_displacement[0] = m_displacement[1] = 0;
	m_light[0] = m_light[1] = 0;
	m_elevation = 0;
	m_minimapColor = 0;
	m_lensHelp = 0;
	m_cloth = 0;
	m_defaultAction = 0;

	m_id = 0;
	m_category = ThingCategory_Invalid;
}

void ThingType::clear()
{
	if(m_frameGroup[ThingFrameGroup_Idle].m_animator == m_frameGroup[ThingFrameGroup_Moving].m_animator)
	{
		//Idle and Moving framegroup can share animator instance so make sure we don't try to remove 2x the same animator
		if(m_frameGroup[ThingFrameGroup_Idle].m_animator)
			delete m_frameGroup[ThingFrameGroup_Idle].m_animator;
	}
	else
	{
		if(m_frameGroup[ThingFrameGroup_Idle].m_animator)
			delete m_frameGroup[ThingFrameGroup_Idle].m_animator;
		if(m_frameGroup[ThingFrameGroup_Moving].m_animator)
			delete m_frameGroup[ThingFrameGroup_Moving].m_animator;
	}
}

bool ThingType::loadType(Uint16 id, ThingCategory category, SDL_RWops* rwops)
{
	m_id = id;
	m_category = category;

	bool loaded = false;
	for(Sint32 i = 0; i < 0xFF; ++i)
	{
		Uint8 attr = SDL_ReadU8(rwops);
		if(attr == 0xFF)
		{
			loaded = true;
			break;
		}

		if(g_clientVersion >= 1000)
		{
			/* In 10.10+ all attributes from 16 and up were
			* incremented by 1 to make space for 16 as
			* "No Movement Animation" flag.
			*/
			if(attr == 16)
				attr = 252;
			else if(attr > 16)
				attr -= 1;
		}
		else if(g_clientVersion >= 860)
		{
			/* Default attribute values follow
			* the format of 8.6-9.86.
			* Therefore no changes here.
			*/
		}
		else if(g_clientVersion >= 780)
		{
			/* In 7.80-8.54 all attributes from 8 and higher were
			* incremented by 1 to make space for 8 as
			* "Item Charges" flag.
			*/
			if(attr == 8)
				attr = 254;
			else if(attr > 8)
				attr -= 1;
		}
		else if(g_clientVersion >= 755)
		{
			/* In 7.55-7.72 attributes 23 is "Floor Change". */
			if(attr == 23)
				attr = 251;
		}
		else if(g_clientVersion >= 740)
		{
			/* In 7.4-7.5 attribute "Ground Border" did not exist
			* attributes 1-15 have to be adjusted.
			* Several other changes in the format.
			*/
			if(attr > 0 && attr <= 15)
				attr += 1;
			else if(attr == 16)
				attr = 21;
			else if(attr == 17)
				attr = 251;
			else if(attr == 18)
				attr = 30;
			else if(attr == 19)
				attr = 25;
			else if(attr == 20)
				attr = 24;
			else if(attr == 22)
				attr = 28;
			else if(attr == 23)
				attr = 20;
			else if(attr == 24)
				attr = 26;
			else if(attr == 25)
				attr = 17;
			else if(attr == 26)
				attr = 18;
			else if(attr == 27)
				attr = 19;
			else if(attr == 28)
				attr = 27;

			/* "Multi Use" and "Force Use" are swapped */
			if(attr == 7)
				attr = 6;
			else if(attr == 6)
				attr = 7;
		}
		else if(g_clientVersion >= 710)
		{
			/* In 7.1-7.3 attributes are simmilar to 7.4-7.5
			* the difference is that there aren't hangable items.
			*/
			if(attr > 0 && attr <= 15)
				attr += 1;
			else if(attr == 16)
				attr = 21;
			else if(attr == 17)
				attr = 251;
			else if(attr == 18)
				attr = 30;
			else if(attr == 19)
				attr = 25;
			else if(attr == 20)
				attr = 24;
			else if(attr == 22)
				attr = 28;
			else if(attr == 23)
				attr = 20;
			else if(attr == 24)
				attr = 26;
			else if(attr == 25)
				attr = 27;
			else if(attr == 26)
				attr = 29;

			/* "Multi Use" and "Force Use" are swapped */
			if(attr == 7)
				attr = 6;
			else if(attr == 6)
				attr = 7;
		}

		switch(attr)
		{
			case 0: //Ground
			{
				m_flags |= ThingAttribute_Ground;
				m_groundSpeed = SDL_ReadLE16(rwops);
			}
			break;
			case 1: m_flags |= ThingAttribute_GroundBorder; break; //Top1 - borders etc
			case 2: m_flags |= ThingAttribute_OnBottom; break; //Top2 - benches etc
			case 3: m_flags |= ThingAttribute_OnTop; break; //Top3 - always on top
			case 4: m_flags |= ThingAttribute_Container; break; //Container
			case 5: m_flags |= ThingAttribute_Stackable; break; //Stackable
			case 6: m_flags |= ThingAttribute_ForceUse; break; //Force Use
			case 7: m_flags |= ThingAttribute_MultiUse; break; //Multi Use
			case 8: //Writable
			{
				m_flags |= ThingAttribute_Writable;
				m_writableSize[0] = SDL_ReadLE16(rwops);
			}
			break;
			case 9: //Writable Once
			{
				m_flags |= ThingAttribute_WritableOnce;
				m_writableSize[1] = SDL_ReadLE16(rwops);
			}
			break;
			case 10: m_flags |= ThingAttribute_FluidContainer; break; //Fluid Container
			case 11: m_flags |= ThingAttribute_Splash; break; //Splash
			case 12: m_flags |= ThingAttribute_NotWalkable; break; //Not Walkable
			case 13: m_flags |= ThingAttribute_NotMoveable; break; //Not Moveable
			case 14: m_flags |= ThingAttribute_BlockProjectile; break; //Nlock Projectile
			case 15: m_flags |= ThingAttribute_NotPathable; break; //Not Pathable
			case 16: m_flags |= ThingAttribute_Pickupable; break; //Pickupable
			case 17: m_flags |= ThingAttribute_Hangable; break; //Hangable
			case 18: m_flags |= ThingAttribute_HookSouth; break; //Hook South
			case 19: m_flags |= ThingAttribute_HookEast; break; //Hook East
			case 20: m_flags |= ThingAttribute_Rotateable; break; //Rotateable
			case 21: //Light
			{
				m_flags |= ThingAttribute_Light;
				m_light[0] = SDL_ReadLE16(rwops);
				m_light[1] = SDL_ReadLE16(rwops);
			}
			break;
			case 22: m_flags |= ThingAttribute_DontHide; break; //Don't Hide
			case 23: m_flags |= ThingAttribute_Translucent; break; //Translucent
			case 24: //Displacement
			{
				m_flags |= ThingAttribute_Displacement;
				if(g_clientVersion >= 755)
				{
					m_displacement[0] = SDL_ReadLE16(rwops);
					m_displacement[1] = SDL_ReadLE16(rwops);
				}
				else
				{
					m_displacement[0] = 8;
					m_displacement[1] = 8;
				}
			}
			break;
			case 25: //Elevation
			{
				m_flags |= ThingAttribute_Elevation;
				m_elevation = SDL_ReadLE16(rwops);
			}
			break;
			case 26: m_flags |= ThingAttribute_LyingCorpse; break; //Lying Corpse
			case 27: m_flags |= ThingAttribute_AnimateAlways; break; //Animate Always
			case 28: //Minimap Color
			{
				m_flags |= ThingAttribute_MinimapColor;
				m_minimapColor = SDL_ReadLE16(rwops);
			}
			break;
			case 29: //Lens Help
			{
				m_flags |= ThingAttribute_LensHelp;
				m_lensHelp = SDL_ReadLE16(rwops);
			}
			break;
			case 30: m_flags |= ThingAttribute_FullGround; break; //Full Ground
			case 31: m_flags |= ThingAttribute_LookThrough; break; //Look Through
			case 32: //Cloth
			{
				m_flags |= ThingAttribute_Cloth;
				m_cloth = SDL_ReadLE16(rwops);
			}
			break;
			case 33: //Market Data
			{
				m_flags |= ThingAttribute_Market;
				m_marketData.m_category = SDL_ReadLE16(rwops);
				m_marketData.m_tradeAs = SDL_ReadLE16(rwops);
				m_marketData.m_showAs = SDL_ReadLE16(rwops);
				m_marketData.m_name = SDL_ReadLEString(rwops);
				m_marketData.m_restrictVocation = SDL_ReadLE16(rwops);
				m_marketData.m_requiredLevel = SDL_ReadLE16(rwops);
			}
			break;
			case 34: //Default Action
			{
				m_flags |= ThingAttribute_DefaultAction;
				m_defaultAction = SDL_ReadLE16(rwops);
			}
			break;
			case 35: m_flags |= ThingAttribute_Wrapable; break; //Wrapable
			case 36: m_flags |= ThingAttribute_Unwrapable; break; //Unwrapable
			case 37: m_flags |= ThingAttribute_TopEffect; break; //Top Effect
			case 251: m_flags |= ThingAttribute_FloorChange; break; //Floor Change
			case 252: m_flags |= ThingAttribute_NoMoveAnimation; break; //No Move Animation
			case 253: m_flags |= ThingAttribute_Usable; break; //Usable
			case 254: m_flags |= ThingAttribute_Chargeable; break; //Chargeable
			default: break;
		}
	}
	if(!loaded)
		return false;

	bool needCopyToIdle = false;
	bool hasFrameGroups = (category == ThingCategory_Creature && g_game.hasGameFeature(GAME_FEATURE_FRAMEGROUPS));
	Uint8 groupCount = (hasFrameGroups ? SDL_ReadU8(rwops) : 1);
	for(Uint8 i = 0; i < groupCount; ++i)
	{
		Uint8 frameGroupType = ThingFrameGroup_Default;
		if(hasFrameGroups)
		{
			frameGroupType = SDL_ReadU8(rwops);
			if(frameGroupType == ThingFrameGroup_Moving && groupCount == 1)
				needCopyToIdle = true;
		}

		FrameGroup& frame = m_frameGroup[frameGroupType];
		frame.m_width = SDL_ReadU8(rwops);
		frame.m_height = SDL_ReadU8(rwops);
		if(frame.m_width > 1 || frame.m_height > 1)
			frame.m_realSize = SDL_ReadU8(rwops);
		else
			frame.m_realSize = 32;

		frame.m_layers = SDL_ReadU8(rwops);
		frame.m_patternX = SDL_ReadU8(rwops);
		frame.m_patternY = SDL_ReadU8(rwops);
		if(g_clientVersion >= 755)
			frame.m_patternZ = SDL_ReadU8(rwops);
		else
			frame.m_patternZ = 1;

		frame.m_animCount = SDL_ReadU8(rwops);
		if(frame.m_animCount > 1 && g_game.hasGameFeature(GAME_FEATURE_ENHANCED_ANIMATIONS))
		{
			frame.m_animator = new Animator();
			frame.m_animator->loadAnimator(frame.m_animCount, rwops);
		}

		Sint32 totalSprites = SDL_static_cast(Sint32, frame.m_width);
		totalSprites *= SDL_static_cast(Sint32, frame.m_height);
		totalSprites *= SDL_static_cast(Sint32, frame.m_layers);
		totalSprites *= SDL_static_cast(Sint32, frame.m_patternX);
		totalSprites *= SDL_static_cast(Sint32, frame.m_patternY);
		totalSprites *= SDL_static_cast(Sint32, frame.m_patternZ);
		totalSprites *= SDL_static_cast(Sint32, frame.m_animCount);
		if(totalSprites > 4096)
			return false;

		frame.m_sprites.resize(SDL_static_cast(size_t, totalSprites));
		for(Sint32 s = 0; s < totalSprites; ++s)
		{
			if(g_game.hasGameFeature(GAME_FEATURE_EXTENDED_SPRITES))
				frame.m_sprites[s] = SDL_ReadLE32(rwops);
			else
				frame.m_sprites[s] = SDL_static_cast(Uint32, SDL_ReadLE16(rwops));
		}
	}
	if(category == ThingCategory_Creature && !g_game.hasGameFeature(GAME_FEATURE_FRAMEGROUPS))
		m_flags |= ThingAttribute_NoMoveAnimation;
	if(needCopyToIdle)
	{
		FrameGroup& fromFrame = m_frameGroup[ThingFrameGroup_Moving];
		FrameGroup& toFrame = m_frameGroup[ThingFrameGroup_Idle];
		toFrame.m_sprites.assign(fromFrame.m_sprites.begin(), fromFrame.m_sprites.end());
		toFrame.m_animator = fromFrame.m_animator;
		toFrame.m_width = fromFrame.m_width;
		toFrame.m_height = fromFrame.m_height;
		toFrame.m_realSize = fromFrame.m_realSize;
		toFrame.m_layers = fromFrame.m_layers;
		toFrame.m_patternX = fromFrame.m_patternX;
		toFrame.m_patternY = fromFrame.m_patternY;
		toFrame.m_patternZ = fromFrame.m_patternZ;
		toFrame.m_animCount = fromFrame.m_animCount;
	}
	return true;
}

bool ThingType::loadAppearance(Sint64 offsetLimit, Uint16& things, SDL_RWops* rwops)
{
	while(SDL_RWtell(rwops) < offsetLimit)
	{
		Uint32 tag = SDL_ReadProtobufTag(rwops);
		Uint32 tagHigh = (tag >> 3);
		Uint8 tagLow = SDL_static_cast(Uint8, tag);
		if(tagHigh == 1 && tagLow == 8)// optional uint32 id = 1;
		{
			m_id = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
			if(m_id == 0)
				return false;
			else
				things = UTIL_max<Uint16>(things, m_id);
		}
		else if(tagHigh == 2 && tagLow == 18)// repeated .tibia.protobuf.appearances.FrameGroup frame_group = 2;
		{
			FrameGroup parseFrameGroup;
			parseFrameGroup.m_animator = NULL;
			parseFrameGroup.m_width = 1;
			parseFrameGroup.m_height = 1;
			parseFrameGroup.m_realSize = 32;
			parseFrameGroup.m_layers = 1;
			parseFrameGroup.m_patternX = 1;
			parseFrameGroup.m_patternY = 1;
			parseFrameGroup.m_patternZ = 1;
			parseFrameGroup.m_animCount = 1;
			ThingFrameGroup frameGroupType = ThingFrameGroup_Default;
			Uint32 frameGroupId = 0;

			Sint64 framegroupLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
			while(SDL_RWtell(rwops) < framegroupLimit)
			{
				tag = SDL_ReadProtobufTag(rwops);
				tagHigh = (tag >> 3);
				tagLow = SDL_static_cast(Uint8, tag);
				if(tagHigh == 1 && tagLow == 8)// optional .tibia.protobuf.appearances.FIXED_FRAME_GROUP fixed_frame_group = 1;
				{
					frameGroupType = SDL_static_cast(ThingFrameGroup, SDL_ReadProtobufVariant(rwops));
					if(frameGroupType >= ThingFrameGroup_Last)
						frameGroupType = ThingFrameGroup_Default;
				}
				else if(tagHigh == 2 && tagLow == 16)// optional uint32 id = 2;
					frameGroupId = SDL_static_cast(Uint32, SDL_ReadProtobufVariant(rwops));
				else if(tagHigh == 3 && tagLow == 26)// optional .tibia.protobuf.appearances.SpriteInfo sprite_info = 3;
				{
					Sint64 spriteInfoLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
					while(SDL_RWtell(rwops) < spriteInfoLimit)
					{
						tag = SDL_ReadProtobufTag(rwops);
						tagHigh = (tag >> 3);
						tagLow = SDL_static_cast(Uint8, tag);
						if(tagHigh == 1 && tagLow == 8)// optional uint32 pattern_width = 1;
							parseFrameGroup.m_patternX = SDL_static_cast(Uint8, SDL_ReadProtobufVariant(rwops));
						else if(tagHigh == 2 && tagLow == 16)// optional uint32 pattern_height = 2;
							parseFrameGroup.m_patternY = SDL_static_cast(Uint8, SDL_ReadProtobufVariant(rwops));
						else if(tagHigh == 3 && tagLow == 24)// optional uint32 pattern_depth = 3;
							parseFrameGroup.m_patternZ = SDL_static_cast(Uint8, SDL_ReadProtobufVariant(rwops));
						else if(tagHigh == 4 && tagLow == 32)// optional uint32 layers = 4;
							parseFrameGroup.m_layers = SDL_static_cast(Uint8, SDL_ReadProtobufVariant(rwops));
						else if(tagHigh == 5 && (tagLow == 40 || tagLow == 42))// repeated uint32 sprite_id = 5;
						{
							if(tagLow == 40)
							{
								bool haveData = true;
								do
								{
									parseFrameGroup.m_sprites.push_back(SDL_static_cast(Uint32, SDL_ReadProtobufVariant(rwops)));
									haveData = (SDL_RWtell(rwops) < spriteInfoLimit);
								} while(haveData && SDL_ReadU8(rwops) == 40);
								if(haveData)//Rewind one byte
									SDL_RWseek(rwops, SDL_RWtell(rwops)-1, RW_SEEK_SET);
							}
							else if(tagLow == 42)
							{
								Sint64 spriteIdLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
								while(SDL_RWtell(rwops) < spriteIdLimit)
									parseFrameGroup.m_sprites.push_back(SDL_static_cast(Uint32, SDL_ReadProtobufVariant(rwops)));
							}
						}
						else if(tagHigh == 6 && tagLow == 50)// optional .tibia.protobuf.appearances.SpriteAnimation animation = 6;
						{
							parseFrameGroup.m_animator = new Animator();

							Sint64 spriteAnimationLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < spriteAnimationLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 default_start_phase = 1;
									parseFrameGroup.m_animator->setStartPhase(SDL_static_cast(Sint32, SDL_ReadProtobufVariant(rwops)));
								else if(tagHigh == 2 && tagLow == 16)// optional bool synchronized = 2;
									parseFrameGroup.m_animator->setAsync(!SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)));
								else if(tagHigh == 3 && tagLow == 24)// optional bool random_start_phase = 3;
								{
									if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
										parseFrameGroup.m_animator->setStartPhase(-2);
								}
								else if(tagHigh == 4 && tagLow == 32)// optional .tibia.protobuf.shared.ANIMATION_LOOP_TYPE loop_type = 4;
								{
									Sint32 loopType = SDL_static_cast(Sint32, SDL_ReadProtobufVariant(rwops));
									if(loopType == 0)//Infinite Loop
										parseFrameGroup.m_animator->setLoopCount(0);
									else if(loopType == 1)//Counted(Should we set this here?)
										parseFrameGroup.m_animator->setLoopCount(1);
									else//Ping Pong
										parseFrameGroup.m_animator->setLoopCount(-1);
								}
								else if(tagHigh == 5 && tagLow == 40)// optional uint32 loop_count = 5;
								{
									Sint32 loopCount = SDL_static_cast(Sint32, SDL_ReadProtobufVariant(rwops));
									if(loopCount > 0)
										parseFrameGroup.m_animator->setLoopCount(loopCount);
								}
								else if(tagHigh == 6 && tagLow == 50)// repeated .tibia.protobuf.appearances.SpritePhase sprite_phase = 6;
								{
									bool haveData = true;
									do
									{
										Sint32 min = 0, max = 0;
										Sint64 spritePhaseLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
										while(SDL_RWtell(rwops) < spritePhaseLimit)
										{
											tag = SDL_ReadProtobufTag(rwops);
											tagHigh = (tag >> 3);
											tagLow = SDL_static_cast(Uint8, tag);
											if(tagHigh == 1 && tagLow == 8)// optional uint32 duration_min = 1;
												min = SDL_static_cast(Sint32, SDL_ReadProtobufVariant(rwops));
											else if(tagHigh == 2 && tagLow == 16)// optional uint32 duration_max = 2;
												max = SDL_static_cast(Sint32, SDL_ReadProtobufVariant(rwops));
											else
												break;
										}
										parseFrameGroup.m_animator->addPhaseDuration(min, max);
										haveData = (SDL_RWtell(rwops) < spriteAnimationLimit);
									} while(haveData && SDL_ReadU8(rwops) == 50);
									if(haveData)//Rewind one byte
										SDL_RWseek(rwops, SDL_RWtell(rwops)-1, RW_SEEK_SET);

									parseFrameGroup.m_animCount = SDL_static_cast(Uint8, parseFrameGroup.m_animator->getAnimationPhases());
								}
								else
									break;
							}
						}
						else if(tagHigh == 7 && tagLow == 56)// optional uint32 bounding_square = 7;
							SDL_ReadProtobufVariant(rwops);
						else if(tagHigh == 8 && tagLow == 64)// optional bool is_opaque = 8;
							SDL_ReadProtobufVariant(rwops);
						else if(tagHigh == 9 && tagLow == 74)// repeated .tibia.protobuf.appearances.Box bounding_box_per_direction = 9;
						{
							bool haveData = true;
							do
							{
								Sint64 spriteBoxLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
								while(SDL_RWtell(rwops) < spriteBoxLimit)
								{
									tag = SDL_ReadProtobufTag(rwops);
									tagHigh = (tag >> 3);
									tagLow = SDL_static_cast(Uint8, tag);
									if(tagHigh == 1 && tagLow == 8)// optional uint32 x = 1;
										SDL_ReadProtobufVariant(rwops);
									else if(tagHigh == 2 && tagLow == 16)// optional uint32 y = 2;
										SDL_ReadProtobufVariant(rwops);
									else if(tagHigh == 3 && tagLow == 24)// optional uint32 width = 3;
										parseFrameGroup.m_realSize = UTIL_max<Uint8>(parseFrameGroup.m_realSize, SDL_static_cast(Uint8, SDL_ReadProtobufVariant(rwops)));
									else if(tagHigh == 4 && tagLow == 32)// optional uint32 height = 4;
										parseFrameGroup.m_realSize = UTIL_max<Uint8>(parseFrameGroup.m_realSize, SDL_static_cast(Uint8, SDL_ReadProtobufVariant(rwops)));
									else
										break;
								}
								haveData = (SDL_RWtell(rwops) < spriteInfoLimit);
							} while(haveData && SDL_ReadU8(rwops) == 74);
							if(haveData)//Rewind one byte
								SDL_RWseek(rwops, SDL_RWtell(rwops)-1, RW_SEEK_SET);
						}
						else
							break;
					}
				}
				else
					break;
			}

			FrameGroup& ourFrame = m_frameGroup[frameGroupType];
			if(parseFrameGroup.m_animCount == 1 && parseFrameGroup.m_animator)
			{
				delete parseFrameGroup.m_animator;
				parseFrameGroup.m_animator = NULL;
			}
			ourFrame.m_animator = parseFrameGroup.m_animator;
			ourFrame.m_width = parseFrameGroup.m_width;
			ourFrame.m_height = parseFrameGroup.m_height;
			ourFrame.m_realSize = parseFrameGroup.m_realSize;
			ourFrame.m_layers = parseFrameGroup.m_layers;
			ourFrame.m_patternX = parseFrameGroup.m_patternX;
			ourFrame.m_patternY = parseFrameGroup.m_patternY;
			ourFrame.m_patternZ = parseFrameGroup.m_patternZ;
			ourFrame.m_animCount = parseFrameGroup.m_animCount;
			g_spriteManager.manageSprites(parseFrameGroup.m_sprites, ourFrame.m_sprites, ourFrame.m_width, ourFrame.m_height);
		}
		else if(tagHigh == 3 && tagLow == 26)// optional .tibia.protobuf.appearances.AppearanceFlags flags = 3;
		{
			Sint64 flagsLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
			while(SDL_RWtell(rwops) < flagsLimit)
			{
				tag = SDL_ReadProtobufTag(rwops);
				tagHigh = (tag >> 3);
				tagLow = SDL_static_cast(Uint8, tag);
				switch(tagHigh)
				{
					case 1:// optional .tibia.protobuf.appearances.AppearanceFlagBank bank = 1;
					{
						if(tagLow == 10)
						{
							Sint64 bankLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < bankLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8) // optional uint32 waypoints = 1;
								{
									m_flags |= ThingAttribute_Ground;
									m_groundSpeed = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 2:// optional bool clip = 2;
					{
						if(tagLow == 16)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_GroundBorder;
						}
						else
							goto loop_break;
					}
					break;
					case 3:// optional bool bottom = 3;
					{
						if(tagLow == 24)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_OnBottom;
						}
						else
							goto loop_break;
					}
					break;
					case 4:// optional bool top = 4;
					{
						if(tagLow == 32)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_OnTop;
						}
						else
							goto loop_break;
					}
					break;
					case 5:// optional bool container = 5;
					{
						if(tagLow == 40)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Container;
						}
						else
							goto loop_break;
					}
					break;
					case 6:// optional bool cumulative = 6;
					{
						if(tagLow == 48)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Stackable;
						}
						else
							goto loop_break;
					}
					break;
					case 7:// optional bool usable = 7;
					{
						if(tagLow == 56)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Usable;
						}
						else
							goto loop_break;
					}
					break;
					case 8:// optional bool forceuse = 8;
					{
						if(tagLow == 64)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_ForceUse;
						}
						else
							goto loop_break;
					}
					break;
					case 9:// optional bool multiuse = 9;
					{
						if(tagLow == 72)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_MultiUse;
						}
						else
							goto loop_break;
					}
					break;
					case 10:// optional .tibia.protobuf.appearances.AppearanceFlagWrite write = 10;
					{
						if(tagLow == 82)
						{
							Sint64 writableLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < writableLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 max_text_length = 1;
								{
									m_flags |= ThingAttribute_Writable;
									m_writableSize[0] = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 11:// optional .tibia.protobuf.appearances.AppearanceFlagWriteOnce write_once = 11;
					{
						if(tagLow == 90)
						{
							Sint64 writableLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < writableLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 max_text_length_once = 1;
								{
									m_flags |= ThingAttribute_WritableOnce;
									m_writableSize[1] = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 12:// optional bool liquidpool = 12;
					{
						if(tagLow == 96)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Splash;
						}
						else
							goto loop_break;
					}
					break;
					case 13:// optional bool unpass = 13;
					{
						if(tagLow == 104)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_NotWalkable;
						}
						else
							goto loop_break;
					}
					break;
					case 14:// optional bool unmove = 14;
					{
						if(tagLow == 112)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_NotMoveable;
						}
						else
							goto loop_break;
					}
					break;
					case 15:// optional bool unsight = 15;
					{
						if(tagLow == 120)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_BlockProjectile;
						}
						else
							goto loop_break;
					}
					break;
					case 16:// optional bool avoid = 16;
					{
						if(tagLow == 128)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_NotPathable;
						}
						else
							goto loop_break;
					}
					break;
					case 17:// optional bool no_movement_animation = 17;
					{
						if(tagLow == 136)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_NoMoveAnimation;
						}
						else
							goto loop_break;
					}
					break;
					case 18:// optional bool take = 18;
					{
						if(tagLow == 144)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Pickupable;
						}
						else
							goto loop_break;
					}
					break;
					case 19:// optional bool liquidcontainer = 19;
					{
						if(tagLow == 152)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_FluidContainer;
						}
						else
							goto loop_break;
					}
					break;
					case 20:// optional bool hang = 20;
					{
						if(tagLow == 160)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Hangable;
						}
						else
							goto loop_break;
					}
					break;
					case 21:// optional .tibia.protobuf.appearances.AppearanceFlagHook hook = 21;
					{
						if(tagLow == 170)
						{
							Sint64 hookLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < hookLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional .tibia.protobuf.shared.HOOK_TYPE direction = 1;
								{
									Uint64 direction = SDL_ReadProtobufVariant(rwops);
									if (direction == 1)
										m_flags |= ThingAttribute_HookSouth;
									else if (direction == 2)
										m_flags |= ThingAttribute_HookEast;
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 22:// optional bool rotate = 22;
					{
						if(tagLow == 176)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Rotateable;
						}
						else
							goto loop_break;
					}
					break;
					case 23:// optional .tibia.protobuf.appearances.AppearanceFlagLight light = 23;
					{
						if(tagLow == 186)
						{
							Sint64 lightLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < lightLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 brightness = 1;
								{
									m_flags |= ThingAttribute_Light;
									m_light[0] = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else if(tagHigh == 2 && tagLow == 16)// optional uint32 color = 2;
								{
									m_flags |= ThingAttribute_Light;
									m_light[1] = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 24:// optional bool dont_hide = 24;
					{
						if(tagLow == 192)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_DontHide;
						}
						else
							goto loop_break;
					}
					break;
					case 25:// optional bool translucent = 25;
					{
						if(tagLow == 200)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Translucent;
						}
						else
							goto loop_break;
					}
					break;
					case 26:// optional .tibia.protobuf.appearances.AppearanceFlagShift shift = 26;
					{
						if(tagLow == 210)
						{
							Sint64 displacementLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < displacementLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 x = 1;
								{
									m_flags |= ThingAttribute_Displacement;
									m_displacement[0] = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else if(tagHigh == 2 && tagLow == 16)// optional uint32 y = 2;
								{
									m_flags |= ThingAttribute_Displacement;
									m_displacement[1] = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 27:// optional .tibia.protobuf.appearances.AppearanceFlagHeight height = 27;
					{
						if(tagLow == 218)
						{
							Sint64 elevationLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < elevationLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 elevation = 1;
								{
									m_flags |= ThingAttribute_Elevation;
									m_elevation = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 28:// optional bool lying_object = 28;
					{
						if(tagLow == 224)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_LyingCorpse;
						}
						else
							goto loop_break;
					}
					break;
					case 29:// optional bool animate_always = 29;
					{
						if(tagLow == 232)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_AnimateAlways;
						}
						else
							goto loop_break;
					}
					break;
					case 30:// optional .tibia.protobuf.appearances.AppearanceFlagAutomap automap = 30;
					{
						if(tagLow == 242)
						{
							Sint64 minimapLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < minimapLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 color = 1;
								{
									m_flags |= ThingAttribute_MinimapColor;
									m_minimapColor = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 31:// optional .tibia.protobuf.appearances.AppearanceFlagLenshelp lenshelp = 31;
					{
						if(tagLow == 250)
						{
							Sint64 lensLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < lensLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 id = 1;
								{
									m_flags |= ThingAttribute_LensHelp;
									m_lensHelp = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 32:// optional bool fullbank = 32;
					{
						if(tagLow == 0)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_FullGround;
						}
						else
							goto loop_break;
					}
					break;
					case 33:// optional bool ignore_look = 33;
					{
						if(tagLow == 8)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_LookThrough;
						}
						else
							goto loop_break;
					}
					break;
					case 34:// optional .tibia.protobuf.appearances.AppearanceFlagClothes clothes = 34;
					{
						if(tagLow == 18)
						{
							Sint64 clothLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < clothLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 slot = 1;
								{
									m_flags |= ThingAttribute_Cloth;
									m_cloth = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 35:// optional .tibia.protobuf.appearances.AppearanceFlagDefaultAction default_action = 35;
					{
						if(tagLow == 26)
						{
							Sint64 actionLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < actionLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional .tibia.protobuf.shared.PLAYER_ACTION action = 1;
								{
									m_flags |= ThingAttribute_DefaultAction;
									m_defaultAction = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 36:// optional .tibia.protobuf.appearances.AppearanceFlagMarket market = 36;
					{
						if(tagLow == 34)
						{
							Sint64 marketLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < marketLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional .tibia.protobuf.shared.ITEM_CATEGORY category = 1;
								{
									m_flags |= ThingAttribute_Market;
									m_marketData.m_category = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else if(tagHigh == 2 && tagLow == 16)// optional uint32 trade_as_object_id = 2;
								{
									m_flags |= ThingAttribute_Market;
									m_marketData.m_tradeAs = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else if(tagHigh == 3 && tagLow == 24)// optional uint32 show_as_object_id = 3;
								{
									m_flags |= ThingAttribute_Market;
									m_marketData.m_showAs = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else if(tagHigh == 4 && tagLow == 34)// optional string name = 4;
								{
									m_flags |= ThingAttribute_Market;
									m_marketData.m_name = SDL_ReadProtobufString(rwops);
								}
								else if(tagHigh == 5 && (tagLow == 40 || tagLow == 42))// repeated .tibia.protobuf.shared.PLAYER_PROFESSION restrict_to_profession = 5;
								{
									m_flags |= ThingAttribute_Market;
									if(tagLow == 40)
									{
										bool haveData = true;
										do
										{
											Uint16 restrictedVocs = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
											if(restrictedVocs == 0)
												m_marketData.m_restrictVocation = 0;//None
											else if(restrictedVocs == 1)
												m_marketData.m_restrictVocation = 1;//Knight
											else if(restrictedVocs == 2)
												m_marketData.m_restrictVocation = 2;//Paladin
											else if(restrictedVocs == 3)
												m_marketData.m_restrictVocation = 3;//Sorcerer
											else if(restrictedVocs == 4)
												m_marketData.m_restrictVocation = 4;//Druid
											else if(restrictedVocs == 10)
												m_marketData.m_restrictVocation = 10;//Promoted
											else
												m_marketData.m_restrictVocation = 0xFFFF;//Any

											haveData = (SDL_RWtell(rwops) < marketLimit);
										} while(haveData && SDL_ReadU8(rwops) == 40);
										if(haveData)//Rewind one byte
											SDL_RWseek(rwops, SDL_RWtell(rwops)-1, RW_SEEK_SET);
									}
									else if(tagLow == 42)
									{
										Sint64 spriteIdLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
										while(SDL_RWtell(rwops) < spriteIdLimit)
										{
											Uint16 restrictedVocs = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
											if(restrictedVocs == 0)
												m_marketData.m_restrictVocation = 0;//None
											else if(restrictedVocs == 1)
												m_marketData.m_restrictVocation = 1;//Knight
											else if(restrictedVocs == 2)
												m_marketData.m_restrictVocation = 2;//Paladin
											else if(restrictedVocs == 3)
												m_marketData.m_restrictVocation = 3;//Sorcerer
											else if(restrictedVocs == 4)
												m_marketData.m_restrictVocation = 4;//Druid
											else if(restrictedVocs == 10)
												m_marketData.m_restrictVocation = 10;//Promoted
											else
												m_marketData.m_restrictVocation = 0xFFFF;//Any
										}
									}
								}
								else if(tagHigh == 6 && tagLow == 48)// optional uint32 minimum_level = 6;
								{
									m_flags |= ThingAttribute_Market;
									m_marketData.m_requiredLevel = SDL_static_cast(Uint16, SDL_ReadProtobufVariant(rwops));
								}
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 37:// optional bool wrap = 37;
					{
						if(tagLow == 40)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Wrapable;
						}
						else
							goto loop_break;
					}
					break;
					case 38:// optional bool unwrap = 38;
					{
						if(tagLow == 48)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Unwrapable;
						}
						else
							goto loop_break;
					}
					break;
					case 39:// optional bool topeffect = 39;
					{
						if(tagLow == 56)
						{
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_TopEffect;
						}
						else
							goto loop_break;
					}
					break;
					case 40:// repeated .tibia.protobuf.appearances.AppearanceFlagNPC npcsaledata = 40;
					{
						if(tagLow == 66)
						{
							bool haveData = true;
							do
							{
								Sint64 actionLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
								while(SDL_RWtell(rwops) < actionLimit)
								{
									tag = SDL_ReadProtobufTag(rwops);
									tagHigh = (tag >> 3);
									tagLow = SDL_static_cast(Uint8, tag);
									if(tagHigh == 1 && tagLow == 10)// optional string name = 1;
										SDL_ReadProtobufString(rwops);
									else if(tagHigh == 2 && tagLow == 18)// optional string location = 2;
										SDL_ReadProtobufString(rwops);
									else if(tagHigh == 3 && tagLow == 24)// optional uint32 sale_price = 3;
										SDL_ReadProtobufVariant(rwops);
									else if(tagHigh == 4 && tagLow == 32)// optional uint32 buy_price = 4;
										SDL_ReadProtobufVariant(rwops);
									else
										break;
								}
								haveData = (SDL_RWtell(rwops) < flagsLimit-1);
							} while(haveData && SDL_ReadLE16(rwops) == 706);
							if(haveData)//Rewind two bytes
								SDL_RWseek(rwops, SDL_RWtell(rwops)-2, RW_SEEK_SET);
						}
						else
							goto loop_break;
					}
					break;
					case 41:// optional .tibia.protobuf.appearances.AppearanceFlagChangedToExpire changedtoexpire = 41;
					{
						if(tagLow == 74)
						{
							Sint64 expireLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < expireLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 former_object_typeid = 1;
									SDL_ReadProtobufVariant(rwops);
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					case 42:// optional bool corpse = 42;
					{
						if(tagLow == 80)
						{
							//Corpse
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Container;
						}
						else
							goto loop_break;
					}
					break;
					case 43:// optional bool player_corpse = 43;
					{
						if(tagLow == 88)
						{
							//Player Corpse
							if(SDL_static_cast(bool, SDL_ReadProtobufVariant(rwops)))
								m_flags |= ThingAttribute_Container;
						}
						else
							goto loop_break;
					}
					break;
					case 44:// optional .tibia.protobuf.appearances.AppearanceFlagCyclopedia cyclopediaitem = 44;
					{
						if(tagLow == 98)
						{
							Sint64 cyclopediaLimit = SDL_static_cast(Sint64, SDL_ReadProtobufSize(rwops))+SDL_RWtell(rwops);
							while(SDL_RWtell(rwops) < cyclopediaLimit)
							{
								tag = SDL_ReadProtobufTag(rwops);
								tagHigh = (tag >> 3);
								tagLow = SDL_static_cast(Uint8, tag);
								if(tagHigh == 1 && tagLow == 8)// optional uint32 cyclopedia_type = 1;
									SDL_ReadProtobufVariant(rwops);
								else
									break;
							}
						}
						else
							goto loop_break;
					}
					break;
					default: goto loop_break;
				}

				continue;
				loop_break:
				break;
			}
		}
		else if(tagHigh == 4 && tagLow == 34)// optional string name = 4;
			m_marketData.m_name = SDL_ReadProtobufString(rwops);
		else
			break;
	}
	return true;
}

Uint32 ThingType::getSprite(ThingFrameGroup f, Uint8 w, Uint8 h, Uint8 l, Uint8 x, Uint8 y, Uint8 z, Uint8 a)
{
	FrameGroup& frame = m_frameGroup[f];
	size_t index = (((((a * frame.m_patternZ + z) * frame.m_patternY + y) * frame.m_patternX + x) * frame.m_layers + l) * frame.m_height + h) * frame.m_width + w;
	return (index < frame.m_sprites.size() ? frame.m_sprites[index] : 0);
}

ThingManager::ThingManager()
{
	m_datLoaded = false;
}

ThingManager::~ThingManager()
{
	clear();
}

void ThingManager::clear()
{
	for(Sint32 i = 0; i < ThingCategory_Last; ++i)
	{
		for(std::vector<ThingType>::iterator it = m_things[i].begin(), end = m_things[i].end(); it != end; ++it)
			(*it).clear();

		m_things[i].clear();
	}
}

void ThingManager::unloadDat()
{
	m_datLoaded = false;
	clear();
}

bool ThingManager::loadDat(const char* filename)
{
	clear();

	try
	{
		SDL_RWops* file = SDL_RWFromFile(filename, "rb");
		if(file)
		{
			SDL_RWseek(file, 0, RW_SEEK_END);
			Sint32 fileSize = SDL_static_cast(Sint32, SDL_RWtell(file));
			SDL_RWseek(file, 0, RW_SEEK_SET);
			unsigned char* data = SDL_reinterpret_cast(unsigned char*, SDL_malloc(fileSize));
			if(data)
			{
				SDL_RWread(file, data, 1, SDL_static_cast(size_t, fileSize));
				SDL_RWclose(file);
				file = SDL_RWFromMem(data, fileSize);
			}
			g_datRevision = SDL_ReadLE32(file);
			for(Sint32 i = 0; i < ThingCategory_Last; ++i)
			{
				Uint16 things = SDL_ReadLE16(file) + 1;
				m_things[i].resize(things);
			}

			for(Sint32 i = 0; i < ThingCategory_Last; ++i)
			{
				Uint16 startId = (i == ThingCategory_Item ? 100 : 1);
				size_t size = m_things[i].size();
				for(Uint16 id = startId; id < size; ++id)
				{
					ThingType& tType = m_things[i][id];
					if(!tType.loadType(id, SDL_static_cast(ThingCategory, i), file))
						return false;
				}
			}

			m_datLoaded = true;
			SDL_RWclose(file);
			if(data)
				SDL_free(data);

			return true;
		}
		else
			return false;
	}
	catch(...)
	{
		return false;
	}
}

bool ThingManager::loadAppearances(const char* filename)
{
	clear();

	try
	{
		SDL_RWops* file = SDL_RWFromFile(filename, "rb");
		if(file)
		{
			SDL_RWseek(file, 0, RW_SEEK_END);
			Sint32 fileSize = SDL_static_cast(Sint32, SDL_RWtell(file));
			SDL_RWseek(file, 0, RW_SEEK_SET);
			unsigned char* data = SDL_reinterpret_cast(unsigned char*, SDL_malloc(fileSize));
			if(data)
			{
				SDL_RWread(file, data, 1, SDL_static_cast(size_t, fileSize));
				SDL_RWclose(file);
				file = SDL_RWFromMem(data, fileSize);
			}

			Uint16 Items = 0;
			Uint16 Creatures = 0;
			Uint16 Effects = 0;
			Uint16 DistanceEffects = 0;

			std::list<ThingType> tempItems;
			std::list<ThingType> tempCreatures;
			std::list<ThingType> tempEffect;
			std::list<ThingType> tempDistanceEffects;
			while(true)
			{
				Uint32 tag = SDL_ReadProtobufTag(file);
				Uint32 tagHigh = (tag >> 3);
				Uint8 tagLow = SDL_static_cast(Uint8, tag);
				if(tagHigh == 1 && tagLow == 10)// repeated .tibia.protobuf.appearances.Appearance object = 1;
				{
					#ifdef HAVE_CXX11_SUPPORT
					tempItems.emplace_back();
					ThingType& tType = tempItems.back();
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), Items, file);
					#else
					ThingType tType;
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), Items, file);
					tempItems.push_back(tType);
					#endif
				}
				else if(tagHigh == 2 && tagLow == 18)// repeated .tibia.protobuf.appearances.Appearance outfit = 2;
				{
					#ifdef HAVE_CXX11_SUPPORT
					tempCreatures.emplace_back();
					ThingType& tType = tempCreatures.back();
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), Creatures, file);
					#else
					ThingType tType;
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), Creatures, file);
					tempCreatures.push_back(tType);
					#endif
					if(!tType.m_frameGroup[ThingFrameGroup_Moving].m_sprites.empty() && tType.m_frameGroup[ThingFrameGroup_Idle].m_sprites.empty())
					{
						FrameGroup& fromFrame = tType.m_frameGroup[ThingFrameGroup_Moving];
						FrameGroup& toFrame = tType.m_frameGroup[ThingFrameGroup_Idle];
						toFrame.m_sprites.assign(fromFrame.m_sprites.begin(), fromFrame.m_sprites.end());
						toFrame.m_animator = fromFrame.m_animator;
						toFrame.m_width = fromFrame.m_width;
						toFrame.m_height = fromFrame.m_height;
						toFrame.m_realSize = fromFrame.m_realSize;
						toFrame.m_layers = fromFrame.m_layers;
						toFrame.m_patternX = fromFrame.m_patternX;
						toFrame.m_patternY = fromFrame.m_patternY;
						toFrame.m_patternZ = fromFrame.m_patternZ;
						toFrame.m_animCount = fromFrame.m_animCount;
					}
				}
				else if(tagHigh == 3 && tagLow == 26)// repeated .tibia.protobuf.appearances.Appearance effect = 3;
				{
					#ifdef HAVE_CXX11_SUPPORT
					tempEffect.emplace_back();
					ThingType& tType = tempEffect.back();
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), Effects, file);
					#else
					ThingType tType;
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), Effects, file);
					tempEffect.push_back(tType);
					#endif
				}
				else if(tagHigh == 4 && tagLow == 34)// repeated .tibia.protobuf.appearances.Appearance missile = 4;
				{
					#ifdef HAVE_CXX11_SUPPORT
					tempDistanceEffects.emplace_back();
					ThingType& tType = tempDistanceEffects.back();
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), DistanceEffects, file);
					#else
					ThingType tType;
					tType.loadAppearance(SDL_static_cast(Sint64, SDL_ReadProtobufSize(file))+SDL_RWtell(file), DistanceEffects, file);
					tempDistanceEffects.push_back(tType);
					#endif
				}
				else
					break;
			}

			#define MOVE_THINGS(fromList, toVector)																		\
				do {																									\
					for(std::list<ThingType>::iterator it = fromList.begin(), end = fromList.end(); it != end; ++it)	\
					{																									\
						ThingType& cType = (*it);																		\
						ThingType& tType = toVector[cType.m_id];														\
						tType.m_id = cType.m_id;																		\
						tType.m_flags = cType.m_flags;																	\
						tType.m_category = cType.m_category;															\
						tType.m_groundSpeed = cType.m_groundSpeed;														\
						tType.m_writableSize[0] = cType.m_writableSize[0];												\
						tType.m_writableSize[1] = cType.m_writableSize[1];												\
						tType.m_displacement[0] = cType.m_displacement[0];												\
						tType.m_displacement[1] = cType.m_displacement[1];												\
						tType.m_light[0] = cType.m_light[0];															\
						tType.m_light[1] = cType.m_light[1];															\
						tType.m_elevation = cType.m_elevation;															\
						tType.m_minimapColor = cType.m_minimapColor;													\
						tType.m_lensHelp = cType.m_lensHelp;															\
						tType.m_cloth = cType.m_cloth;																	\
						tType.m_defaultAction = cType.m_defaultAction;													\
						tType.m_marketData.m_category = cType.m_marketData.m_category;									\
						tType.m_marketData.m_requiredLevel = cType.m_marketData.m_requiredLevel;						\
						tType.m_marketData.m_restrictVocation = cType.m_marketData.m_restrictVocation;					\
						tType.m_marketData.m_showAs = cType.m_marketData.m_showAs;										\
						tType.m_marketData.m_tradeAs = cType.m_marketData.m_tradeAs;									\
						tType.m_marketData.m_name.assign(cType.m_marketData.m_name);									\
						for(Sint32 i = 0; i < ThingFrameGroup_Last; ++i)												\
						{																								\
							tType.m_frameGroup[i].m_width = cType.m_frameGroup[i].m_width;								\
							tType.m_frameGroup[i].m_height = cType.m_frameGroup[i].m_height;							\
							tType.m_frameGroup[i].m_realSize = cType.m_frameGroup[i].m_realSize;						\
							tType.m_frameGroup[i].m_layers = cType.m_frameGroup[i].m_layers;							\
							tType.m_frameGroup[i].m_patternX = cType.m_frameGroup[i].m_patternX;						\
							tType.m_frameGroup[i].m_patternY = cType.m_frameGroup[i].m_patternY;						\
							tType.m_frameGroup[i].m_patternZ = cType.m_frameGroup[i].m_patternZ;						\
							tType.m_frameGroup[i].m_animCount = cType.m_frameGroup[i].m_animCount;						\
							tType.m_frameGroup[i].m_animator = cType.m_frameGroup[i].m_animator;						\
							tType.m_frameGroup[i].m_sprites.swap(cType.m_frameGroup[i].m_sprites);						\
						}																								\
					}																									\
					fromList.clear();																					\
				} while(0)

			if(Items > 0)
			{
				m_things[ThingCategory_Item].resize(Items + 1);
				MOVE_THINGS(tempItems, m_things[ThingCategory_Item]);
			}
			if(Creatures > 0)
			{
				m_things[ThingCategory_Creature].resize(Creatures + 1);
				MOVE_THINGS(tempCreatures, m_things[ThingCategory_Creature]);
			}
			if(Effects > 0)
			{
				m_things[ThingCategory_Effect].resize(Effects + 1);
				MOVE_THINGS(tempEffect, m_things[ThingCategory_Effect]);
			}
			if(DistanceEffects > 0)
			{
				m_things[ThingCategory_DistanceEffect].resize(DistanceEffects + 1);
				MOVE_THINGS(tempDistanceEffects, m_things[ThingCategory_DistanceEffect]);
			}
			#undef MOVE_THINGS

			m_datLoaded = true;
			SDL_RWclose(file);
			if(data)
				SDL_free(data);

			return true;
		}
		else
			return false;
	}
	catch(...)
	{
		return false;
	}
}

ThingType* ThingManager::getThingType(ThingCategory category, Uint16 id)
{
	if(category >= ThingCategory_Last || id >= m_things[category].size())
		return NULL;
	return &m_things[category][id];
}
