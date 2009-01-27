#define NTDDI_WINXPSP2                      0x05010200
#define OSVERSION_MASK      0xFFFF0000
#define SPVERSION_MASK      0x0000FF00
#define SUBVERSION_MASK     0x000000FF


//
// macros to extract various version fields from the NTDDI version
//
#define OSVER(Version)  ((Version) & OSVERSION_MASK)
#define SPVER(Version)  (((Version) & SPVERSION_MASK) >> 8)
#define SUBVER(Version) (((Version) & SUBVERSION_MASK) )
//#define NTDDI_VERSION   NTDDI_WINXPSP2

#include <ntifs.h>
#include <wdm.h>


typedef struct _ConnectionChecker
{
	ULONG last_contact_time;
	ULONG wait_contact_time;
	KTIMER connection_timer;
	KDPC timer_deferred_procedure;
	LONG volatile connected;
} ConnectionChecker;

LONGLONG GetCurrentTime()
{
	LARGE_INTEGER current_system_time;
	LARGE_INTEGER current_local_time;
	//ULONG time;

	KeQuerySystemTime( &current_system_time );
	ExSystemTimeToLocalTime( &current_system_time, &current_local_time );
	//RtlTimeToSecondsSince1970( &current_local_time , &time );
	return current_local_time.QuadPart;
}

VOID UpdateLastContactTime( ConnectionChecker* checker )
{
	checker->last_contact_time = GetCurrentTime();
}

VOID ConnectionCheckTimer( IN struct _KDPC  *Dpc,
							 IN PVOID  DeferredContext,
							 IN PVOID  SystemArgument1,
							 IN PVOID  SystemArgument2 )
{
	ULONG last_contact_difference = 0;
	ConnectionChecker* checker = (ConnectionChecker*)DeferredContext;

	last_contact_difference = GetCurrentTime();
	last_contact_difference -= checker->last_contact_time;

	if( last_contact_difference > checker->wait_contact_time )
	{
		// No contact has been received in wait_contact_time
		InterlockedBitTestAndReset( &checker->connected, 0 );
	}
	else
	{
		InterlockedBitTestAndSet( &checker->connected, 0 );
	}
}

VOID InitialiseConnectionChecker( ConnectionChecker* checker, LONG connection_timeout )
{
	LARGE_INTEGER timeout;

	timeout.QuadPart = 0; // Timer doesn't expire

	checker->wait_contact_time = connection_timeout;

	UpdateLastContactTime( checker );

	InterlockedBitTestAndSet( &checker->connected, 0 );

	KeInitializeDpc( &checker->timer_deferred_procedure, (PKDEFERRED_ROUTINE)ConnectionCheckTimer, checker);

	KeInitializeTimer( &checker->connection_timer );

	KeSetTimerEx( &checker->connection_timer, 
		timeout, checker->wait_contact_time, 
		&checker->timer_deferred_procedure );
}

VOID DeleteConnectionChecker( ConnectionChecker* checker )
{
	// Not needed as checker is in a device extension so is called automatically
	//KeFlushQueuedDpcs( ); // Wait just in case the timer function has been queued to be executed

	KeCancelTimer( &checker->connection_timer );
}
