/*
 * token - Chat for token-ring topology based networks
 *
 * Copyright (C) 2014  Rafael Ravedutti Lucio Machado
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define MACHINE_TAG             'D'
#define MACHINE_HOST            "priorat"
#define MACHINE_DEST_HOST       "dalmore"
#define MACHINE_SOURCE_PORT     63924
#define MACHINE_DEST_PORT       63921
#define MAX_MSG_LENGTH          1024
#define MAX_MSG_PER_TIME        20
#define BAT_TIME                0.5
#define AVAIABLE_TAGS           "ABCD"

#include "ring.c"
