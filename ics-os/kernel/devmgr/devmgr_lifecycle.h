#ifndef ICSOS_DEVMGR_LIFECYCLE_H
#define ICSOS_DEVMGR_LIFECYCLE_H

/* Registry lifecycle. Accessors are called while devmgr_busy is held. */
#define DEVMGR_STATE_FREE       0
#define DEVMGR_STATE_LIVE       1
#define DEVMGR_STATE_QUIESCING  2
#define DEVMGR_STATE_DEAD       3

static inline int devmgr_lifecycle_quiesce(int *state)
{
   if (!state)
      return 0;
   if (*state == DEVMGR_STATE_LIVE)
      *state = DEVMGR_STATE_QUIESCING;
   return *state == DEVMGR_STATE_QUIESCING;
}

static inline int devmgr_lifecycle_get(int state, DWORD *refs)
{
   if (state != DEVMGR_STATE_LIVE || !refs || *refs == ~(DWORD)0)
      return 0;
   (*refs)++;
   return 1;
}

static inline int devmgr_lifecycle_put(DWORD *refs)
{
   if (!refs || *refs == 0)
      return 0;
   (*refs)--;
   return 1;
}

static inline int devmgr_lifecycle_can_retire(int state, DWORD refs)
{
   return state == DEVMGR_STATE_QUIESCING && refs == 0;
}

#endif
