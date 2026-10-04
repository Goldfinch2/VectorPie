/* vp_error.h — result codes shared by every VectorPie SDK call.
 *
 * Every function that can fail returns an int: VP_OK (0) on success, or one of
 * the negative codes below. vp_last_error() gives the human-readable detail of
 * the most recent failure on the calling thread (e.g. the OS error text when a
 * port cannot be opened).
 */
#ifndef VECTORPIE_VP_ERROR_H
#define VECTORPIE_VP_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VP_OK            =  0,
    VP_ERR_INVALID   = -1,  /* a bad argument */
    VP_ERR_NOMEM     = -2,  /* out of memory */
    VP_ERR_NODEV     = -3,  /* no DVG port configured, or it could not be opened */
    VP_ERR_IO        = -4,  /* a read or write on the port failed */
    VP_ERR_TIMEOUT   = -5,  /* the board did not answer in time */
    VP_ERR_PROTOCOL  = -6,  /* the board answered something unexpected */
    VP_ERR_NOTFOUND  = -7   /* a file or key does not exist */
};

/* Short fixed description of a result code ("timeout", ...). Never NULL. */
const char *vp_strerror(int code);

/* Detail of the last failure on this thread, or "" if none. Never NULL. The
 * text stays valid until the next failing SDK call on the same thread. */
const char *vp_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_ERROR_H */
