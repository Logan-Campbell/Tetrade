# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

project(
	TETRADE-PSX
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "Tetrade: PSn00bSDK PSX Tetris Clone"
)

psn00bsdk_add_executable(tetrade GPREL 
	src/main.c 
	src/engine/graphics2d.c 
	src/engine/timer.c 
	src/engine/input.c 
	src/engine/text.c 
	src/engine/audio.c
)

psn00bsdk_target_incbin(tetrade PRIVATE text_sheet_image gfx/character_sheet.tim)
psn00bsdk_target_incbin(tetrade PRIVATE big_font gfx/big_font.tim)

psn00bsdk_target_incbin(tetrade PRIVATE background_left gfx/background_left.tim)
psn00bsdk_target_incbin(tetrade PRIVATE background_right gfx/background_right.tim)
psn00bsdk_target_incbin(tetrade PRIVATE foreground_left gfx/foreground_left.tim)
psn00bsdk_target_incbin(tetrade PRIVATE foreground_right gfx/foreground_right.tim)

psn00bsdk_target_incbin(tetrade PRIVATE blocks gfx/blocks.tim)
psn00bsdk_target_incbin(tetrade PRIVATE blocks_small gfx/blocks_small.tim)
psn00bsdk_target_incbin(tetrade PRIVATE title gfx/title.tim)

psn00bsdk_target_incbin(tetrade PRIVATE click sfx/click1.vag)
psn00bsdk_target_incbin(tetrade PRIVATE confirm sfx/confirm.vag)
psn00bsdk_target_incbin(tetrade PRIVATE clear sfx/clear.vag)
psn00bsdk_target_incbin(tetrade PRIVATE place sfx/place.vag)
psn00bsdk_target_incbin(tetrade PRIVATE negative sfx/negative.vag)
psn00bsdk_target_incbin(tetrade PRIVATE hold sfx/hold.vag)


psn00bsdk_target_incbin(tetrade PRIVATE theme sfx/loop3.vag)


psn00bsdk_add_cd_image(
	iso      # Target name
	TETRADE_PSX # Output file name (= template.bin + template.cue)
	iso.xml  # Path to config file
	DEPENDS tetrade
)
