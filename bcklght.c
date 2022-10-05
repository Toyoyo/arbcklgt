/*armada-backlight.c -- Compaq Armada backlight control

  Copyright (c) 2001 Georg Acher, georg@acher.org, http://www.acher.org
  Copyright (c) 2002 Nat Pryce, nat.pryce@b13media.com, http://www.b13media.com

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.


  This program adjusts the LCD backlight brightness of the Compaq
  Armada M300,E500 and M700 by changing the PWM output of the FDC37N971
  Super-IO-chip. Pin 199 (PWM1) goes straight to the CCFL voltage
  converter.

  For further information look at the data sheet of the 37N971.

  Use the --help option to get usage information.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_PORT 0xe0
#define DATA_PORT (INDEX_PORT+1)

#define CONF_START 0x55
#define CONF_END 0xAA

#define REG_LOGDN 0x07
#define REG_CHIP_ID 0x20

#define REG_LDN_ACT 0x30
#define REG_LDN_BASEH 0x60
#define REG_LDN_BASEL 0x61

#define LOGICAL_DEVICE_MAILBOXES 0x09

#define MBX_PWM0 0x92
#define MBX_PWM1 0x93

static inline void outb(uint16_t port_, uint8_t value_);
#pragma aux outb = "out dx, al" parm[dx][al]

static inline uint8_t inb(uint16_t port_);
#pragma aux inb = "in al, dx" parm[dx] value[al]

static int clamp( int value, int min, int max ) {
    if( value < min ) {
        return min;
    }
    else if( value > max ) {
        return max;
    }
    else {
        return value;
    }
}

static void out_idx( int reg, int val ) {
    outb(INDEX_PORT, reg);
    outb(DATA_PORT, val);
}

static unsigned int in_idx( int reg ) {
    outb(INDEX_PORT, reg);
    return inb(DATA_PORT);
}


/** Opens the IO port of the backlight and returns the base register address
 *  for use as a handle to the device.
 */
int backlight_open() {
    int id;
    int base_addr;

    outb(INDEX_PORT, CONF_START);
    id = in_idx(REG_CHIP_ID);

    if( id!=0xa && id!=0xb ) {
        fprintf( stderr,"unknown chip id: %02x\n", id );
        exit(-1);
    }

    out_idx( REG_LOGDN, LOGICAL_DEVICE_MAILBOXES );
    base_addr = (in_idx(REG_LDN_BASEH)<<8) + in_idx(REG_LDN_BASEL);

#ifdef DEBUG
    fprintf( stderr, "base of LDN %i: %04x\n", 
                     LOGICAL_DEVICE_MAILBOXES, base_addr );
    fprintf( stderr, "activation of LDN %i: %02x\n",
                     LOGICAL_DEVICE_MAILBOXES, in_idx(REG_LDN_ACT) );
#endif

    return base_addr;
}


void backlight_close( int bl_addr ) {
    outb(INDEX_PORT, CONF_END);
}

int backlight_get_brightness( int bl_addr ) {
    outb(bl_addr, MBX_PWM1);
    return inb(bl_addr+1) >> 1;
}

void backlight_set_brightness( int bl_addr, int new_brightness ) {
    outb(bl_addr+1, clamp(new_brightness,0,63) << 1);
}


/*----------------------------------------------------------------------------
 */

static void print_help(char* progname ) {
    fprintf( stderr,
             "%1$s\n"
             "Copyright (c) 2001 Georg Acher, georg@acher.org\n"
             "Copyright (c) 2002 Nat Pryce, nat.pryce@b13media.com\n"
             "Sets brightness of Compaq Armada LCD backlight\n"
             "Usage: %1$s [<+|-|value>]\n"
             "no parameters: Returns current brightness\n"
             "+/-:           Increases/decreases brightness\n"
             "value:         Sets brightness (0<=value<=8)\n",
             progname );
}

static int help_requested( int argc, char **argv ) {
    int i;

    for( i = 0; i < argc; i++ ) {
        if( strcmp(argv[i],"--help") == 0 ) return 1;
    }
    return argc > 2;
}



int main( int argc, char **argv )
{
    int bl_addr;
    int current_brightness;

    if( help_requested( argc, argv ) ) {
        print_help( argv[0] );
        exit(0);
    }

    bl_addr = backlight_open();
    current_brightness = backlight_get_brightness( bl_addr );

    if( argc == 1 ) {
        printf( "%d\n", (current_brightness+1)/8 );
    }
    else if( strcmp(argv[1],"+") == 0 ) {
        backlight_set_brightness( bl_addr, current_brightness + 8 );
    }
    else if( strcmp(argv[1],"-") == 0 ) {
        backlight_set_brightness( bl_addr, current_brightness - 8 );
    }
    else {
        backlight_set_brightness( bl_addr, 8*atoi(argv[1]) );
    }

    backlight_close( bl_addr );
    return 0;
}
