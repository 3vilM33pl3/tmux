/* $OpenBSD$ */

/*
 * Copyright (c) 2026 Olivier Van Acker <olivier@robotmotel.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tmux.h"

#define WINDOW_BIG_LABEL_WIDTH 5
#define WINDOW_BIG_LABEL_HEIGHT 7
#define WINDOW_BIG_LABEL_SPACING 1
#define WINDOW_BIG_LABEL_STEP (WINDOW_BIG_LABEL_WIDTH + WINDOW_BIG_LABEL_SPACING)
#define WINDOW_BIG_LABEL_WHITE 15

struct window_big_label_mode_data {
	struct screen		screen;
	char		       *label;
	int			colour;
	TAILQ_ENTRY(window_big_label_mode_data) entry;
};

struct window_big_label_colour_data {
	u_int			pane_id;
	int			colour;
	TAILQ_ENTRY(window_big_label_colour_data) entry;
};

TAILQ_HEAD(window_big_label_modes, window_big_label_mode_data);
static struct window_big_label_modes	window_big_label_modes =
    TAILQ_HEAD_INITIALIZER(window_big_label_modes);
TAILQ_HEAD(window_big_label_colours, window_big_label_colour_data);
static struct window_big_label_colours	window_big_label_colours =
    TAILQ_HEAD_INITIALIZER(window_big_label_colours);
static u_int	window_big_label_pick_serial;

static struct screen *window_big_label_init(struct window_mode_entry *,
		    struct cmd_find_state *, struct args *);
static void	window_big_label_free(struct window_mode_entry *);
static void	window_big_label_resize(struct window_mode_entry *, u_int, u_int);
static void	window_big_label_key(struct window_mode_entry *, struct client *,
		    struct session *, struct winlink *, key_code,
		    struct mouse_event *);

static void	window_big_label_draw_screen(struct window_mode_entry *);
static int	window_big_label_pick_colour(struct window_pane *);
static const u_char *window_big_label_glyph(u_char);

const struct window_mode window_big_label_mode = {
	.name = "big-label-mode",

	.init = window_big_label_init,
	.free = window_big_label_free,
	.resize = window_big_label_resize,
	.key = window_big_label_key,
};

struct window_big_label_glyph {
	u_char	ch;
	u_char	rows[WINDOW_BIG_LABEL_HEIGHT];
};

static const struct window_big_label_glyph window_big_label_glyphs[] = {
	{ 'A', { 0x0e,0x11,0x11,0x1f,0x11,0x11,0x11 } },
	{ 'B', { 0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e } },
	{ 'C', { 0x0e,0x11,0x10,0x10,0x10,0x11,0x0e } },
	{ 'D', { 0x1e,0x11,0x11,0x11,0x11,0x11,0x1e } },
	{ 'E', { 0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f } },
	{ 'F', { 0x1f,0x10,0x10,0x1e,0x10,0x10,0x10 } },
	{ 'G', { 0x0e,0x11,0x10,0x17,0x11,0x11,0x0e } },
	{ 'H', { 0x11,0x11,0x11,0x1f,0x11,0x11,0x11 } },
	{ 'I', { 0x1f,0x04,0x04,0x04,0x04,0x04,0x1f } },
	{ 'J', { 0x07,0x02,0x02,0x02,0x02,0x12,0x0c } },
	{ 'K', { 0x11,0x12,0x14,0x18,0x14,0x12,0x11 } },
	{ 'L', { 0x10,0x10,0x10,0x10,0x10,0x10,0x1f } },
	{ 'M', { 0x11,0x1b,0x15,0x15,0x11,0x11,0x11 } },
	{ 'N', { 0x11,0x19,0x15,0x13,0x11,0x11,0x11 } },
	{ 'O', { 0x0e,0x11,0x11,0x11,0x11,0x11,0x0e } },
	{ 'P', { 0x1e,0x11,0x11,0x1e,0x10,0x10,0x10 } },
	{ 'Q', { 0x0e,0x11,0x11,0x11,0x15,0x12,0x0d } },
	{ 'R', { 0x1e,0x11,0x11,0x1e,0x14,0x12,0x11 } },
	{ 'S', { 0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e } },
	{ 'T', { 0x1f,0x04,0x04,0x04,0x04,0x04,0x04 } },
	{ 'U', { 0x11,0x11,0x11,0x11,0x11,0x11,0x0e } },
	{ 'V', { 0x11,0x11,0x11,0x11,0x11,0x0a,0x04 } },
	{ 'W', { 0x11,0x11,0x11,0x15,0x15,0x15,0x0a } },
	{ 'X', { 0x11,0x11,0x0a,0x04,0x0a,0x11,0x11 } },
	{ 'Y', { 0x11,0x11,0x0a,0x04,0x04,0x04,0x04 } },
	{ 'Z', { 0x1f,0x01,0x02,0x04,0x08,0x10,0x1f } },

	{ '0', { 0x0e,0x11,0x13,0x15,0x19,0x11,0x0e } },
	{ '1', { 0x04,0x0c,0x04,0x04,0x04,0x04,0x0e } },
	{ '2', { 0x0e,0x11,0x01,0x02,0x04,0x08,0x1f } },
	{ '3', { 0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e } },
	{ '4', { 0x02,0x06,0x0a,0x12,0x1f,0x02,0x02 } },
	{ '5', { 0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e } },
	{ '6', { 0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e } },
	{ '7', { 0x1f,0x01,0x02,0x04,0x08,0x08,0x08 } },
	{ '8', { 0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e } },
	{ '9', { 0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e } },

	{ ' ', { 0x00,0x00,0x00,0x00,0x00,0x00,0x00 } },
	{ '-', { 0x00,0x00,0x00,0x1f,0x00,0x00,0x00 } },
	{ '_', { 0x00,0x00,0x00,0x00,0x00,0x00,0x1f } },
	{ '.', { 0x00,0x00,0x00,0x00,0x00,0x00,0x04 } },
	{ ':', { 0x00,0x04,0x00,0x00,0x00,0x04,0x00 } },
	{ '!', { 0x04,0x04,0x04,0x04,0x04,0x00,0x04 } },
	{ '+', { 0x00,0x04,0x04,0x1f,0x04,0x04,0x00 } },
	{ '/', { 0x01,0x02,0x04,0x08,0x10,0x00,0x00 } },
	{ '?', { 0x0e,0x11,0x01,0x02,0x04,0x00,0x04 } },
};

static const int window_big_label_pastels[] = {
	95, 96, 101, 102, 103, 131, 132, 137, 138, 139, 144, 145
};

static const u_char *
window_big_label_glyph(u_char ch)
{
	u_int	i;

	ch = toupper(ch);
	for (i = 0; i < nitems(window_big_label_glyphs); i++) {
		if (window_big_label_glyphs[i].ch == ch)
			return (window_big_label_glyphs[i].rows);
	}
	for (i = 0; i < nitems(window_big_label_glyphs); i++) {
		if (window_big_label_glyphs[i].ch == '?')
			return (window_big_label_glyphs[i].rows);
	}
	return (window_big_label_glyphs[0].rows);
}

static int
window_big_label_pick_colour(struct window_pane *wp)
{
	struct window_big_label_mode_data	*data;
	struct window_big_label_colour_data	*colour_data;
	int					 used[nitems(window_big_label_pastels)];
	int					 last;
	u_int					 i, start, offset, count;

	count = nitems(window_big_label_pastels);
	memset(used, 0, sizeof used);
	last = -1;

	TAILQ_FOREACH(data, &window_big_label_modes, entry) {
		for (i = 0; i < count; i++) {
			if (window_big_label_pastels[i] == data->colour)
				used[i] = 1;
		}
	}
	TAILQ_FOREACH(colour_data, &window_big_label_colours, entry) {
		if (colour_data->pane_id == wp->id) {
			last = colour_data->colour;
			break;
		}
	}

	start = ((u_int)time(NULL) + wp->id * 97 + window_big_label_pick_serial++) %
	    count;
	for (offset = 0; offset < count; offset++) {
		i = (start + offset) % count;
		if (!used[i] && window_big_label_pastels[i] != last)
			return (window_big_label_pastels[i]);
	}
	for (offset = 0; offset < count; offset++) {
		i = (start + offset) % count;
		if (!used[i])
			return (window_big_label_pastels[i]);
	}
	for (offset = 0; offset < count; offset++) {
		i = (start + offset) % count;
		if (window_big_label_pastels[i] != last)
			return (window_big_label_pastels[i]);
	}
	return (window_big_label_pastels[start]);
}

static struct screen *
window_big_label_init(struct window_mode_entry *wme, struct cmd_find_state *fs,
    struct args *args)
{
	struct window_pane			*wp = wme->wp;
	struct window_big_label_mode_data	*data;
	struct window_big_label_colour_data	*colour_data;
	struct screen				*s;
	const char				*text;
	char					*ptr;

	wme->data = data = xcalloc(1, sizeof *data);

	text = NULL;
	if (args != NULL && args_count(args) != 0)
		text = args_string(args, 0);
	if (text == NULL || *text == '\0') {
		if (fs != NULL && fs->s != NULL && fs->s->name != NULL)
			text = fs->s->name;
		else if (wp->window->name != NULL)
			text = wp->window->name;
	}
	if (text == NULL || *text == '\0')
		text = "TMUX";

	data->label = xstrdup(text);
	for (ptr = data->label; *ptr != '\0'; ptr++)
		*ptr = toupper((u_char)*ptr);
	data->colour = window_big_label_pick_colour(wp);
	TAILQ_FOREACH(colour_data, &window_big_label_colours, entry) {
		if (colour_data->pane_id == wp->id)
			break;
	}
	if (colour_data == NULL) {
		colour_data = xcalloc(1, sizeof *colour_data);
		colour_data->pane_id = wp->id;
		TAILQ_INSERT_TAIL(&window_big_label_colours, colour_data, entry);
	}
	colour_data->colour = data->colour;
	TAILQ_INSERT_TAIL(&window_big_label_modes, data, entry);

	s = &data->screen;
	screen_init(s, screen_size_x(&wp->base), screen_size_y(&wp->base), 0);
	s->mode &= ~MODE_CURSOR;

	window_big_label_draw_screen(wme);
	return (s);
}

static void
window_big_label_free(struct window_mode_entry *wme)
{
	struct window_big_label_mode_data	*data = wme->data;

	TAILQ_REMOVE(&window_big_label_modes, data, entry);
	screen_free(&data->screen);
	free(data->label);
	free(data);
}

static void
window_big_label_resize(struct window_mode_entry *wme, u_int sx, u_int sy)
{
	struct window_big_label_mode_data	*data = wme->data;
	struct screen				*s = &data->screen;

	screen_resize(s, sx, sy, 0);
	window_big_label_draw_screen(wme);
}

static void
window_big_label_key(struct window_mode_entry *wme, __unused struct client *c,
    __unused struct session *s, __unused struct winlink *wl,
    __unused key_code key, __unused struct mouse_event *m)
{
	window_pane_reset_mode(wme->wp);
}

static void
window_big_label_draw_screen(struct window_mode_entry *wme)
{
	struct window_big_label_mode_data	*data = wme->data;
	struct screen				*s = &data->screen;
	struct screen_write_ctx			 ctx;
	struct grid_cell			 gc, pixel;
	const u_char				*rows;
	u_int					 sx, sy, len;
	u_int					 content_width, x, y, i, j, k;
	u_char					 ch;

	screen_write_start(&ctx, s);
	screen_write_clearscreen(&ctx, data->colour);

	len = strlen(data->label);
	sx = screen_size_x(s);
	sy = screen_size_y(s);
	if (len == 0 || sx == 0 || sy == 0) {
		screen_write_stop(&ctx);
		return;
	}

	content_width = len * WINDOW_BIG_LABEL_STEP - WINDOW_BIG_LABEL_SPACING;

	memcpy(&gc, &grid_default_cell, sizeof gc);
	gc.flags |= GRID_FLAG_NOPALETTE;
	gc.fg = WINDOW_BIG_LABEL_WHITE;
	gc.bg = data->colour;

	if (sx < content_width || sy < WINDOW_BIG_LABEL_HEIGHT) {
		if (sx > len)
			x = (sx - len) / 2;
		else
			x = 0;
		y = sy / 2;
		screen_write_cursormove(&ctx, x, y, 0);
		screen_write_nputs(&ctx, sx - x, &gc, "%s", data->label);
		screen_write_stop(&ctx);
		return;
	}

	x = (sx - content_width) / 2;
	y = (sy - WINDOW_BIG_LABEL_HEIGHT) / 2;

	memcpy(&pixel, &grid_default_cell, sizeof pixel);
	pixel.flags |= GRID_FLAG_NOPALETTE;
	pixel.bg = WINDOW_BIG_LABEL_WHITE;
	pixel.fg = data->colour;

	for (k = 0; k < len; k++) {
		ch = data->label[k];
		rows = window_big_label_glyph(ch);

		for (j = 0; j < WINDOW_BIG_LABEL_HEIGHT; j++) {
			for (i = 0; i < WINDOW_BIG_LABEL_WIDTH; i++) {
				if (rows[j] & (1 << (WINDOW_BIG_LABEL_WIDTH - 1 - i))) {
					screen_write_cursormove(&ctx, x + i, y + j, 0);
					screen_write_putc(&ctx, &pixel, ' ');
				}
			}
		}

		x += WINDOW_BIG_LABEL_STEP;
	}

	screen_write_stop(&ctx);
}
