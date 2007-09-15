/*
 * LaTeX.c
 * pidgin-latex plugin
 *
 * This a plugin for Pidgin to display LaTeX formula in conversation
 *
 * PLEASE, send any comment, bug report, etc. to the trackers at sourceforge.net
 *
 * Copyright (C) 2006-2007 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 * 		   Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
 * Windows port  : Copyright (C) 2005-2006 Nicolai Stange (nic-stange@t-online.de)
 * Other portions heavily inspired and copied from gaim sources
 * Copyright (C) 1998-2007 Pidgin developers pidgin.im
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; This document is under the scope of
 * the version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef _PIDGIN_LATEX_H_
#define _PIDGIN_LATEX_H_
#define PURPLE_PLUGINS

//include
#include <libpurple/conversation.h>
//#include <libpurple/core.h>
#include <libpurple/debug.h>
#include <libpurple/signals.h>
#include <libpurple/imgstore.h>
#include <libpurple/util.h>
#include <libpurple/notify.h>
#include <libpurple/server.h>
#include <libpurple/log.h>
#include <libpurple/version.h>

// Constant
#define IMG_BEGIN "<img id=\""
#define IMG_END "\">"

#define BEG "[tex]"
#define END "[/tex]"
#define KOPETE_TEX "$$"

#define LATEX_PLUGIN_ID "qjuh-LaTeX"
#define WEBSITE "http://sourceforge.net/projects/pidgin-latex/"

#define HEADER "\\documentclass[12pt]{article}\\usepackage[dvips]{graphicx}\\usepackage{amsmath}\\usepackage{amssymb}\\pagestyle{empty}"
#define HEADER_MATH "\\begin{document}\\begin{gather*}"

#define FOOTER "\\end{document}"
#define FOOTER_MATH "\\end{gather*}"

#define FILTER_AND "&amp;"
#define FILTER_BR "<br>"

/* Yes, this is simply a copy/paste of KopeteTex blacklist. */
/* But too bad in LaTeX and system security to verify all   */
/* of this */
#define NB_BLACKLIST (42)
#define BLACKLIST {"\\def","\\let","\\futurelet","\\newcommand","\\renewcomment","\\else","\\fi","\\write","\\input","\\include","\\chardef","\\catcode","\\makeatletter","\\noexpand","\\toksdef","\\every","\\errhelp","\\errorstopmode","\\scrollmode","\\nonstopmode","\\batchmode","\\read","\\csname","\\newhelp","\\relax","\\afterground","\\afterassignment","\\expandafter","\\noexpand","\\special","\\command","\\loop","\\repeat","\\toks","\\output","\\line","\\mathcode","\\name","\\item","\\section","\\mbox","\\DeclareRobustCommand"}


// prototypes

/* Verify Blacklist */
/* return true if one word of the message is blacklisted */
static gboolean is_blacklisted(char *message);

/* 
 * latex_to_image creates PNG-image  with the LaTeX code pointed by *latex
 * *latex points to latex-input
 * **file_tex receives filename of temporary generated LaTeX-file; file must be deleted and string must be freed by caller
 * **file_dvi receives filename of temporary generated dvi-file; file must be deleted and string must be freed by caller
 * **file_ps receives filename of temporary generated ps-file; file must be deleted and string must be freed by caller
 * **file_png receives filename of temporary generated png-file; file must be deleted and string must be freed by caller
 *
 * returns TRUE on success, false otherwise
 */
static gboolean latex_to_image(char *latex, char **file_tex, char **file_dvi, char **file_ps, char **file_png);

/*
 * Transform *tmp2 extracting some *startdelim here *enddelim thing, make png-image from latex-input
 *  and tmp2 becomes 'some<img="number">thing'
 * returns TRUE on success, FALSE otherwise
 */
static gboolean analyse(PurpleConversation *conv, char **tmp2, char *startdelim, char *enddelim, gboolean remote);

/*
 * pidgin_latex_write perform the effective write of the latex code in the IM windows
 * 	*conv is a pointer onto the conversation context
 *	*nom is the name of the correspondent
 *	*message is the modified message with the image
 *	*messFlags is Flags related to the messages
 *	*original is the original message unmodified
 * return TRUE.
 */
static gboolean pidgin_latex_write(PurpleConversation *conv, char *nom, char *message, PurpleMessageFlags messFlag, char *original);

/* to intercept outgoing messages */
static gboolean message_send(PurpleAccount *account, const char *who, char **buffer, PurpleConversation *conv, PurpleMessageFlags flags);

/* to intercept incoming messages */
static gboolean message_recv(PurpleAccount *account, char **sender, char **buffer, PurpleConversation *conv, PurpleMessageFlags *flags);


#define TMPPRE "gltx" /* temorary files will be prefixed by TMPPRE */
/*
 * getdirname returns the directory's part of a filename.
 * Parsing is done OS-dependently (with path-separator as defined in glib/gutils.h)
 *      *path is the filename you want to extract a directory-part from
 * return directory's part on success, NULL otherwise; must be freed with
 * free()!
 */
static char* getdirname(const char const *file);

/*
 * get_latex_cmd returns a system()-executable-command for latex
 *
 * return the executable command if successfull, NULL otherwise; must be
 * freed with free()
 */
static char* get_latex_cmd();

/*
 * get_dvips_cmd returns a system()-executable-command for dvips
 *
 * return the executable command if successfull, NULL otherwise; must be
 * freed with free()
 */
static char* get_dvips_cmd();

/*
 * get_convert_cmd returns a system()-executable-command for convert
 *
 * return the executable command if successfull, NULL otherwise; must be
 * freed with free()
 */
static char* get_convert_cmd();

/*
 * win32_purple_notify extracts error information of last occured WIN32-Error due to an API-call and asserts it to gaim via purple_notify_error
 * Error message will be prepended by *prep:
 */
void win32_purple_notify_error(char *prep);

/*
 * searchPATH searches the PATH-environment for the specified file file.
 * *file is the name of the executable to search for
 * returns the right full path, e.g. with the executable's name appended, NULL on failure, must be freed with free()
 */
char* searchPATH(const char const *file);

/*
 * execute executes the *cmd with opts appended WITHOUT ANY SPACES INBETWEEN to the commandline.
 * Advantage to system()-call under win32 ist that no console window pops up.
 * On systems other than windows this function is just a wrapper around the system()-call.
 * *cmd is the comannd to execute
 * *opts[] is an array of elements to be appended to the commandline
 * copts is the count of elements within *opts[]
 *
 * returns -1 if execution failed, otherwise the return code of the executed program.
*/
#ifdef _WIN32
static DWORD execute(char *cmd, char *opts[], int copts);
#else
static int execute(char *cmd, char *opts[], int copts);
#endif


#endif
