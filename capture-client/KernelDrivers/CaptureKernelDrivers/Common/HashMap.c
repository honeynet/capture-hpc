/*
*	PROJECT: Capture
*	FILE: HashMap.c
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

#include "HashMap.h"

BOOLEAN InitialiseHashMap( HashMap* hash_map )
{
	RtlZeroMemory( hash_map, sizeof(HashMap) );

	KeInitializeSpinLock(&hash_map->lock);

	return TRUE;
}

VOID DestroyHashMap( HashMap* hash_map )
{
	int i = 0;

	for(i = 0; i < HASH_MAP_SIZE; i++)
	{
		Pair* pair = NULL;
		Entry* entry = hash_map->map[i];

		if( entry == NULL )
			continue;

		pair = entry->head;

		while( pair != NULL )
		{
			Pair* next_pair = pair->next;

			ExFreePoolWithTag( pair, HASH_MAP_POOL );

			pair = next_pair;
		}

		ExFreePoolWithTag( entry, HASH_MAP_POOL );

	}
}

BOOLEAN InsertHashMap( HashMap* hash_map, UINT key, PVOID value )
{
	Pair* pair = NULL;
	UINT hash = key % HASH_MAP_SIZE;

	DbgPrint("InsertHashMap: k=%08x v=%08x\n", key, value);

	if( hash_map->map[hash] == NULL )
	{
		hash_map->map[hash] = (Entry*)ExAllocatePoolWithTag( NonPagedPool, sizeof(Entry), HASH_MAP_POOL );

		if( hash_map->map[hash] == NULL )
		{
			return FALSE;
		}

		KeInitializeSpinLock( &hash_map->map[hash]->lock );

		hash_map->map[hash]->head = NULL;
		hash_map->map[hash]->tail = NULL;
	}

	pair = (Pair*)ExAllocatePoolWithTag( NonPagedPool, sizeof(Pair), HASH_MAP_POOL );

	if( pair == NULL )
	{
		return FALSE;
	}

	// TODO Lock me real hard

	pair->key = key;
	pair->value = value;
	pair->next = NULL;
	pair->prev = hash_map->map[hash]->tail;

	if( hash_map->map[hash]->head == NULL )
	{
		hash_map->map[hash]->head = pair;
	}

	if( hash_map->map[hash]->tail != NULL )
	{
		hash_map->map[hash]->tail->next = pair;
	}

	hash_map->map[hash]->tail = pair;

	// TODO Unlock me gently

	return TRUE;
}

Pair* FindHashMap( HashMap* hash_map, UINT key )
{
	Pair* pair = NULL;
	UINT hash = key % HASH_MAP_SIZE;

	if( hash_map->map[hash] == NULL )
	{
		return NULL;
	}

	pair = hash_map->map[hash]->head;

	while( pair != NULL )
	{
		if( pair->key == key )
		{
			return pair;
		}

		pair = pair->next;
	}

	return NULL;
}

PVOID FindHashMapValue( HashMap* hash_map, UINT key )
{
	Pair* pair = FindHashMap(hash_map, key);

	if(pair == NULL)
		return NULL;

	return pair->value;
};

BOOLEAN RemoveHashMap( HashMap* hash_map, UINT key )
{
	Entry* entry = NULL;
	UINT hash = key % HASH_MAP_SIZE;
	Pair* pair = FindHashMap( hash_map, key );

	if( pair == NULL )
	{
		return FALSE;
	}

	if( pair->next != NULL )
	{
		pair->next->prev = pair->prev;
	}

	if( pair->prev != NULL )
	{
		pair->prev->next = pair->next;
	}

	entry = hash_map->map[hash];

	if( entry->head == pair )
	{
		entry->head = pair->next;
	}

	if( entry->tail == pair )
	{
		entry->tail = pair->prev;
	}

	ExFreePoolWithTag( pair, HASH_MAP_POOL );

	return TRUE;
}

VOID IterateHashMap(HashMap* hash_map, HashMapVisitor visitor, PVOID context)
{
	Pair* pair = NULL;
	unsigned int i = 0;

	for(i = 0; i < HASH_MAP_SIZE; i++)
	{
		if(hash_map->map[i] == NULL)
			continue;

		pair = hash_map->map[i]->head;

		while( pair != NULL )
		{
			visitor(context, pair->key, pair->value);
			pair = pair->next;
		}
	}
}