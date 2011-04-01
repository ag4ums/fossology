/***************************************************************
Copyright (C) 2010-2011 Hewlett-Packard Development Company, L.P.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************/

/***************************************************************
 * @brief Common C agent functions
 *
 * This file contains common C agent funcitons and the 
 * API for working with the scheduler.
***************************************************************/

/* local includes */
#include "libfossscheduler.h"

#ifndef SVN_REV
#define SVN_REV "SVN_REV Unknown"
#endif

/* ************************************************************************** */
/* **** Locals ************************************************************** */
/* ************************************************************************** */

int  items_processed;   ///< the number of items processed by the agent
char buffer[2048];      ///< the last thing received from the scheduler
int  valid;             ///< if the information stored in buffer is valid
int  found;             ///< if the agent is even connected to the scheduler

/**
 * Global verbose flags that agents should use instead of specific verbose
 * flags. This is used by the scheduler to turn verbose on a particular agent
 * on during run time. When the verbose flag is turned on by the scheduler
 * the on_verbose function will be called. If nothing needs to be done when
 * verbose is turned on, simply pass NULL to scheduler_connect
 */
int verbose;

/**
 * @brief fo_heartbeat() isn an internal function to send a heartbeat to the 
 * scheduler along with the number of items processed.
 * Agents should NOT call this function directly.
 *
 * This is the alarm SIGALRM function.
 * @return void
 */
void fo_heartbeat()
{
  fprintf(stdout, "HEART: %d\n", items_processed);
  fflush(stdout);
  alarm(ALARM_SECS);
}

/* ************************************************************************** */
/* **** Global Functions **************************************************** */
/* ************************************************************************** */

/**
 * @brief fo_scheduler_heart() 
 * This function must be called by agents to let the scheduler know they
 * are alive and how many items they have processed.
 *
 * @param i   This is the number of itmes processed since the last call to 
 * fo_scheduler_heart()
 *
 * @return void
 */
void  fo_scheduler_heart(int i)
{
  items_processed += i;
}

/**
 * @brief fo_scheduler_connect()
 * Function to establish a connection between an agent and the scheduler.
 *
 * Steps taken by this function:
 *   - initialize memory associated with agent connection
 *   - send "SPAWNED" to the scheduler
 *   - receive the number of items between notifications
 *   - check the nfs mounts for the agent
 *   - set up the heartbeat()
 *
 * Making a call to this function should be the first thing that an agent does
 * after parsing its command line arguments.
 *
 * @param int* argc
 * @param char** argv
 * @returns void
 */
void fo_scheduler_connect(int* argc, char** argv)
{
  found = 0;

  /* check for --scheduler command line option */
  if(strcmp(argv[(*argc) - 1], "--scheduler_start") == 0)
  {
    fprintf(stdout, "%s\n", SVN_REV);
    (*argc)--;
    argv[*argc] = NULL;
    found = 1;
  }

  /* initialize memory associated with agent connection */
  items_processed = 0;
  memset(buffer, 0, sizeof(buffer));
  valid = 0;
  verbose = 0;

  /* send "OK" to the scheduler */
  if(found) 
  {
    fprintf(stdout, "OK\n");
    fflush(stdout);

    /* check the nfs mounts for the agent */
    // TODO

    /* set up the heartbeat() */
    signal(SIGALRM, fo_heartbeat);
    alarm(ALARM_SECS);
  }
}

/**
 * @brief Function to disconnect the scheduler connection.
 * Making a call to this function should be the last thing that an agent does
 * before exiting.
 *
 * @return There is no return.  This function calls an exit(0)
 */
void fo_scheduler_disconnect()
{
  /* send "CLOSED" to the scheduler */
  if(found) 
  {
    fprintf(stdout, "BYE\n");
    fflush(stdout);
  }

  /* call exit(0) */
  exit(0);
}

/**
 * @brief Get the next data to process from the scheduler.
 * It is the job of the agent to decide how this string is
 * interpreted.
 *
 * Steps taken by this function:
 *   - get the next line from the scheduler
 *     - if the scheduler has paused this agent this will block till unpaused
 *   - check for "CLOSE" from scheduler, return NULL if received
 *   - check for "VERBOSE" from scheduler
 *     - if this is received turn the verbose flag to whatever is specified
 *     - a new line must be received, perform same task (i.e. recursive call)
 *   - check for "END" from scheduler, if received print OK and recurse
 *     - this is used to simplify communications within the scheduler
 *   - return whatever has been received
 *
 * @return char* for the next thing to analyze, NULL if there is nothing
 *          left in this job, in which case the agent should close
 */
char* fo_scheduler_next()
{
  fflush(stdout);

  /* get the next line from the scheduler and possibly WAIT */
  if(fgets(buffer, sizeof(buffer), stdin) == NULL || strncmp(buffer, "CLOSE", 5) == 0)
  {
    valid = 0;
    return NULL;
  }
  else if(strncmp(buffer, "END", 3) == 0)
  {
    fprintf(stdout, "OK\n");
    fflush(stdout);
    return fo_scheduler_next();
  }
  else if(strncmp(buffer, "VERBOSE", 7) == 0)
  {
    verbose = atoi(&buffer[8]);
    valid = 0;
    return fo_scheduler_next();
  }
  else if(strncmp(buffer, "VERSION", 7) == 0)
  {
    fprintf(stdout, "%s\n", SVN_REV);
    valid = 0;
    return fo_scheduler_next();
  }

  valid = 1;
  return buffer;
}

/**
 * @brief Get the last read string from the scheduler.
 *
 * @return Returns the string buffer if it is valid.  
 * If it is not valid, return NULL
 * The buffer is not valid if the last received data from the scheduler
 * was a command, rather than data to operate on.
 */
char* fo_scheduler_current()
{
  return valid ? buffer : NULL;
}
