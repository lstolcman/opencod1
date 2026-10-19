// bg_misc_items.c -- the item list

#include "g_public.h"
#include "bg_public.h"

// weapon slots are placeholders that BG_FillInWeaponItems fills in
// from the weapon files
// keep: the classname keeps the argument's quotes, and the strings are "" not NULL
#define EMPTY_WEAPON_ITEM( x ) \
	{ "emptyitem_" #x, "", {"", ""}, "", "", "", 0, IT_BAD, 0, 0, 0 }

gitem_t bg_itemlist[] =
{
	{ NULL, NULL, {NULL, NULL}, NULL, NULL, NULL, 0, IT_BAD, 0, 0, 0 },     // leave index 0 alone

	EMPTY_WEAPON_ITEM( "w01" ),
	EMPTY_WEAPON_ITEM( "w02" ),
	EMPTY_WEAPON_ITEM( "w03" ),
	EMPTY_WEAPON_ITEM( "w04" ),
	EMPTY_WEAPON_ITEM( "w05" ),
	EMPTY_WEAPON_ITEM( "w06" ),
	EMPTY_WEAPON_ITEM( "w07" ),
	EMPTY_WEAPON_ITEM( "w08" ),
	EMPTY_WEAPON_ITEM( "w09" ),
	EMPTY_WEAPON_ITEM( "w10" ),
	EMPTY_WEAPON_ITEM( "w11" ),
	EMPTY_WEAPON_ITEM( "w12" ),
	EMPTY_WEAPON_ITEM( "w13" ),
	EMPTY_WEAPON_ITEM( "w14" ),
	EMPTY_WEAPON_ITEM( "w15" ),
	EMPTY_WEAPON_ITEM( "w16" ),
	EMPTY_WEAPON_ITEM( "w17" ),
	EMPTY_WEAPON_ITEM( "w18" ),
	EMPTY_WEAPON_ITEM( "w19" ),
	EMPTY_WEAPON_ITEM( "w20" ),
	EMPTY_WEAPON_ITEM( "w21" ),
	EMPTY_WEAPON_ITEM( "w22" ),
	EMPTY_WEAPON_ITEM( "w23" ),
	EMPTY_WEAPON_ITEM( "w24" ),
	EMPTY_WEAPON_ITEM( "w25" ),
	EMPTY_WEAPON_ITEM( "w26" ),
	EMPTY_WEAPON_ITEM( "w27" ),
	EMPTY_WEAPON_ITEM( "w28" ),
	EMPTY_WEAPON_ITEM( "w29" ),
	EMPTY_WEAPON_ITEM( "w30" ),
	EMPTY_WEAPON_ITEM( "w31" ),
	EMPTY_WEAPON_ITEM( "w32" ),
	EMPTY_WEAPON_ITEM( "w33" ),
	EMPTY_WEAPON_ITEM( "w34" ),
	EMPTY_WEAPON_ITEM( "w35" ),
	EMPTY_WEAPON_ITEM( "w36" ),
	EMPTY_WEAPON_ITEM( "w37" ),
	EMPTY_WEAPON_ITEM( "w38" ),
	EMPTY_WEAPON_ITEM( "w39" ),
	EMPTY_WEAPON_ITEM( "w40" ),
	EMPTY_WEAPON_ITEM( "w41" ),
	EMPTY_WEAPON_ITEM( "w42" ),
	EMPTY_WEAPON_ITEM( "w43" ),
	EMPTY_WEAPON_ITEM( "w44" ),
	EMPTY_WEAPON_ITEM( "w45" ),
	EMPTY_WEAPON_ITEM( "w46" ),
	EMPTY_WEAPON_ITEM( "w47" ),
	EMPTY_WEAPON_ITEM( "w48" ),
	EMPTY_WEAPON_ITEM( "w49" ),
	EMPTY_WEAPON_ITEM( "w50" ),
	EMPTY_WEAPON_ITEM( "w51" ),
	EMPTY_WEAPON_ITEM( "w52" ),
	EMPTY_WEAPON_ITEM( "w53" ),
	EMPTY_WEAPON_ITEM( "w54" ),
	EMPTY_WEAPON_ITEM( "w55" ),
	EMPTY_WEAPON_ITEM( "w56" ),
	EMPTY_WEAPON_ITEM( "w57" ),
	EMPTY_WEAPON_ITEM( "w58" ),
	EMPTY_WEAPON_ITEM( "w59" ),
	EMPTY_WEAPON_ITEM( "w60" ),
	EMPTY_WEAPON_ITEM( "w61" ),
	EMPTY_WEAPON_ITEM( "w62" ),
	EMPTY_WEAPON_ITEM( "w63" ),
	EMPTY_WEAPON_ITEM( "w64" ),

	{
		"item_ammo_stielhandgranate_open",
		"grenade_pickup",
		{ "xmodel/ammo_stielhandgranate1", NULL },
		"gfx/icons/hud@steilhandgrenate",
		"gfx/icons/hud@steilhandgrenate",
		"Stielhandgranate_mp Ammo Open",
		10, IT_AMMO, -1, -1, -1
	},
	{
		"item_ammo_stielhandgranate_closed",
		"grenade_pickup",
		{ "xmodel/ammo_stielhandgranate2", NULL },
		"gfx/icons/hud@steilhandgrenate",
		"gfx/icons/hud@steilhandgrenate",
		"Stielhandgranate_mp Ammo Closed",
		10, IT_AMMO, -1, -1, -1
	},
	{
		"item_health_small",
		"health_pickup_small",
		{ "xmodel/health_small", NULL },
		"icons/iconh_small",
		NULL,
		"Small Health",
		10, IT_HEALTH, 0, 0, 0
	},
	{
		"item_health",
		"health_pickup_medium",
		{ "xmodel/health_medium", NULL },
		"icons/iconh_med",
		NULL,
		"Med Health",
		25, IT_HEALTH, 0, 0, 0
	},
	{
		"item_health_large",
		"health_pickup_large",
		{ "xmodel/health_large", NULL },
		"icons/iconh_large",
		NULL,
		"Large Health",
		50, IT_HEALTH, 0, 0, 0
	},

	{ NULL, NULL, {NULL, NULL}, NULL, NULL, NULL, 0, IT_BAD, 0, 0, 0 }
};

const int bg_numItems = sizeof( bg_itemlist ) / sizeof( bg_itemlist[0] ) - 1;
