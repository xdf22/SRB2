
set (SRB2_CORE_SOURCES
	comptime.c
	md5.c
	config.h.in
	string.c
	d_main.c
	dehacked.c
	deh_soc.c
	deh_lua.c
	deh_tables.c
	z_zone.c
	f_finale.c
	f_wipe.c
	g_demo.c
	g_game.c
	g_input.c
	am_map.c
	command.c
	console.c
	hu_stuff.c
	i_time.c
	y_inter.c
	st_stuff.c
	m_aatree.c
	m_anigif.c
	m_argv.c
	m_bbox.c
	m_cheat.c
	m_cond.c
	m_easing.c
	m_fixed.c
	m_menu.c
	m_misc.c
	m_perfstats.c
	m_random.c
	m_tokenizer.c
	m_queue.c
	m_vector.c
	info.c
	p_ceilng.c
	p_enemy.c
	p_floor.c
	p_inter.c
	p_lights.c
	p_map.c
	p_maputl.c
	p_mobj.c
	p_polyobj.c
	p_saveg.c
	p_setup.c
	p_sight.c
	p_spec.c
	p_telept.c
	p_tick.c
	p_user.c
	p_slopes.c
	tables.c
	r_bsp.c
	r_data.c
	r_draw.c
	r_fps.c
	r_main.c
	r_plane.c
	r_segs.c
	r_skins.c
	r_sky.c
	r_splats.c
	r_things.c
	r_bbox.c
	r_textures.c
	r_translation.c
	r_patch.c
	r_patchrotation.c
	r_picformats.c
	r_portal.c
	screen.c
	taglist.c
	v_video.c
	s_sound.c
	sounds.c
	w_wad.c
	filesrch.c
	lzf.c
	b_bot.c
	u_list.c
	snake.c
	lua_script.c
	lua_baselib.c
	lua_mathlib.c
	lua_hooklib.c
	lua_consolelib.c
	lua_infolib.c
	lua_mobjlib.c
	lua_playerlib.c
	lua_skinlib.c
	lua_thinkerlib.c
	lua_maplib.c
	lua_taglib.c
	lua_polyobjlib.c
	lua_blockmaplib.c
	lua_hudlib.c
	lua_hudlib_drawlist.c
	lua_colorlib.c
	lua_inputlib.c
)

set (SRB2_BLUA_SOURCES
	lapi.c
	lbaselib.c
	ldo.c
	lfunc.c
	linit.c
	liolib.c
	llex.c
	lmem.c
	lobject.c
	lstate.c
	lstrlib.c
	ltablib.c
	lundump.c
	lzio.c
	lauxlib.c
	lcode.c
	ldebug.c
	ldump.c
	lgc.c
	lopcodes.c
	lparser.c
	lstring.c
	ltable.c
	ltm.c
	lvm.c
	loslib.c
)

set (SRB2_NETCODE_SOURCES
	d_clisrv.c
	server_connection.c
	client_connection.c
	tic_command.c
	net_command.c
	gamestate.c
	commands.c
	d_net.c
	d_netcmd.c
	d_netfil.c
	http-mserv.c
	i_tcp.c
	mserv.c
)

add_executable(SRB2DSI)

target_link_libraries(SRB2DSI
    curl
    mbedtls
    mbedx509
    mbedcrypto
    png
    z
    dswifi9
)
target_link_directories(SRB2DSI PRIVATE
    $ENV{BLOCKSDSEXT}/libcurl/lib
    $ENV{BLOCKSDSEXT}/mbedtls/lib
)
target_include_directories(SRB2DSI PRIVATE
    $ENV{BLOCKSDSEXT}/libcurl/include
    $ENV{BLOCKSDSEXT}/mbedtls/include
)

set (SRB2_NDS_SOURCES
    i_main.c
    i_video.c
    i_system.c
    i_cdmus.c
    i_sound.c
    i_net.c
)

list(TRANSFORM SRB2_CORE_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/src/")
list(TRANSFORM SRB2_NETCODE_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/src/netcode/")
list(TRANSFORM SRB2_BLUA_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/src/blua/")
target_sources(SRB2DSI PRIVATE ${SRB2_CORE_SOURCES} ${SRB2_BLUA_SOURCES} ${SRB2_NETCODE_SOURCES} ${SRB2_NDS_SOURCES})
target_compile_options(SRB2DSI PRIVATE -std=gnu17)
target_compile_definitions(SRB2DSI PRIVATE -D_NDS -DHAVE_ZLIB -DHAVE_PNG -DHAVE_CURL -DNOPOSTPROCESSING -DNOMD5 -DHAVE_THREADS)

nds_create_rom(SRB2DSI
    NAME "Sonic Robo Blast 2"
    SUBTITLE "v2.2.15"
    AUTHOR "STJr"
    ICON "icon.gif"
    NITROFS "${CMAKE_SOURCE_DIR}/assets/installer/"
)
