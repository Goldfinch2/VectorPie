/* vp_version.h — the SDK's version, at compile time and at run time. */
#ifndef VECTORPIE_VP_VERSION_H
#define VECTORPIE_VP_VERSION_H

#define VP_VERSION_MAJOR 0
#define VP_VERSION_MINOR 1
#define VP_VERSION_PATCH 0
#define VP_VERSION_STRING "0.1.0"

#ifdef __cplusplus
extern "C" {
#endif

/* The version of the library actually linked, which can differ from the
 * headers a game was compiled against. */
const char *vp_version(void);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_VERSION_H */
