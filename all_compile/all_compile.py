#!/usr/bin/python

import os, sys, struct, getopt
from sys import argv
from stat import *

emulator_name=-1
emulator=-1
extensions=-1
padding=-1

POCKETNES=0
GOOMBA=1
GOOMBACOLOR=2
COLOGNE=3
SMSADVANCE=4

NES_PAD=(32+4*4)
NES_NAME="pocketnes"
NES_EXT=[".nes"]

#pocketnes header
#typedef struct {
#        char name[32];
#        u32 filesize;
#        u32 flags;
#        u32 spritefollow;
#        u32 reserved;
#} romheader;

GOOMBA_PAD=0x108
GOOMBA_NAME="goomba"
GOOMBA_EXT=[".gb", ".sgb", ".gbc"]

#goomba header
#n/a

GOOMBACOLOR_PAD=0x108
GOOMBACOLOR_NAME="goombacolor"
GOOMBACOLOR_EXT=[".gb", ".sgb", ".gbc"]

#goombacolor header
#n/a

COLOGNE_EMUID=0x1A4C4F43
COLOGNE_PAD=(4*8+32)
COLOGNE_NAME="cologne"
COLOGNE_EXT=[".col", ".rom", ".bin"]

#cologne header
#typedef struct {
#        u32 identifier;
#        u32 filesize;
#        u32 flags;
#        u32 spritefollow;
#        u32 reserved[4];
#        char name[32];
#} romheader;

SMS_EMUID=0x1A534D53
SMS_PAD=(4*8+32)
SMS_NAME="smsadvance"
SMS_EXT=[".sms", ".gg"]

SMS_FLAG=0
GG_FLAG=4
BIOS_FLAG=1

#smsadvance header
#typedef struct {
#        u32 identifier;
#        u32 filesize;
#        u32 flags;
#        u32 spritefollow;
#        u32 reserved[4];
#        char name[32];
#} romheader;

EMULATORS = [[NES_NAME, POCKETNES, NES_EXT, NES_PAD],
	[GOOMBACOLOR_NAME, GOOMBACOLOR, GOOMBACOLOR_EXT, GOOMBACOLOR_PAD],
	[GOOMBA_NAME, GOOMBA, GOOMBA_EXT, GOOMBA_PAD],
	[SMS_NAME, SMSADVANCE, SMS_EXT, SMS_PAD],
	[COLOGNE_NAME, COLOGNE, COLOGNE_EXT, COLOGNE_PAD]]

emptysave = "\xff" * 65536

def readfile(name):
	try:
		fd = open(name, "rb")
		contents = fd.read()
		fd.close()
	except IOError:
		print "Error reading", name
		sys.exit(1)
	return contents

def writefile(name, contents):
	try:
		fd = open(name, "wb")
		fd.write(contents)
		fd.close()
	except IOError:
		print "Error writing", name
		sys.exit(1)

def usage(name):
	print "Usage: %s [-ap] [-i<image>] [-c<bios>] [-s<bios>] [-g<bios>] emulator.gba rom [romtitle] [rom2 [romtitle2] ...]" % name
	print "\t-a Auto title rom (don't include romtime on commandline)"
	print "\t-p Pad end of output rom for EZ-Flash IV"
	print "\t-i Include splash image"
	print "\t-c Cologne bios (mandatory for cologne)"
	print "\t-s SMS bios (optional for smsadavance)"
	print "\t-g GG bios (optional for smsadvance)"
	sys.exit(1)

