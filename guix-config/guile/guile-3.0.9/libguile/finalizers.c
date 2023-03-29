/* Copyright 2012-2014,2018-2020,2022
     Free Software Foundation, Inc.

   This file is part of Guile.

   Guile is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Guile is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Guile.  If not, see
   <https://www.gnu.org/licenses/>.  */




#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <full-write.h>
#include <stdio.h>
#include <unistd.h>

#include "async.h"
#include "bdw-gc.h"
#include "gc.h"
#include "gsubr.h"
#include "init.h"
#include "threads.h"

#include "finalizers.h"




static int automatic_finalization_p = 1;

static size_t finalization_count;

static SCM run_finalizers_subr;




void
scm_i_set_finalizer (void *obj, scm_t_finalizer_proc proc, void *data)
{
  GC_finalization_proc prev;
  void *prev_data;
  GC_REGISTER_FINALIZER_NO_ORDER (obj, proc, data, &prev, &prev_data);
}

struct scm_t_chained_finalizer
{
  int resuscitating_p;
  scm_t_finalizer_proc proc;
  void *data;
  scm_t_finalizer_proc prev;
  void *prev_data;
};

static void
chained_finalizer (void *obj, void *data)
{
  struct scm_t_chained_finalizer *chained_data = data;
  if (chained_data->resuscitating_p)
    {
      if (chained_data->prev)
        scm_i_set_finalizer (obj, chained_data->prev, chained_data->prev_data);
      chained_data->proc (obj, chained_data->data);
    }
  else
    {
      chained_data->proc (obj, chained_data->data);
      if (chained_data->prev)
        chained_data->prev (obj, chained_data->prev_data);
    }
}

void
scm_i_add_resuscitator (void *obj, scm_t_finalizer_proc proc, void *data)
{
  struct scm_t_chained_finalizer *chained_data;
  chained_data = scm_gc_malloc (sizeof (*chained_data), "chained finalizer");
  chained_data->resuscitating_p = 1;
  chained_data->proc = proc;
  chained_data->data = data;
  GC_REGISTER_FINALIZER_NO_ORDER (obj, chained_finalizer, chained_data,
                                  &chained_data->prev,
                                  &chained_data->prev_data);
}

static void
shuffle_resuscitators_to_front (struct scm_t_chained_finalizer *cd)
{
  while (cd->prev == chained_finalizer)
    {
      struct scm_t_chained_finalizer *prev = cd->prev_data;
      scm_t_finalizer_proc proc = cd->proc;
      void *data = cd->data;

      if (!prev->resuscitating_p)
        break;

      cd->resuscitating_p = 1;
      cd->proc = prev->proc;
      cd->data = prev->data;

      prev->resuscitating_p = 0;
      prev->proc = proc;
      prev->data = data;

      cd = prev;
    }
}

void
scm_i_add_finalizer (void *obj, scm_t_finalizer_proc proc, void *data)
{
  struct scm_t_chained_finalizer *chained_data;
  chained_data = scm_gc_malloc (sizeof (*chained_data), "chained finalizer");
  chained_data->resuscitating_p = 0;
  chained_data->proc = proc;
  chained_data->data = data;
  GC_REGISTER_FINALIZER_NO_ORDER (obj, chained_finalizer, chained_data,
                                  &chained_data->prev,
                                  &chained_data->prev_data);
  shuffle_resuscitators_to_front (chained_data);
}




static SCM
run_finalizers_async_thunk (void)
{
  scm_run_finalizers ();
  return SCM_UNSPECIFIED;
}


/* The function queue_finalizer_async is run by the GC when there are
 * objects to finalize.  It will enqueue an asynchronous call to
 * GC_invoke_finalizers() at the next SCM_TICK in this thread.
 */
static void
queue_finalizer_async (void)
{
  scm_thread *t = SCM_I_CURRENT_THREAD;

  /* Could be that the current thread is is NULL when we're allocating
     in threads.c:guilify_self_1.  In that case, rely on the
     GC_invoke_finalizers call there after the thread spins up.  */
  if (!t) return;

  scm_system_async_mark_for_thread (run_finalizers_subr, t->handle);
}




#if SCM_USE_PTHREAD_THREADS

static int finalization_pipe[2] = { -1, -1 };
static scm_i_pthread_mutex_t finalization_thread_lock =
  SCM_I_PTHREAD_MUTEX_INITIALIZER;
static pthread_t finalization_thread;
static int finalization_thread_is_running = 0;

static void
notify_finalizers_to_run (void)
{
  char byte = 0;
  full_write (finalization_pipe[1], &byte, 1);
}

static void
notify_about_to_fork (void)
{
  char byte = 1;
  full_write (finalization_pipe[1], &byte, 1);
}

static void
reset_finalization_pipe (void)
{
  close (finalization_pipe[0]);
  close (finalization_pipe[1]);
  finalization_pipe[0] = -1;
  finalization_pipe[1] = -1;
}

struct finalization_pipe_data
{
  char byte;
  ssize_t n;
  int err;
};

static void*
read_finalization_pipe_data (void *data)
{
  struct finalization_pipe_data *fdata = data;
  
  fdata->n = read (finalization_pipe[0], &fdata->byte, 1);
  fdata->err = errno;

  return NULL;
}
  
