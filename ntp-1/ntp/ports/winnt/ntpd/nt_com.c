/*
 * nt_com.c
 * 
 * Windows NT Serial I/O Support Module
 *
 * Win32 I/O interface functions to handle serial I/O
 * for the Trimble Palisade Reference Clock
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && defined(CLOCK_PALISADE)

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_control.h"
#include "ntp_refclock.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"
#include "ntp_if.h"


#include <stdio.h>
#include <string.h>

extern void palisade_io(int, int, unsigned char *, l_fp *, struct palisade_unit *);

#define NUMPORTS 4
#define LENCODE 74
#define TSIP_PARSED_FULL        1

struct palisade_unit {
	short 		unit;		/* NTP refclock unit number	*/
	int 		polled;		/* flag to detect noreplies 	*/
	short 		eventcnt;	/* Palisade event counter	*/
	l_fp 		codedelay;	/* time to hold HW signal for event */
	char		rpt_status;	/* TSIP Parser State		*/
	short 		rpt_code;	/* TSIP packet ID  		*/
	short 		rpt_cnt;	/* TSIP packet length so far 	*/
	char 		rpt_buf[LENCODE+1]; /* assembly buffer 		*/
};

void IOTask(struct peer *);
int SetupCommPort(int);
int nt_open_socket(int); 
int nt_io_start(struct peer *);
int nt_poll(struct refclockproc *, l_fp *);
BOOL BreakDownCommPort(struct peer *);

#define PORT "COM%d:"

static BOOL exit_flag = 0;
static BOOL polling = 0;	/* Flag to keep Threads from locking over COM Port */
static struct palisade_unit u;
static int sendsocket;

static HANDLE  hCommPort[NUMPORTS];
static HANDLE	IOThread[NUMPORTS];
static DWORD	tid[NUMPORTS];

int SetupCommPort(int unit) {
   COMMTIMEOUTS timeouts;

	char gszPort[6];
	DCB dcb = {0};
    //
    // open communication port handle
    //
	(void) sprintf(gszPort, PORT, unit + 1);

    hCommPort[unit] = CreateFile( gszPort, GENERIC_READ, 0, 0,
                                      OPEN_ALWAYS, 0, 0);

    if (hCommPort[unit] == INVALID_HANDLE_VALUE) {   
        msyslog(LOG_ERR, "NT_COM: Unit %d: Port %s CreateFile ", unit, gszPort);
        return -1;
    }

	if (!SetupComm( hCommPort[unit], 4096, 4096)) {
		msyslog(LOG_ERR, "SetupCommPort: Unit %d: SetupComm ", unit);
    return -1;
	}

	if (!GetCommState(hCommPort[unit], &dcb)) {
      // Error getting current DCB settings
		msyslog(LOG_ERR, "NT_COM: Unit %d: GetCommState", unit);
        return -1;
	}
    
	if (!BuildCommDCB("9600,o,8,1", &dcb)) {   
      // Couldn't build the DCB. Usually a problem
      // with the communications specification string.
	   msyslog(LOG_ERR, "NT_COM: Unit %d: BuildCommDCB", unit);
       return -1;
   }

   dcb.fOutxCtsFlow = 0;
   dcb.fOutxDsrFlow = 0;
   dcb.fDtrControl = DTR_CONTROL_DISABLE;
   dcb.fDsrSensitivity = 0;
   dcb.fOutX = 0; 
   dcb.fInX = 0;
   dcb.fErrorChar = 0;
   dcb.fNull = 0;
   dcb.fRtsControl = RTS_CONTROL_DISABLE;
   dcb.fAbortOnError = 0;
   dcb.ErrorChar = 0;
   dcb.EvtChar = 0;
   dcb.EofChar = 0;

	if (!SetCommState(hCommPort[unit], &dcb)) {
      // Error in SetCommState. Possibly a problem with the communications 
      // port handle or a problem with the DCB structure itself.
		msyslog(LOG_ERR, "NT_COM: Unit %d: SetCommState", unit);
        return -1;
	}
	
	timeouts.ReadIntervalTimeout = 20; 
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 100;

	   // Error setting time-outs.
	if (!SetCommTimeouts(hCommPort[unit], &timeouts)) {
		msyslog(LOG_ERR, "NT_COM: Unit %d: SetCommTimeOuts", unit);
        return -1;
	}

	return 0;
}


BOOL BreakDownCommPort(peer)
	struct peer *peer;

{
	struct refclockproc *pp;
	struct palisade_unit *up;

	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;

	exit_flag = 1;
	WaitForSingleObject(IOThread[up->unit], 1000);
    CloseHandle(hCommPort[up->unit]);
	closesocket(pp->io.fd);	
	closesocket(sendsocket);	
    return 0;
}



