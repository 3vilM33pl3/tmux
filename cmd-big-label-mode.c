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

#include "tmux.h"

/*
 * Enter big label mode.
 */

static enum cmd_retval	cmd_big_label_mode_exec(struct cmd *, struct cmdq_item *);

const struct cmd_entry cmd_big_label_mode_entry = {
	.name = "big-label-mode",
	.alias = NULL,

	.args = { "t:", 0, 1, NULL },
	.usage = CMD_TARGET_PANE_USAGE " [text]",

	.target = { 't', CMD_FIND_PANE, 0 },

	.flags = CMD_AFTERHOOK,
	.exec = cmd_big_label_mode_exec
};

static enum cmd_retval
cmd_big_label_mode_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = cmd_get_args(self);
	struct cmd_find_state	*target = cmdq_get_target(item);
	struct window_pane	*wp = target->wp;

	window_pane_set_mode(wp, NULL, &window_big_label_mode, target, args);
	return (CMD_RETURN_NORMAL);
}
