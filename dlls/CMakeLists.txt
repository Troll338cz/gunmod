#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 2.6.0)
project (SVDLL)

set (SVDLL_LIBRARY server)

add_definitions(-DCLIENT_WEAPONS)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_definitions(-D_LINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set (SVDLL_SOURCES 
	agrunt.cpp
	airtank.cpp
	aflock.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	barnacle.cpp
	barney.cpp
	bigmomma.cpp
	bloater.cpp
	bmodels.cpp
	bullsquid.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	controller.cpp
	crossbow.cpp
	crowbar.cpp
	defaultai.cpp
	doors.cpp
	effects.cpp
	egon.cpp
	explode.cpp
	enttools.cpp
	flyingmonster.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	gamerules.cpp
	gargantua.cpp
	gauss.cpp
	genericmonster.cpp
	ggrenade.cpp
	globals.cpp
	glock.cpp
	gman.cpp
	h_ai.cpp
	h_battery.cpp
	h_cycler.cpp
	h_cine.cpp
	h_export.cpp
	handgrenade.cpp
	hassassin.cpp
	headcrab.cpp
	healthkit.cpp
	hgrunt.cpp
	hornet.cpp
	hornetgun.cpp
	houndeye.cpp
	ichthyosaur.cpp
	islave.cpp
	items.cpp
	leech.cpp
	lights.cpp
	maprules.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	mp5.cpp
	multiplay_gamerules.cpp
	nihilanth.cpp
	nodes.cpp
	osprey.cpp
	pathcorner.cpp
	plane.cpp
	plats.cpp
	player.cpp
	playermonster.cpp
	python.cpp
	rat.cpp
	roach.cpp
	rpg.cpp
	satchel.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	shotgun.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	sniperrifle.cpp
	squadmonster.cpp
	squeakgrenade.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	tentacle.cpp
	tempmonster.cpp
	triggers.cpp
	tripmine.cpp
	turret.cpp
	util.cpp
	weapons.cpp
	world.cpp
	xen.cpp
	zombie.cpp
	prop.cpp
	gravgun.cpp
	ar2.cpp
	big_cock.cpp
	unpredictedweapon.cpp
	coop.cpp
	gateofbabylon.cpp
	gravgunmod.cpp
	shock.cpp
	shockrifle.cpp
	vehicle.cpp
	displacer.cpp
	m249.cpp
	admin.cpp
	gunmod.cpp
	BMOD_flyingcrowbar.cpp
	BMOD_player.cpp
	bottle.cpp
	knife.cpp
	grapple.cpp
	sporegrenade.cpp
	sporelauncher.cpp
	spore_ammo.cpp
	hideandseek_gamerules.cpp
	dust.cpp
	portalgun.cpp
	portal_creator.cpp
	prop_portal.cpp
	santahat.cpp
	xtree.cpp
	gearbox/blkop_apache.cpp
	gearbox/blkop_osprey.cpp
	gearbox/cleansuit_scientist.cpp
	gearbox/drillsergeant.cpp
	gearbox/fgrunt.cpp
	gearbox/gearbox_effects.cpp
	gearbox/gearbox_triggers.cpp
	gearbox/gearbox_utils.cpp
	gearbox/generic_items.cpp
	gearbox/geneworm.cpp
	gearbox/gonome.cpp
	gearbox/houndeye_dead.cpp
	gearbox/islave_dead.cpp
	gearbox/loader.cpp
	gearbox/massn.cpp
	gearbox/nuclearbomb.cpp
	gearbox/op4mortar.cpp
	gearbox/otis.cpp
	gearbox/penguin.cpp
	gearbox/pitdrone.cpp
	gearbox/pitworm.cpp
	gearbox/recruit.cpp
	gearbox/ropes.cpp
	gearbox/shockroach.cpp
	gearbox/skeleton.cpp
	gearbox/strooper.cpp
	gearbox/voltigore.cpp
	gearbox/zombie_barney.cpp
	gearbox/zombie_soldier.cpp
	bshift/bshift_items.cpp
	bshift/rosenberg.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_shared.c
)
	
include_directories (. ./gearbox wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if(USE_VOICEMGR)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		../game_shared/voice_gamemgr.cpp)
else()
	add_definitions(-DNO_VOICEGAMEMGR)
endif()

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})

#set_target_properties (${SVDLL_LIBRARY} PROPERTIES
#	POSITION_INDEPENDENT_CODE 1)

if(APPLE OR WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")
	if(64BIT)
		set(SVDLL_NAME "${SERVER_LIBRARY_NAME}64")
	endif()

#	set_target_properties(${SVDLL_LIBRARY} PROPERTIES
#		OUTPUT_NAME ${SVDLL_NAME}
#		PREFIX "")
endif()

install( TARGETS ${SVDLL_LIBRARY}
	DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)
