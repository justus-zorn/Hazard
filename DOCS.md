# Hazard Engine Documentation

# Command line
Hazard always executes the project in the current working directory.
When no arguments are given, Hazard starts in integrated mode, meaning that client and server are
combined in the same process.

To only run a server, the argument '--server' must be added.

To connect with an already running server, the argument '--connect' must be added, followed by the
URL of the server (optionally with :PORT, in case the server runs on a different port than the one
specified in config.lua) and the name of the player. Player names must be unique.

# Configuration
All configuration options must be contained in the file 'config.lua' at the root of the project
directory.

## Config.textures
Textures that must be loaded by the engine. All textures are contained in the subdirectory
'Textures'.
## Config.title
The title of the game window.
## Config.width
The width (in pixels) of the game window. Default is 800.
## Config.height
The height (in pixels) of the game window. Default is 800.
## Config.port
The UDP port to use for networking. Default is 34344.
## Config.max_players
The maximum number of players that can be in a game at the same time. Default is 32.

# Callbacks
All callback functions must be exported by the file 'main.lua' at the root of the project
directory.

## Game.on_tick(dt)
'Game.on_tick' is executed on every server tick. 'dt' is the time (in seconds) the last server tick
took.
## Game.on_login(player): boolean
'Game.on_login' is executed when a player tries to join a game. 'player' is the name of the player.
The function must return a boolean. A return value of 'true' means that the player is allowed to
join. When this function is called, the player is not yet registered, meaning that no API functions
can be used with the player's name.
## Game.on_join(player)
'Game.on_join' is executed after a player successfully joined a game. 'player' is the name of the
player. In contrast to 'Game.on_login', when this function is called, the player is fully
registered, meaning that API functions can be used with the player's name.
## Game.on_disconnect(player)
'Game.on_disconnect' is executed when a player disconnects. When this function is called, the
player still is registered, meaning that API functions can be used with the player's name.
## Game.on_key_event(player, key, pressed)
'Game.on_key_event' is executed when a player presses or releases a key on their keyboard. 'player'
is the player's name, 'key' is the name of the key that was pressed or released, and 'pressed' is a
boolean value that indicates whether the key is currently pressed ('true') or released ('false').
## Game.on_button_event(player, button, pressed)
'Game.on_button_event' is executed when a player presses or releases a button on their mouse.
'player' is the player's name, 'button' is the name of the button that was pressed or released, and
'pressed' is a boolean value that indicates whether the button is currently pressed ('true') or
released ('false').
## Game.on_axis_event(player, axis, state)
'Game.on_axis_event' is executed when a player moves their mouse. 'player' is the player's name,
'axis' is the name of the movement direction ('Mouse X' or 'Mouse Y'), and 'state' is a signed
integer value indicating the current position of the mouse on the screen (in pixels). The center of
the screen is at (0, 0).

# Functions
## get_players()
'get_players' returns an array of all players that are currently online.
## is_online(player)
'is_online' returns a boolean indicating whether 'player' is currently online.
## kick(player)
'kick' removes a player from the game. 'Game.on_disconnect' is still called.
## is_key_down(player, key)
'is_key_down' returns a boolean indicating whether 'player' is currently pressing 'key' on their
keyboard.
## is_button_down(player, button)
'is_button_down' returns a boolean indicating whether 'player' is currently pressing 'button' on
their mouse.
## get_axis(player, axis)
'get_axis' returns the current state of 'axis' for 'player'. Valid values for axis are 'Mouse X'
and 'Mouse Y'.
## draw_sprite(player, texture, x, y, size, frame_length?, animation_start?)
'draw_sprite' draws a square texture on the screen of the specified player. 'x' and 'y' are screen
coordinates (in pixels), where (0, 0) is the center of the screen. 'size' is the size of the
sprite (in pixels), which is independent of the actual size of the texture. 'frame_length' is
optional and specifies how long every frame of an animation should take (in milliseconds). By
default, no animation is played. 'animation_start' is also optional and specifies the starting
time of the animation (in ticks since the start of the game). The default value is 0.
## get_ticks()
'get_ticks' returns the number of milliseconds since the start of the game.
