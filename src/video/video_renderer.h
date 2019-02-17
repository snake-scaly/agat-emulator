#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "videoint.h"

/*
A video renderer renders emulated memory contents onto an off-screen
surface.

Each renderer is hard-coded to render into a surface of a certain size
and type.  Compatible renderers render on a surface of the same type.
Only compatible renderers can be combined.

<VIDEO_RENDERER> contents never change at runtime. The list of supported
renderers is hard-coded.

Field: surface_type

  Target surface type expected by this renderer.

Field: support_flash

  Whether this renderer supports hardware flashing like 32x32 text renderer.

Function: render_line

  Render a whole horizontal line containing <scanline>.

  The line height depends on the renderer.  For low-resolution and text
  renderers calling this function will overwrite multiple scanlines on
  the target surface.

  Parameters:

    vs - video state
    page - video page number to render. The mapping between page numbers
      and memory addresses is renderer-specific
    scanline - scanline number that must be covered by the rendered line.
      Scanlines correspond to line numbers on the target surface
    target - surface to render into
    rect - receives the modified target surface rectangle

  Return:

    First scanline before the rendered line which was not modified.

Function: map_address

  Determine the range of scanlines affected by modification of a given
  memory address.

  Parameters:

    page - renderer-specific page number
    addr - modified address
    first_scanline - receives the index of the first scanline affected
      by the change. Only valid if return value is non-zero
    last_scanline - receives the index of the first scanline NOT affected
      by the change. Only valid if return value is non-zero

  Return:

    Non-zero if modifying addr affects visual representation of the
    given page; zero if not.

Function: round_scanline

  Round the given scanline number to the next full line a renderer can render.

  Parameters:

    scanline - the raw scanline in the target surface

  Return:

    A scanline at the start of one of the renderer's lines. A scanline past the
    last line of the renderer is also valid, meaning that the input scanline
    was too close to the end of the screen to render anything.

    The returned scanline is never smaller than the input.
*/
struct VIDEO_RENDERER
{
	enum RENDER_SURFACE_TYPE surface_type;
	int support_flash;
	int (*render_line)(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect);
	int (*map_address)(int page, int addr, int*first_scanline, int*last_scanline);
	int (*round_scanline)(int scanline);
};

/*
Determine video renderer and page number corresponding to an I/O address.

Parameters:

  io_address - I/O address such as 0xC036
  page - receives the page number

Return:

  Video renderer descriptor.
*/
const struct VIDEO_RENDERER*ng_get_renderer(struct VIDEO_STATE*vs, int mode, int*page);
const struct VIDEO_RENDERER*ng_apple_get_current_text_renderer(struct VIDEO_STATE*vs, int*page);

#endif // VIDEO_RENDERER_H
