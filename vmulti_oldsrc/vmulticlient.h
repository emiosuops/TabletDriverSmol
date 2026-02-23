#if !defined(_VMULTI_CLIENT_H_)
#define _VMULTI_CLIENT_H_

#include "vmulticommon.h"

typedef struct _vmulti_client_t* pvmulti_client;

pvmulti_client vmulti_alloc(void);

void vmulti_free(pvmulti_client vmulti);

BOOL vmulti_connect(pvmulti_client vmulti);

void vmulti_disconnect(pvmulti_client vmulti);

BOOL vmulti_update_mouse(pvmulti_client vmulti, BYTE button, USHORT x, USHORT y);



BOOL vmulti_write_message(pvmulti_client vmulti, VMultiMessageReport* pReport);

BOOL vmulti_read_message(pvmulti_client vmulti, VMultiMessageReport* pReport);

#endif