if __name__ == "__main__":
	try:
		opts, args = getopt.gnu_getopt(argv[1:], "api:g:s:c:", [])
	except getopt.GetoptError:
		usage(argv[0])
		sys.exit(1)
	autoname = 0
	pad = 0
	splash_image = ""
	gg_bios_rom = ""
	cologne_bios_rom = "bios.col"
	sms_bios_rom = ""
	for o, a in opts:
		if o in ("-a"):
			autoname = 1
		if o in ("-p"):
			pad = 1
		if o in ("-i"):
			splash_image = readfile(a)
			if len(splash_image) != 76800:
				print "Splash image must be 76800 bytes"
				sys.exit(10)
		if o in ("-c"):
			cologne_bios_name = a
			cologne_bios_rom = readfile(a)
			cologne_bios_rom += "\0" * ((4-len(cologne_bios_rom)%4)%4)
		if o in ("-s"):
			sms_bios_name = a
			sms_bios_rom = readfile(a)
			sms_bios_rom += "\0" * ((4-len(sms_bios_rom)%4)%4)
		if o in ("-g"):
			gg_bios_name = a
			gg_bios_rom = readfile(a)
			gg_bios_rom += "\0" * ((4-len(gg_bios_rom)%4)%4)
	if len(args) < 2:
		usage(argv[0])
	emulator_rom = readfile(args[0])
	given_name = args[0].lower()
	slash = given_name.rfind("/")
	slash2 = given_name.rfind("\\")
	if slash2 > slash:
		slash = slash2
	given_name = given_name[slash+1:]
	for name, type, ext, p in EMULATORS:
		if len(given_name) >= len(name):
			if given_name[:len(name)] == name:
				emulator_name = name
				emulator = type
				extensions = ext
				padding = p
				break
	if emulator == -1:
		usage(argv[0])
	if len(args) < 2:
		print "Roms required"
		sys.exit(3)
	out = emulator_rom
	out += "\0" * ((4-len(out)%4)%4)
	if splash_image:
		out += splash_image
	if emulator == COLOGNE:
		if not cologne_bios_rom:
			print "Cologne BIOS missing.  You must specify a bios."
			sys.exit(8)
		biosheader = struct.pack("2Ih2bIb3b3I31sc", COLOGNE_EMUID, len(cologne_bios_rom), 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, cologne_bios_name, "\0")
		out += biosheader + cologne_bios_rom
		out += "\0" * ((4-len(out)%4)%4)
	if emulator == SMSADVANCE:
		if sms_bios_rom:
			biosheader = struct.pack("8I31sc", SMS_EMUID, len(sms_bios_rom), SMS_FLAG, 0, BIOS_FLAG, 0, 0, 0, sms_bios_name, "\0")
			out += biosheader + sms_bios_rom
			out += "\0" * ((4-len(out)%4)%4)
		if gg_bios_rom:
			biosheader = struct.pack("8I31sc", SMS_EMUID, len(gg_bios_rom), GG_FLAG, 0, BIOS_FLAG, 0, 0, 0, gg_bios_name, "\0")
			out += biosheader + gg_bios_rom
			out += "\0" * ((4-len(out)%4)%4)
	idx = 1
	romcount = 0
	while idx < len(args):
		romfile = args[idx]
		idx += 1
		ext = romfile.rfind(".")
		if ext == -1:
			print "Rom without extension specified.  Cannot continue."
			sys.exit(4)
		e = romfile[ext:].lower()
		if not e in extensions:
			print "Invalid extension %s.  Cannot continue." % args[idx][ext:].lower()
			sys.exit(5)
		rom = readfile(romfile)
		if emulator == GOOMBA or emulator == GOOMBACOLOR or autoname == 1:
			rom_name = romfile[:ext]
		else:
			if idx == len(args):
				print "Rom title missing."
				sys.exit(7)
			rom_name = args[idx]
			idx += 1
		if emulator == POCKETNES:
			romheader = struct.pack("31scIIII", rom_name, "\0", len(rom), 0, 0, 0)
			out += romheader + rom
		if emulator == GOOMBA or emulator == GOOMBACOLOR:
			if rom[0:4] != "TRIM":
				size = 32768<<ord(rom[0x148])
				if len(rom) > size:
					print "Rom is too large.  Aborting."
					sys.exit(6)
				if len(rom) < size:
					rom += "\0" * size - len(rom)
			out += rom
		if emulator == SMSADVANCE:
			if e == ".sms":
				romheader = struct.pack("8I31sc", SMS_EMUID, len(rom), SMS_FLAG, 0, 0, 0, 0, 0, rom_name, "\0")
			else:
				romheader = struct.pack("8I31sc", SMS_EMUID, len(rom), GG_FLAG, 0, 0, 0, 0, 0, rom_name, "\0")
			out += romheader + rom
		if emulator == COLOGNE:
			romheader = struct.pack("8I31sc", COLOGNE_EMUID, len(rom), 0, 0, 0, 0, 0, 0, rom_name, "\0")
			out += romheader + rom
		out += "\0" * ((4-len(out)%4)%4)
		romcount += 1
	if pad:
		out = out + "\0" * padding
	if romcount == 1:
		outputfilename = os.path.splitext(romfile)[0] + ".gba"	
		outputsavename = os.path.splitext(romfile)[0] + ".sav"
	else:
		outputfilename = "%s_out.gba" % emulator_name
		outputsavename = "%s_out.sav" % emulator_name
	writefile(outputfilename, out)
	writefile(outputsavename, emptysave)
