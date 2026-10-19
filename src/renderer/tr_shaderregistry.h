/*
 * @fidelity: verified
 */
#ifndef TR_SHADERREGISTRY_H
#define TR_SHADERREGISTRY_H

typedef struct {
    int numShaders;             /* 0x016CB848            */
    int shaders[4096];          /* 0x016CB84C  shader_t* */
    int sortedShaders[4096];    /* 0x016CF84C  shader_t* */
} tr_shaderRegistry_t;
typedef char tr_sr_assert_shaders_off[
    ((int)(long)&((tr_shaderRegistry_t *)0)->shaders - 4) == 0 ? 1 : -1];
typedef char tr_sr_assert_sorted_off[
    ((int)(long)&((tr_shaderRegistry_t *)0)->sortedShaders - 0x4004) == 0 ? 1 : -1];
typedef char tr_sr_assert_size[
    sizeof(tr_shaderRegistry_t) == 0x8004 ? 1 : -1];

/* tr.numShaders / tr.shaders / tr.sortedShaders: fields of the renderer's tr object
   (TRG blob 0x016C4D40 + 0x6B08 = 0x016CB848) */
#define tr_shaderRegistry   (*(tr_shaderRegistry_t *)(TRG + 0x6B08))

#define tr_numShaders   ((int *)(void *)&tr_shaderRegistry)
#define tr_shaders   (tr_shaderRegistry.shaders)
#define tr_sortedShaderSlots   (tr_shaderRegistry.sortedShaders)
#define tr_sortedShaderSlots_1   (tr_shaderRegistry.sortedShaders + 1)

#endif
