/*
 *	PROJECT: Capture
 *	FILE: HashMap.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <ntddk.h>

typedef unsigned int UINT;

#define HASH_MAP_SIZE	1024
#define HASH_MAP_POOL	'hMap'

typedef VOID (*HashMapVisitor)( PVOID context, UINT key, PVOID value );

typedef struct _Pair		Pair;
typedef struct _Entry		Entry;
typedef struct _HashMap		HashMap;

struct _Pair
{
	Pair*			next;
	Pair*			prev;

	UINT			key;
	PVOID			value;
};

struct _Entry
{
	KSPIN_LOCK	lock;

	Pair*		head;
	Pair*		tail;
};

struct _HashMap
{
	KSPIN_LOCK	lock;

	Entry*		map[HASH_MAP_SIZE];
};

BOOLEAN InitialiseHashMap( HashMap* hash_map );

VOID DestroyHashMap( HashMap* hash_map );

BOOLEAN InsertHashMap( HashMap* hash_map, UINT key, PVOID value );

Pair* FindHashMap( HashMap* hash_map, UINT key );

PVOID FindHashMapValue( HashMap* hash_map, UINT key );

BOOLEAN RemoveHashMap( HashMap* hash_map, UINT key );

VOID IterateHashMap(HashMap* hash_map, HashMapVisitor visitor, PVOID context);