static void*
finalization_thread_proc (void *unused)
{
  while (1)
    {
      struct finalization_pipe_data data;

      scm_without_guile (read_finalization_pipe_data, &data);

      if (data.n == 0)
        /* The other end of the pipe was closed, so exit.  */
        return NULL;
      else if (data.n < 0)
        {
          if (data.err != EINTR)
            {
              errno = data.err;
              perror ("error in finalization thread");
              return NULL;
            }
        }
      else
        {
          switch (data.byte)
            {
            case 0:
              scm_run_finalizers ();
              break;
            case 1:
              return NULL;
            default:
              abort ();
            }
        }
    }
}

static void*
run_finalization_thread (void *arg)
{
  return scm_with_guile (finalization_thread_proc, arg);
}

static void
start_finalization_thread (void)
{
  scm_i_pthread_mutex_lock (&finalization_thread_lock);
  if (!finalization_thread_is_running)
    {
      assert (finalization_pipe[0] == -1);

      /* Use the raw pthread API and scm_with_guile, because we don't want
	 to block on any lock that scm_spawn_thread might want to take,
	 and we don't want to inherit the dynamic state (fluids) of the
	 caller.  */
      if (pipe2 (finalization_pipe, O_CLOEXEC) != 0)
	perror ("error creating finalization pipe");
      else if (pthread_create (&finalization_thread, NULL,
			       run_finalization_thread, NULL))
	{
	  reset_finalization_pipe ();
	  perror ("error creating finalization thread");
	}
      else
	{
	  GC_set_finalizer_notifier (notify_finalizers_to_run);
	  finalization_thread_is_running = 1;
	}
    }
  scm_i_pthread_mutex_unlock (&finalization_thread_lock);
}

static void
stop_finalization_thread (void)
{
  scm_i_pthread_mutex_lock (&finalization_thread_lock);
  if (finalization_thread_is_running)
    {
      notify_about_to_fork ();
      if (pthread_join (finalization_thread, NULL))
        perror ("joining finalization thread");

      reset_finalization_pipe ();
      finalization_thread_is_running = 0;
    }
  scm_i_pthread_mutex_unlock (&finalization_thread_lock);
}

static void
spawn_finalizer_thread (void)
{
  start_finalization_thread ();
}

#endif /* SCM_USE_PTHREAD_THREADS */




void
scm_i_finalizer_pre_fork (void)
{
#if SCM_USE_PTHREAD_THREADS
  if (automatic_finalization_p)
    {
      stop_finalization_thread ();
      GC_set_finalizer_notifier (spawn_finalizer_thread);
    }
#endif
}




static void
async_gc_finalizer (void *ptr, void *data)
{
  void **obj = ptr;
  void (*callback) (void) = obj[0];

  callback ();

  scm_i_set_finalizer (ptr, async_gc_finalizer, data);
}

/* Arrange to call CALLBACK asynchronously after each GC.  The callback
   will be invoked from a finalizer, which may be from an async or from
   another thread.

   As an implementation detail, the way this works is that we allocate a
   fresh object and put the callback in the object.  We know that this
   object should get collected the next time GC is run, so we attach a
   finalizer to it to trigger the callback.

   Once the callback runs, we re-attach a finalizer to that fresh object
   to prepare for the next GC, and the process repeats indefinitely.

   We could use the scm_after_gc_hook, but using a finalizer has the
   advantage of potentially running in another thread, decreasing pause
   time.

   Note that libgc currently has a heuristic that adding 500 finalizable
   objects will cause GC to collect rather than expand the heap,
   drastically reducing performance on workloads that actually need to
   expand the heap.  Therefore scm_i_register_async_gc_callback is
   inappropriate for using on unbounded numbers of callbacks.  */
void
scm_i_register_async_gc_callback (void (*callback) (void))
{
  void **obj = GC_MALLOC_ATOMIC (sizeof (void*));

  obj[0] = (void*)callback;

  scm_i_set_finalizer (obj, async_gc_finalizer, NULL);
}


int
scm_set_automatic_finalization_enabled (int enabled_p)
{
  int was_enabled_p = automatic_finalization_p;

  if (enabled_p == was_enabled_p)
    return was_enabled_p;

  if (!scm_initialized_p)
    {
      automatic_finalization_p = enabled_p;
      return was_enabled_p;
    }

  if (enabled_p)
    {
#if SCM_USE_PTHREAD_THREADS
      GC_set_finalizer_notifier (spawn_finalizer_thread);
#else
      GC_set_finalizer_notifier (queue_finalizer_async);
#endif
    }
  else
    {
      GC_set_finalizer_notifier (0);

#if SCM_USE_PTHREAD_THREADS
      stop_finalization_thread ();
#endif
    }

  automatic_finalization_p = enabled_p;

  return was_enabled_p;
}

int
scm_run_finalizers (void)
{
  int finalized = GC_invoke_finalizers ();

  finalization_count += finalized;

  return finalized;
}




void
scm_init_finalizers (void)
{
  /* When the async is to run, the cdr of the pair gets set to the
     asyncs queue of the current thread.  */
  run_finalizers_subr = scm_c_make_gsubr ("%run-finalizers", 0, 0, 0,
                                          run_finalizers_async_thunk);

  if (automatic_finalization_p)
    GC_set_finalizer_notifier (queue_finalizer_async);
}

void
scm_init_finalizer_thread (void)
{
#if SCM_USE_PTHREAD_THREADS
  if (automatic_finalization_p)
    GC_set_finalizer_notifier (spawn_finalizer_thread);
#endif
}