int nt_poll(
			pp,
			postpoll
			)
	struct refclockproc *pp;
	l_fp *postpoll;
{
	register struct palisade_unit *up;

	up = (struct palisade_unit *)pp->unitptr;

	polling = 1;

	get_systime(&pp->lastrec);
	if 	(!EscapeCommFunction(hCommPort[up->unit], SETRTS)) {

		msyslog (LOG_ERR, "NT_COM: Unit %d: Error set/clear RTS %m", up->unit);
		polling = 0;
		return -1;
	}
	if 	(!EscapeCommFunction(hCommPort[up->unit], CLRRTS)) {
		
		msyslog (LOG_ERR, "NT_COM: Unit %d: Error set/clear RTS %m", up->unit);
		polling = 0;
		return -1;
	}
	get_systime(postpoll);

	polling = 0;
	return 0;
}

int nt_io_socket (unit)
	int unit;
	{
  if (SetupCommPort(unit)) {
		msyslog (LOG_ERR, "NT_COM: Unit %d: SetupCommPort() failed!", unit);
		return -1;
  }

	if ((sendsocket = nt_open_socket(unit)) < 0) {
		msyslog (LOG_ERR, "NT_COM: Unit %d: open_socket() failed!", unit);
		return -1;
	};

	return nt_open_socket(unit);
	}


int nt_open_socket(unit)
	int unit;
 {
  struct sockaddr_in inaddr;
  int alen = sizeof(inaddr);
  int sv;
  if ((sv = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
    msyslog(LOG_ERR,"NT_COM: Unit %d: create_sockets: socket(AF_INET, SOCK_DGRAM) failed: %m", unit);
    return -1;
  }

    inaddr.sin_addr.s_addr = htonl(0x7F000001);
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = 0;

  if (bind (sv, (struct sockaddr *)&inaddr, sizeof(inaddr)) != 0) {
	  msyslog (LOG_ERR, "NT_COM: Unit %d: bind", unit);
	  return -1; 
  }

  if (getsockname(sv, (struct sockaddr *)&inaddr, &alen) != 0) {
	  msyslog (LOG_ERR, "NT_COM: Unit %d: getsockname", unit);
	  return -1; 
  }

  return sv;
}


int nt_io_start(peer)
	struct peer *peer;
{
	struct refclockproc *pp;
	struct palisade_unit *up;

	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;

	IOThread[up->unit] = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE) &IOTask, peer, 0, &tid[up->unit]);
	if (IOThread[up->unit] == NULL) {
		msyslog (LOG_ERR, "NT_COM: Unit %d: Error creating IO task", up->unit);
		return -1;
	}

   if (!SetThreadPriority(IOThread[up->unit], GetThreadPriority(IOThread[up->unit]) - 1)) {
      msyslog (LOG_ERR, "NT_COM: Unit %d: Cannot change thread priority",up->unit);
      return -1;
   }

	return 0;
}


void IOTask(peer)
	struct peer *peer;
{
	struct refclockproc *pp;
	struct palisade_unit *up;

	struct sockaddr_in addr;
	int alen = sizeof(addr);

	DWORD rc;
	DWORD dwCommEvent  = 0;
	DWORD dwCommStatus = 0;

	unsigned char inc;
	l_fp t_in;

	pp = peer->procptr;
	up = (struct palisade_unit *)pp->unitptr;

    if (getsockname(pp->io.fd, (struct sockaddr *)&addr, &alen) != 0) {
		msyslog (LOG_ERR, "NT_COM: Unit %d: getsockname() failed!", up->unit);
		ExitThread(0);
	}

	while (!exit_flag) {
		if (polling) {
			sleep(20);
			continue;
		}

		if (!ReadFile(hCommPort[up->unit], &inc, 1, &rc, 0)) {
			msyslog(LOG_ERR, "NT_COM: Unit %d: Error reading Com Port\n", 
						up->unit);
			exit_flag = 1;
		}

		if (rc == 1) {
 		//	printf(".");
			palisade_io((int) (pp->sloppyclockflag & CLK_FLAG2), rc, &inc, &t_in, &u);

			if (u.rpt_status == TSIP_PARSED_FULL) {
				if ((u.rpt_buf[1] || u.rpt_buf[2]) || 
					(pp->sloppyclockflag & CLK_FLAG2)) {
#ifdef DEBUG
//				if (debug > 3)
					printf("Unit %d: Sending Poll Reply %d\n",up->unit, pp->polls);
#endif
						memcpy (up->rpt_buf, u.rpt_buf, sizeof(u.rpt_buf));
						if (pp->sloppyclockflag & CLK_FLAG2) 
							pp->lastrec = t_in;
						up->rpt_status = TSIP_PARSED_FULL;
						up->rpt_cnt = u.rpt_cnt;
						up->rpt_code = u.rpt_code;

						/* send a 'token' message to trigger the receive function */
						if (sendto(sendsocket, &inc, 1, 0, 
							(struct sockaddr *) &addr, alen) == SOCKET_ERROR) {
								msyslog(LOG_ERR, "NT_COM: Unit %d: Socket Error\n", 
									up->unit);
								exit_flag = 1;
						}
				} /* polled */
			memset(u.rpt_buf, 0, sizeof(u.rpt_buf));
			u.rpt_status = 0;
			u.rpt_cnt = 0;
			u.rpt_code = 0;
			} /* full */
			rc = 0;
		} /* rc == 1 */
	} /* while ... */

	ExitThread(0);
}

#endif /* REFCLOCK */