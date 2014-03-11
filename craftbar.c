
/********************************************************
 ** F***ing Small Panel 0.7 Copyright (c) 2000-2001 By **
 ** Peter Zelezny <zed@linuxpower.org>                 **
 ** See file COPYING for license details.              **
 ********************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "khash.h"

KHASH_MAP_INIT_INT(Window, Window)
#include "cfg.h"
Display *dd;
Window root_win;
/* WM supports EWMH
This flag is set if the window manager supports the EWMH protocol for e.g.
switching workspaces. The fallback if this is not supported is to use the
Gnome variant. This is determined by looking for the presence of the
_NET_SUPPORTED property of the root window. Note that this is only used
for communication with the WM, whether each client supports this protocol
is up to the individual client. */
int wm_use_ewmh;
int scr_screen;
khash_t(Window) * bindings;

char *atom_names[] = {
	"_NET_SUPPORTED",
	"_NET_CURRENT_DESKTOP",
	"_NET_WM_DESKTOP",
	"_NET_ACTIVE_WINDOW"
};

#define ATOM_COUNT (sizeof (atom_names) / sizeof (atom_names[0]))

Atom atoms[ATOM_COUNT];

#define atom__NET_SUPPORTED atoms[0]
#define atom__NET_CURRENT_DESKTOP atoms[1]
#define atom__NET_WM_DESKTOP atoms[2]
#define atom__NET_ACTIVE_WINDOW atoms[3]

void *get_prop_data(Window win, Atom prop, Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty(dd, win, prop, 0, 0x7fffffff, False,
			   type, &type_ret, &format_ret, &items_ret,
			   &after_ret, &prop_data);
	if (items)
		*items = items_ret;

	return prop_data;
}

void handle_error(Display * d, XErrorEvent * ev)
{
}

unsigned int numlock_mask = 0;
unsigned int scrolllock_mask = 0;
unsigned int capslock_mask = 0;

#include <err.h>

void handle_keypress(XKeyEvent * ev)
{
	unsigned int keycode = ev->keycode;
	unsigned int modifiers = ev->state;

	// never consider capslock, numlock, scrolllock
	modifiers &= ~(capslock_mask | numlock_mask | scrolllock_mask);

	if (modifiers == USE_MASK) {
		/* If this key was remembered, try to raise and focus the window bound
		   to it. If unable to do that, forget the key */
		khiter_t k = kh_get(Window, bindings, keycode);
		if (k == kh_end(bindings))
			return;

		Window w = kh_value(bindings, k);
        warnx("mapping 0x%8hX", (unsigned int)w);

        /* switch current desktop to the window */
        int desktop = -1;
		void* data = get_prop_data(w, atom__NET_WM_DESKTOP, XA_CARDINAL, 0);
        if(data) {
            desktop = *(int*)data;
            XFree(data);
        }
        if(desktop != -1) {
            XEvent ev;
            memset(&ev, 0, sizeof(ev));
            ev.xclient.type = ClientMessage;
            ev.xclient.window = root_win;
            ev.xclient.message_type = atom__NET_CURRENT_DESKTOP;
            ev.xclient.format = 32;
            ev.xclient.data.l[0] = desktop;
            XSendEvent(dd, root_win, 0, PropertyChangeMask, &ev);
        }

        /* raise and focus the window */
		XMapWindow(dd, w);
		XRaiseWindow(dd, w);
		XSetInputFocus(dd, w, RevertToNone, CurrentTime);
        return;
	}

	if (modifiers == BIND_MASK) {
		/* Remember this key to be bound to active window */
		Window focused;
		int rev, t;
		khiter_t k = kh_put(Window, bindings, keycode, &t);

		XGetInputFocus(dd, &focused, &rev);
		if (focused) {
            warnx("remembering 0x%8hX", (unsigned int)focused);
			kh_value(bindings, k) = focused;
		}

		return;
	}
}

void get_offending_modifiers()
{
	/* pulled from xbindkeys */
	int i;
	XModifierKeymap *modmap;
	KeyCode nlock, slock;
	static int mask_table[8] = {
		ShiftMask, LockMask, ControlMask, Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};

	nlock = XKeysymToKeycode(dd, XK_Num_Lock);
	slock = XKeysymToKeycode(dd, XK_Scroll_Lock);

	/*
	 * Find out the masks for the NumLock and ScrollLock modifiers,
	 * so that we can bind the grabs for when they are enabled too.
	 */
	modmap = XGetModifierMapping(dd);

	if (modmap != NULL && modmap->max_keypermod > 0) {
		for (i = 0; i < 8 * modmap->max_keypermod; i++) {
			if (modmap->modifiermap[i] == nlock && nlock != 0)
				numlock_mask =
				    mask_table[i / modmap->max_keypermod];
			else if (modmap->modifiermap[i] == slock && slock != 0)
				scrolllock_mask =
				    mask_table[i / modmap->max_keypermod];
		}
	}

	capslock_mask = LockMask;

	if (modmap)
		XFreeModifiermap(modmap);
}

static const int digits[] = { BINDABLE_KEYS };

const int digits_size = sizeof(digits) / sizeof(digits[0]);	// SUDDENLY 10

void grab_keys()
{
	get_offending_modifiers();
	/*
	 * Grab Ctrl+0-9 & Alt+0-9
	 * */
	for (int j = 0; j < digits_size; j++) {
		int keycode = XKeysymToKeycode(dd, digits[j]);

		for (int i = 0; i < 8; i++) {
			unsigned int modifiers =
			    (i & 1 ? capslock_mask : 0) | (i & 2 ?
							   scrolllock_mask : 0)
			    | (i & 4 ? numlock_mask : 0);

			XGrabKey(dd, keycode,
				 BIND_MASK | modifiers, root_win,
				 False, GrabModeAsync, GrabModeAsync);

			XGrabKey(dd, keycode,
				 USE_MASK | modifiers, root_win,
				 False, GrabModeAsync, GrabModeAsync);
		}
	}
}

int
#ifdef NOSTDLIB
_start(void)
#else
main(int argc, char *argv[])
#endif
{
	bindings = kh_init(Window);
	XEvent ev;
	void *prop;

	dd = XOpenDisplay(NULL);
	if (!dd)
		return 0;
	scr_screen = DefaultScreen(dd);
	root_win = RootWindow(dd, scr_screen);

	grab_keys(dd);
	/* helps us catch windows closing/opening */
	XSelectInput(dd, root_win, PropertyChangeMask);

	XSetErrorHandler((XErrorHandler) handle_error);

	XInternAtoms(dd, atom_names, ATOM_COUNT, False, atoms);

	/* check if the WM supports EWMH
	   Note that this is not reliable. When switching to a EWMH-unaware WM, it
	   will not delete this property. Also, we can't react to changes in this
	   without a restart. */
	prop = get_prop_data(root_win, atom__NET_SUPPORTED, XA_ATOM, NULL);
	if (prop) {
		wm_use_ewmh = 1;
		XFree(prop);
	}

	while (1) {
		while (XPending(dd)) {
			XNextEvent(dd, &ev);
			switch (ev.type) {
			case KeyPress:
				handle_keypress(&ev.xkey);
				break;
				/*default:
				   printf ("unknown evt type: %d\n", ev.type); */
			}
		}
	}

	/*XCloseDisplay (dd);

	   return 0; */
}
